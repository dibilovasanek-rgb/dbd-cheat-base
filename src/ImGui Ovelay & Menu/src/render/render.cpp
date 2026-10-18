#include "render.h"

#include <cmath>

namespace render
{
	static int g_screen_width = 0;
	static int g_screen_height = 0;

	void set_screen_size( int width, int height ) noexcept
	{
		g_screen_width = width;
		g_screen_height = height;
	}

	int get_screen_width( ) noexcept
	{
		return g_screen_width;
	}

	int get_screen_height( ) noexcept
	{
		return g_screen_height;
	}

	camera_basis_t make_basis( const camera_info_t& cam_info, double far_distance ) noexcept
	{
		const matrix4x4_t mat = matrix_from_rotation( cam_info.rotation );

		camera_basis_t basis;
		basis.location = cam_info.location;

		basis.forward = vec3_t( mat.m[ 0 ][ 0 ], mat.m[ 0 ][ 1 ], mat.m[ 0 ][ 2 ] );
		basis.right   = vec3_t( mat.m[ 1 ][ 0 ], mat.m[ 1 ][ 1 ], mat.m[ 1 ][ 2 ] );
		basis.up      = vec3_t( mat.m[ 2 ][ 0 ], mat.m[ 2 ][ 1 ], mat.m[ 2 ][ 2 ] );

		basis.center_x = g_screen_width / 2.0;
		basis.center_y = g_screen_height / 2.0;
		basis.focal = basis.center_x / std::tan( cam_info.fov * M_PI / 360.0 );
		basis.far_plane = far_distance;

		return basis;
	}

	bool world_to_screen( const camera_basis_t& basis, const vec3_t& world_pos, vec2_t& screen_out ) noexcept
	{
		const vec3_t delta = world_pos - basis.location;

		const double depth = delta.dot( basis.forward );
		if ( depth < basis.near_plane )
			return false;

		screen_out.x = basis.center_x + delta.dot( basis.right ) * basis.focal / depth;
		screen_out.y = basis.center_y - delta.dot( basis.up ) * basis.focal / depth;

		return true;
	}

	bool world_to_screen( const camera_info_t& cam_info, const vec3_t& world_pos, vec2_t& screen_out ) noexcept
	{
		return world_to_screen( make_basis( cam_info, 1e9 ), world_pos, screen_out );
	}

	bool is_sphere_visible( const camera_basis_t& basis, const vec3_t& center, double radius ) noexcept
	{
		const vec3_t delta = center - basis.location;

		const double depth = delta.dot( basis.forward );
		if ( depth + radius < basis.near_plane )
			return false;
		if ( depth - radius > basis.far_plane )
			return false;

		if ( depth > basis.near_plane )
		{
			const double half_width = basis.center_x * depth / basis.focal;
			const double half_height = basis.center_y * depth / basis.focal;

			if ( std::abs( delta.dot( basis.right ) ) - radius > half_width )
				return false;
			if ( std::abs( delta.dot( basis.up ) ) - radius > half_height )
				return false;
		}

		return true;
	}
}
