#include "overlay.h"
#include "../render/render.h"
#include "../render/gui/c_gui.hpp"
#include "../core/entity/c_visuals.hpp"
#include "../core/raycast/c_raycast.hpp"
#include "../core/settings/settings.h"
#include "../classes/c_world/c_world.h"

#include <cstdio>

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dwmapi.lib" )

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

c_overlay::c_overlay( )
{
	m_visible = false;
	m_dpi_changed = false;
	m_window = nullptr;
	m_game_window = nullptr;
	m_device = nullptr;
	m_device_context = nullptr;
	m_render_target_view = nullptr;
	m_swap_chain = nullptr;
	m_margins = { -1 };
	m_width = 0;
	m_height = 0;
	m_running = false;
}

c_overlay::~c_overlay( )
{
	if ( m_render_target_view )
	{
		m_render_target_view->Release( );
		m_render_target_view = nullptr;
	}
	if ( m_swap_chain )
	{
		m_swap_chain->Release( );
		m_swap_chain = nullptr;
	}
	if ( m_device_context )
	{
		m_device_context->Release( );
		m_device_context = nullptr;
	}
	if ( m_device )
	{
		m_device->Release( );
		m_device = nullptr;
	}
	if ( m_window )
	{
		DestroyWindow( m_window );
		m_window = nullptr;
	}
}

bool c_overlay::setup_overlay( ) noexcept
{
	m_window = FindWindowA( "Chrome_WidgetWin_1", "Discord Overlay" );
	if ( !m_window )
		return false;

	m_game_window = FindWindowW( L"UnrealWindow", NULL );
	if ( !m_game_window )
		return false;

	DwmExtendFrameIntoClientArea( m_window, &m_margins );
	SetLayeredWindowAttributes( m_window, RGB( 0, 0, 0 ), 255, LWA_ALPHA );
	SetWindowDisplayAffinity( m_window, WDA_EXCLUDEFROMCAPTURE );

	ShowWindow( m_window, SW_SHOW );
	UpdateWindow( m_window );

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory( &scd, sizeof( scd ) );
	scd.BufferCount = 2;
	scd.BufferDesc.Width = 0;
	scd.BufferDesc.Height = 0;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = m_window;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.Windowed = TRUE;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT flags = 0;
	D3D_FEATURE_LEVEL used_level;
	D3D_FEATURE_LEVEL levels[ ] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

	if ( FAILED( D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels, 2,
		D3D11_SDK_VERSION, &scd, &m_swap_chain, &m_device, &used_level, &m_device_context ) ) )
		return false;

	ID3D11Texture2D* back_buffer = nullptr;
	m_swap_chain->GetBuffer( 0, IID_PPV_ARGS( &back_buffer ) );
	if ( !back_buffer )
		return false;

	m_device->CreateRenderTargetView( back_buffer, nullptr, &m_render_target_view );
	back_buffer->Release( );
	return true;
}

bool c_overlay::setup_imgui( ) noexcept
{
	IMGUI_CHECKVERSION( );
	ImGui::CreateContext( );
	ImGuiIO& io = ImGui::GetIO( );
	ImGui::StyleColorsDark( );
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	ImGui_ImplWin32_Init( m_window );
	ImGui_ImplDX11_Init( m_device, m_device_context );

	m_running = true;
	MSG msg;

	while ( m_running )
	{
		while ( PeekMessage( &msg, nullptr, 0U, 0U, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
			if ( msg.message == WM_QUIT )
			{
				m_running = false;
				break;
			}
		}
		if ( !m_running )
			break;

		RECT game_rect;
		if ( GetClientRect( m_game_window, &game_rect ) )
		{
			const int new_width = game_rect.right - game_rect.left;
			const int new_height = game_rect.bottom - game_rect.top;
			POINT client_origin = { 0, 0 };

			if ( ClientToScreen( m_game_window, &client_origin ) )
			{
				static int cached_x = -1, cached_y = -1, cached_w = -1, cached_h = -1;
				if ( client_origin.x != cached_x || client_origin.y != cached_y ||
					 new_width != cached_w || new_height != cached_h )
				{
					SetWindowPos( m_window, HWND_TOPMOST, client_origin.x, client_origin.y,
						new_width, new_height, SWP_NOACTIVATE );
					cached_x = client_origin.x;
					cached_y = client_origin.y;
					cached_w = new_width;
					cached_h = new_height;
				}
			}

			if ( m_width != new_width || m_height != new_height )
			{
				m_width = new_width;
				m_height = new_height;
				render::set_screen_size( m_width, m_height );

				if ( m_render_target_view )
				{
					m_render_target_view->Release( );
					m_render_target_view = nullptr;
				}
				if ( SUCCEEDED( m_swap_chain->ResizeBuffers( 0, m_width, m_height, DXGI_FORMAT_UNKNOWN, 0 ) ) )
				{
					ID3D11Texture2D* back_buffer = nullptr;
					if ( SUCCEEDED( m_swap_chain->GetBuffer( 0, IID_PPV_ARGS( &back_buffer ) ) ) )
					{
						if ( back_buffer )
						{
							m_device->CreateRenderTargetView( back_buffer, nullptr, &m_render_target_view );
							back_buffer->Release( );
						}
					}
				}
			}
		}

		ImGuiIO& cur_io = ImGui::GetIO( );
		POINT cursor;
		if ( GetCursorPos( &cursor ) )
		{
			POINT origin = { 0, 0 };
			if ( ClientToScreen( m_game_window, &origin ) )
			{
				cur_io.MousePos = ImVec2(
					static_cast< float >( cursor.x - origin.x ),
					static_cast< float >( cursor.y - origin.y ) );
			}
		}
		cur_io.MouseDown[ 0 ] = ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 ) != 0;

		static bool was_insert_pressed = false;
		bool is_insert_pressed = ( GetAsyncKeyState( VK_INSERT ) & 0x8000 ) != 0;
		if ( is_insert_pressed && !was_insert_pressed )
			m_visible = !m_visible;
		was_insert_pressed = is_insert_pressed;

		const float clear_color[ 4 ] = { 0.f, 0.f, 0.f, 0.f };
		m_device_context->OMSetRenderTargets( 1, &m_render_target_view, nullptr );
		m_device_context->ClearRenderTargetView( m_render_target_view, clear_color );

		ImGui_ImplDX11_NewFrame( );
		ImGui_ImplWin32_NewFrame( );
		ImGui::NewFrame( );

		camera_info_t cam_info{ };
		const bool cam_valid = get_camera_info( cam_info );

		if ( cam_valid )
			g_visual->on_render( m_width, m_height, cam_info );

		if ( cam_valid && g_settings->map_wireframe )
			g_raycast->render_geometry( m_width, m_height, cam_info );

		if ( m_visible )
			g_render->on_render_menu( m_visible );

		ImGui::Render( );
		ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );
		m_swap_chain->Present( 1, 0 );
	}

	ImGui_ImplDX11_Shutdown( );
	ImGui_ImplWin32_Shutdown( );
	ImGui::DestroyContext( );
	return true;
}
