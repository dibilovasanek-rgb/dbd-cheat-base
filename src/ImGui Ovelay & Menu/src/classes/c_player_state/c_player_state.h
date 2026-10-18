#pragma once
#include <cstdint>
#include <string>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../c_pawn/c_pawn.h"
enum class e_game_state : uint8_t
{
	VE_Active = 0,
	VE_Killed = 1,
	VE_Escaped = 2,
	VE_EscapedInjured = 3,
	VE_Sacrificed = 4,
	VE_None = 5,
	VE_Disconnected = 6,
	VE_ManuallyLeftMatch = 7,
	VE_BledOut = 8,
	VE_SurrenderLoss = 9,
	VE_SurrenderDraw = 10,
	VE_SurrenderWin = 11,
	VE_MAX = 12,
};
class c_player_state
{
public:
	c_pawn* get_pawn() const noexcept
	{
		return g_memory->read< c_pawn* >(
			reinterpret_cast<uintptr_t>(this) + offsets::player_state::pawn_private);
	}
	std::string get_player_name( ) const noexcept
	{
		const uintptr_t name_addr =
			reinterpret_cast< uintptr_t >( this ) + offsets::player_state::player_name;

		uint64_t data = g_memory->read< uint64_t >( name_addr );
		int32_t len = g_memory->read< int32_t >( name_addr + 0x8 );

		if ( !data || len <= 0 || len > 256 )
			return std::string( "NULL" );

		char buffer[ 256 ] = { };
		int read_len = ( len < 128 ) ? len : 127;

		wchar_t wide_buffer[ 128 ] = { };
		g_memory->read_raw( data, wide_buffer, read_len * 2 );

		for ( int i = 0; i < read_len && wide_buffer[ i ]; i++ )
			buffer[ i ] = static_cast< char >( wide_buffer[ i ] );

		return std::string( buffer );
	}


};
