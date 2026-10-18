#pragma once
#include <cstdint>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../../math/math.h"

class c_scene_component
{
public:
	vec3_t get_relative_location( ) const noexcept
	{
		return g_memory->read< vec3_t >( reinterpret_cast< uintptr_t >( this ) + offsets::scene_component::relative_location );
	}
};
