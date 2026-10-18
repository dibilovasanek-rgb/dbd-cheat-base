#pragma once
#include "../memory/memory.h"
#include "../math/math.h"
#include <cstdio>
#include <cmath>

namespace bruteforce
{
	struct transform_t
	{
		double rot[ 4 ];
		vec3_t translation;
		double pad;
		vec3_t scale;
		double pad2;
	};

	inline bool is_valid_pointer( uint64_t p ) noexcept
	{
		return p > 0x10000 && p < 0x7FFFFFFFFFFF;
	}

	inline bool is_valid_world_position( const vec3_t& v ) noexcept
	{
		if ( std::isnan( v.x ) || std::isnan( v.y ) || std::isnan( v.z ) )
			return false;
		if ( std::isinf( v.x ) || std::isinf( v.y ) || std::isinf( v.z ) )
			return false;
		if ( v.x == 0.0 && v.y == 0.0 && v.z == 0.0 )
			return false;
		if ( std::abs( v.x ) > 5000000.0 || std::abs( v.y ) > 5000000.0 || std::abs( v.z ) > 5000000.0 )
			return false;
		return true;
	}

	inline bool is_valid_transform( const transform_t& t ) noexcept
	{
		if ( !is_valid_world_position( t.translation ) )
			return false;

		double quat_len = t.rot[ 0 ] * t.rot[ 0 ] + t.rot[ 1 ] * t.rot[ 1 ] +
						  t.rot[ 2 ] * t.rot[ 2 ] + t.rot[ 3 ] * t.rot[ 3 ];
		if ( quat_len < 0.8 || quat_len > 1.2 )
			return false;

		return true;
	}

	inline uint64_t find_component_to_world( uint64_t mesh, const vec3_t& expected_pos, float tolerance = 1000.f ) noexcept
	{
		printf( "[bruteforce] searching ComponentToWorld in mesh 0x%llX\n", mesh );
		printf( "[bruteforce] expected pos: %.1f %.1f %.1f (tolerance=%.1f)\n",
			expected_pos.x, expected_pos.y, expected_pos.z, tolerance );

		for ( uint64_t offset = 0x80; offset < 0x800; offset += 0x8 )
		{
			transform_t transform = g_memory->read< transform_t >( mesh + offset );

			if ( !is_valid_transform( transform ) )
				continue;

			double dist = std::sqrt(
				( transform.translation.x - expected_pos.x ) * ( transform.translation.x - expected_pos.x ) +
				( transform.translation.y - expected_pos.y ) * ( transform.translation.y - expected_pos.y ) +
				( transform.translation.z - expected_pos.z ) * ( transform.translation.z - expected_pos.z ) );

			if ( dist < tolerance )
			{
				printf( "[bruteforce] FOUND ComponentToWorld at offset 0x%llX (dist=%.1f)\n", offset, dist );
				printf( "[bruteforce]   translation: %.1f %.1f %.1f\n",
					transform.translation.x, transform.translation.y, transform.translation.z );
				printf( "[bruteforce]   quat: %.4f %.4f %.4f %.4f\n",
					transform.rot[ 0 ], transform.rot[ 1 ], transform.rot[ 2 ], transform.rot[ 3 ] );
				printf( "[bruteforce]   scale: %.2f %.2f %.2f\n",
					transform.scale.x, transform.scale.y, transform.scale.z );
			}
		}

		printf( "[bruteforce] ComponentToWorld scan done\n" );
		return 0;
	}

	inline bool is_valid_bone_transform( const transform_t& t ) noexcept
	{
		if ( std::isnan( t.translation.x ) || std::isnan( t.translation.y ) || std::isnan( t.translation.z ) )
			return false;
		if ( std::isinf( t.translation.x ) || std::isinf( t.translation.y ) || std::isinf( t.translation.z ) )
			return false;
		if ( std::abs( t.translation.x ) > 500000.0 || std::abs( t.translation.y ) > 500000.0 ||
			 std::abs( t.translation.z ) > 500000.0 )
			return false;

		double quat_len = t.rot[ 0 ] * t.rot[ 0 ] + t.rot[ 1 ] * t.rot[ 1 ] +
						  t.rot[ 2 ] * t.rot[ 2 ] + t.rot[ 3 ] * t.rot[ 3 ];
		if ( quat_len < 0.5 || quat_len > 1.5 )
			return false;

		return true;
	}

