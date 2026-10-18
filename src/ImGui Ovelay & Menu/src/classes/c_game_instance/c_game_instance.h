#pragma once
#include <cstdint>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../../types/types.h"
#include "../c_local_player/c_local_player.h"

class c_game_instance
{
public:
	array_t get_local_players( ) const noexcept
	{
		return g_memory->read< array_t >(
			reinterpret_cast< uintptr_t >( this ) + offsets::game_instance::local_players );
	}

	c_local_player* get_first_local_player( ) const noexcept
	{
		array_t players = get_local_players( );
		if ( !players.data || players.count <= 0 )
			return nullptr;
		return g_memory->read< c_local_player* >( players.data );
	}
};
