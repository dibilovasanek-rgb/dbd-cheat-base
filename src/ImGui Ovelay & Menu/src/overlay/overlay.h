#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>
#include <Windows.h>

#include "../../thirdparty/imgui/imgui.h"
#include "../../thirdparty/imgui/imgui_impl_dx11.h"
#include "../../thirdparty/imgui/imgui_impl_win32.h"

class c_overlay
{
public:
	c_overlay( );
	~c_overlay( );

	bool setup_overlay( ) noexcept;
	bool setup_imgui( ) noexcept;

	bool m_visible;
	bool m_running;
	int m_width;
	int m_height;

private:
	HWND m_window;
	HWND m_game_window;
	MARGINS m_margins;
	bool m_dpi_changed;

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_device_context;
	ID3D11RenderTargetView* m_render_target_view;
	IDXGISwapChain* m_swap_chain;
};
