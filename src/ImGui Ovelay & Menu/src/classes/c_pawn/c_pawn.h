#pragma once
#include <cstdint>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../c_actor/c_actor.h"

class c_pawn : public c_actor
{
public:
	uint64_t get_mesh( ) const noexcept
	{
		return g_memory->read< uint64_t >( get_base( ) + offsets::character::mesh );
	}

	uint64_t get_health_component( ) const noexcept
	{
		return g_memory->read< uint64_t >( get_base( ) + offsets::dbd::health_component );
	}

	int32_t get_current_health( ) const noexcept
	{
		const uint64_t health_component = get_health_component( );
		if ( !health_component )
			return 0;

		return g_memory->read< int32_t >( health_component + offsets::dbd::health_state_count );
	}

	uint64_t get_base( ) const noexcept
	{
		return reinterpret_cast< uintptr_t >( this );
	}
};
