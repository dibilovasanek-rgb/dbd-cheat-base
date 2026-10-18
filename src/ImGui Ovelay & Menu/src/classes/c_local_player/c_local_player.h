#pragma once
#include <cstdint>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../c_player_controller/c_player_controller.h"

class c_local_player
{
public:
	c_player_controller* get_player_controller( ) const noexcept
	{
		return g_memory->read< c_player_controller* >(
			reinterpret_cast< uintptr_t >( this ) + offsets::local_player::player_controller );
	}
};
