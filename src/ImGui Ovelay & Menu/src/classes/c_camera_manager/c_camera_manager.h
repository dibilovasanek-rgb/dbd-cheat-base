#pragma once
#include <cstdint>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../../render/render.h"

class c_camera_manager
{
public:
	camera_info_t get_camera_cache_private( ) const noexcept
	{
		return g_memory->read< camera_info_t >(
			reinterpret_cast< uintptr_t >( this ) + offsets::camera_manager::camera_cache_private );
	}
};
