#pragma once
#include <cstdint>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../c_pawn/c_pawn.h"
#include "../c_camera_manager/c_camera_manager.h"

class c_player_controller
{
public:
	c_pawn* get_acknowledged_pawn( ) const noexcept
	{
		return g_memory->read< c_pawn* >(
			reinterpret_cast< uintptr_t >( this ) + offsets::player_controller::acknowledged_pawn );
	}

	c_camera_manager* get_player_camera_manager( ) const noexcept
	{
		return g_memory->read< c_camera_manager* >(
			reinterpret_cast< uintptr_t >( this ) + offsets::player_controller::player_camera_manager );
	}
};
