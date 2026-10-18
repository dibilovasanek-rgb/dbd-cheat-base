#pragma once
#include <cstdint>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../../types/types.h"

class c_game_state
{
public:
	array_t get_player_array( ) const noexcept
	{
		return g_memory->read< array_t >(
			reinterpret_cast< uintptr_t >( this ) + offsets::game_state::player_array );
	}
};
