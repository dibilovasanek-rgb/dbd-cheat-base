#pragma once
#include <cstdint>
#include <string>
#include <cstdio>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"

namespace fname
{
	inline uintptr_t g_base = 0;

	inline void init( ) noexcept
	{
		if ( !g_base )
			g_base = g_memory->get_base_address( );
	}

	inline std::string get_name_by_id( uint32_t name_id, bool debug = false ) noexcept
	{
		init( );

		if ( name_id == 0 )
			return "None";

		uint32_t chunk_idx = name_id >> 16;
		uint16_t name_idx = static_cast< uint16_t >( name_id );

		uint64_t pool = g_base + offsets::g_name_table;
		uint64_t block_ptr = g_memory->read< uint64_t >( pool + 0x10 + chunk_idx * 8 );

		if ( debug )
		{
			printf( "[fname] id=%u (chunk=%u name=%u)\n", name_id, chunk_idx, name_idx );
			printf( "[fname] pool=0x%llX block_ptr=0x%llX\n", pool, block_ptr );
		}

		if ( !block_ptr )
			return "";

		uint64_t entry = block_ptr + static_cast< uint64_t >( name_idx ) * 4;
		uint16_t header = g_memory->read< uint16_t >( entry + 4 );

		uint32_t len = header >> 1;
		bool is_wide = header & 1;

		if ( debug )
			printf( "[fname] entry=0x%llX header=0x%04X len=%u wide=%d\n", entry, header, len, is_wide );

		if ( len == 0 || len > 255 )
			return "";

		if ( is_wide )
		{
			wchar_t wide_buf[ 256 ] = { };
			g_memory->read_raw( entry + 6, wide_buf, len * 2 );
			char buf[ 256 ] = { };
			for ( uint32_t i = 0; i < len && wide_buf[ i ]; i++ )
				buf[ i ] = static_cast< char >( wide_buf[ i ] );
			if ( debug ) printf( "[fname] result=\"%s\"\n", buf );
			return std::string( buf );
		}
		else
		{
			char buf[ 256 ] = { };
			g_memory->read_raw( entry + 6, buf, len );
			if ( debug ) printf( "[fname] result=\"%s\"\n", buf );
			return std::string( buf, len );
		}
	}
}
