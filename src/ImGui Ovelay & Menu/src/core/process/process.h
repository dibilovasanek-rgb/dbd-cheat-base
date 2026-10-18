#pragma once
#include <Windows.h>
#include <cstdint>

struct search_data_t
{
	DWORD process_id;
	const char* target_class;
};

static BOOL CALLBACK enum_windows_callback( HWND hwnd, LPARAM lparam )
{
	auto data = reinterpret_cast< search_data_t* >( lparam );
	char class_name[ 256 ];
	if ( GetClassNameA( hwnd, class_name, sizeof( class_name ) ) )
	{
		if ( strcmp( class_name, data->target_class ) == 0 )
		{
			GetWindowThreadProcessId( hwnd, &data->process_id );
			return FALSE;
		}
	}
	return TRUE;
}

static DWORD get_process_id( ) noexcept
{
	search_data_t data{ };
	data.process_id = 0;
	data.target_class = "UnrealWindow";
	EnumWindows( enum_windows_callback, reinterpret_cast< LPARAM >( &data ) );
	return data.process_id;
}
