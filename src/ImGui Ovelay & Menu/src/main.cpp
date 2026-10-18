#include "memory/memory.h"
#include "core/process/process.h"
#include "core/settings/settings.h"
#include "core/raycast/c_raycast.hpp"
#include "render/render.h"
#include "overlay/overlay.h"

#include <cstdio>
#include <thread>

int main( )
{
	int process_id = get_process_id( );
	if ( !process_id )
	{
		printf( "process not found\n" );
		return 1;
	}

	g_memory->set_process_id( process_id );
	printf( "[INFO] Pid = %d\n", process_id );

	uintptr_t base = g_memory->get_base_address( );
	printf( "[INFO] Base = 0x%p\n", reinterpret_cast< void* >( base ) );

	if ( !base )
	{
		printf( "base not found\n" );
		return 1;
	}

	g_raycast->start_worker( );

	c_overlay overlay{ };
	if ( !overlay.setup_overlay( ) )
	{
		printf( "overlay setup failed\n" );
		g_raycast->stop_worker( );
		return 1;
	}

	render::set_screen_size( overlay.m_width, overlay.m_height );
	overlay.setup_imgui( );

	g_raycast->stop_worker( );
	return 0;
}