	inline uint64_t find_bone_array( uint64_t mesh, const vec3_t& component_pos, float tolerance = 2000.f ) noexcept
	{
		printf( "[bruteforce] searching BoneArray in mesh 0x%llX (range 0x0..0x1800)\n", mesh );
		printf( "[bruteforce] ComponentToWorld translation: %.1f %.1f %.1f\n",
			component_pos.x, component_pos.y, component_pos.z );

		int candidates = 0;

		for ( uint64_t offset = 0x0; offset < 0x1800; offset += 0x8 )
		{
			uint64_t potential_array = g_memory->read< uint64_t >( mesh + offset );

			if ( !is_valid_pointer( potential_array ) )
				continue;

			for ( int count_offset = 8; count_offset <= 12; count_offset += 4 )
			{
				int32_t bone_count = g_memory->read< int32_t >( mesh + offset + count_offset );
				if ( bone_count < 2 || bone_count > 500 )
					continue;

				transform_t bone0 = g_memory->read< transform_t >( potential_array );
				if ( !is_valid_bone_transform( bone0 ) )
					continue;

				transform_t bone1 = g_memory->read< transform_t >( potential_array + sizeof( transform_t ) );
				if ( !is_valid_bone_transform( bone1 ) )
					continue;

				transform_t bone5{ };
				if ( bone_count > 5 )
					bone5 = g_memory->read< transform_t >( potential_array + sizeof( transform_t ) * 5 );

				printf( "[bruteforce] CANDIDATE at mesh+0x%llX (ptr=0x%llX, count=%d, countOfs=+%d)\n",
					offset, potential_array, bone_count, count_offset );
				printf( "[bruteforce]   bone[0]: pos(%.1f, %.1f, %.1f) q(%.3f,%.3f,%.3f,%.3f)\n",
					bone0.translation.x, bone0.translation.y, bone0.translation.z,
					bone0.rot[ 0 ], bone0.rot[ 1 ], bone0.rot[ 2 ], bone0.rot[ 3 ] );
				printf( "[bruteforce]   bone[1]: pos(%.1f, %.1f, %.1f) q(%.3f,%.3f,%.3f,%.3f)\n",
					bone1.translation.x, bone1.translation.y, bone1.translation.z,
					bone1.rot[ 0 ], bone1.rot[ 1 ], bone1.rot[ 2 ], bone1.rot[ 3 ] );
				if ( bone_count > 5 && is_valid_bone_transform( bone5 ) )
					printf( "[bruteforce]   bone[5]: pos(%.1f, %.1f, %.1f)\n",
						bone5.translation.x, bone5.translation.y, bone5.translation.z );

				candidates++;
				if ( candidates >= 10 )
				{
					printf( "[bruteforce] ... (max candidates reached)\n" );
					goto done;
				}
			}
		}
	done:
		printf( "[bruteforce] BoneArray scan done (%d candidates)\n", candidates );
		return 0;
	}

	inline void run_all( uint64_t pawn, const vec3_t& actor_pos ) noexcept
	{
		printf( "\n========= BRUTEFORCE START =========\n" );
		printf( "pawn = 0x%llX\n", pawn );

		constexpr uint64_t mesh_offset = 0x0370;

		uint64_t mesh = g_memory->read< uint64_t >( pawn + mesh_offset );
		printf( "mesh (pawn+0x%llX) = 0x%llX\n", mesh_offset, mesh );

		if ( !is_valid_pointer( mesh ) )
		{
			printf( "mesh pointer invalid!\n" );
			printf( "========= BRUTEFORCE END (FAILED) =========\n\n" );
			return;
		}

		find_component_to_world( mesh, actor_pos );
		find_bone_array( mesh, actor_pos );

		printf( "========= BRUTEFORCE END =========\n\n" );
	}
}
