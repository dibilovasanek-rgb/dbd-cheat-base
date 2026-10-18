#pragma once
#include <cstdint>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../c_scene_component/c_scene_component.h"

class c_actor
{
public:
	c_scene_component* get_root_component( ) const noexcept
	{
		return g_memory->read< c_scene_component* >( reinterpret_cast< uintptr_t >( this ) + offsets::actor::root_component );
	}

	uint32_t get_name_id( ) const noexcept
	{
		return g_memory->read< uint32_t >( reinterpret_cast< uintptr_t >( this ) + offsets::object::name_id );
	}
};
