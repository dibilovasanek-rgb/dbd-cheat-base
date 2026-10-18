#pragma once
#include <cstdint>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../c_game_state/c_game_state.h"
#include "../c_game_instance/c_game_instance.h"

class c_world
{
public:
	static c_world* get( ) noexcept
	{
		static uintptr_t base = 0;
		if ( !base )
			base = g_memory->get_base_address( );

		return g_memory->read< c_world* >( base + offsets::g_world );
	}

	c_game_state* get_game_state( ) const noexcept
	{
		return g_memory->read< c_game_state* >( get_base( ) + offsets::world::game_state );
	}

	c_game_instance* get_game_instance( ) const noexcept
	{
		return g_memory->read< c_game_instance* >( get_base( ) + offsets::world::game_instance );
	}

private:
	uintptr_t get_base( ) const noexcept
	{
		return reinterpret_cast< uintptr_t >( this );
	}
};
