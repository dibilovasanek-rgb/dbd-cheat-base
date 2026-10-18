#pragma once
#include <cstdint>
#include <vector>
#include "../../memory/memory.h"
#include "../../offsets/offsets.h"
#include "../../math/math.h"

struct transform_t
{
	double rot[ 4 ];
	vec3_t translation;
	double pad;
	vec3_t scale;
	double pad2;
};

class c_skeletal_mesh
{
public:
	transform_t get_component_to_world( ) const noexcept
	{
		return g_memory->read< transform_t >(
			reinterpret_cast< uintptr_t >( this ) + offsets::scene_component::component_to_world );
	}

	uint64_t get_bone_array_ptr( ) const noexcept
	{
		const uintptr_t base = reinterpret_cast< uintptr_t >( this );
		uint64_t arr = g_memory->read< uint64_t >( base + offsets::skeletal_mesh::bone_array );
		if ( !arr )
			arr = g_memory->read< uint64_t >( base + offsets::skeletal_mesh::bone_array_alt );
		return arr;
	}

	int32_t get_bone_count( ) const noexcept
	{
		const uintptr_t base = reinterpret_cast< uintptr_t >( this );
		int32_t count = g_memory->read< int32_t >( base + offsets::skeletal_mesh::bone_array + 0x8 );
		if ( count <= 0 )
			count = g_memory->read< int32_t >( base + offsets::skeletal_mesh::bone_array_alt + 0x8 );
		return count;
	}

	transform_t get_bone_transform( int index, uint64_t cached_array = 0 ) const noexcept
	{
		uint64_t arr = cached_array ? cached_array : get_bone_array_ptr( );
		return g_memory->read< transform_t >( arr + index * sizeof( transform_t ) );
	}

	std::vector< transform_t > get_bone_transforms( int32_t count ) const noexcept
	{
		std::vector< transform_t > out;
		uint64_t arr = get_bone_array_ptr( );
		if ( !arr || count <= 0 || count > 512 )
			return out;

		out.resize( static_cast< std::size_t >( count ) );
		g_memory->read_raw( arr, out.data( ), static_cast< std::size_t >( count ) * sizeof( transform_t ) );
		return out;
	}

	vec3_t get_bone_world_position( int index ) const noexcept
	{
		transform_t c2w = get_component_to_world( );
		transform_t bone = get_bone_transform( index );

		return bone_world_position( c2w, bone );
	}

	static vec3_t bone_world_position( const transform_t& c2w, const transform_t& bone ) noexcept
	{

		double cx = c2w.rot[ 0 ], cy = c2w.rot[ 1 ], cz = c2w.rot[ 2 ], cw = c2w.rot[ 3 ];

		double bx = bone.translation.x;
		double by = bone.translation.y;
		double bz = bone.translation.z;

		double rx = bx * ( 1.0 - 2.0 * ( cy * cy + cz * cz ) ) +
					by * 2.0 * ( cx * cy - cz * cw ) +
					bz * 2.0 * ( cx * cz + cy * cw );

		double ry = bx * 2.0 * ( cx * cy + cz * cw ) +
					by * ( 1.0 - 2.0 * ( cx * cx + cz * cz ) ) +
					bz * 2.0 * ( cy * cz - cx * cw );

		double rz = bx * 2.0 * ( cx * cz - cy * cw ) +
					by * 2.0 * ( cy * cz + cx * cw ) +
					bz * ( 1.0 - 2.0 * ( cx * cx + cy * cy ) );

		return vec3_t(
			rx * c2w.scale.x + c2w.translation.x,
			ry * c2w.scale.y + c2w.translation.y,
			rz * c2w.scale.z + c2w.translation.z );
	}
	uint32_t get_mesh_id( ) const noexcept
	{
		return g_memory->read< uint32_t >( reinterpret_cast< uintptr_t >( this ) + 0x18 );
	}
};
