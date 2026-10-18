#pragma once
#include "../math/math.h"

struct camera_info_t
{
	vec3_t location;
	vec3_t rotation;
	float fov;
};

namespace render
{
	void set_screen_size( int width, int height ) noexcept;
	int get_screen_width( ) noexcept;
	int get_screen_height( ) noexcept;

	struct camera_basis_t
	{
		vec3_t location{ };
		vec3_t forward{ };
		vec3_t right{ };
		vec3_t up{ };
		double focal     = 0.0;
		double center_x  = 0.0;
		double center_y  = 0.0;
		double near_plane = 0.098;
		double far_plane  = 0.0;
	};

	camera_basis_t make_basis( const camera_info_t& cam_info, double far_distance ) noexcept;

	bool world_to_screen( const camera_basis_t& basis, const vec3_t& world_pos, vec2_t& screen_out ) noexcept;
	bool world_to_screen( const camera_info_t& cam_info, const vec3_t& world_pos, vec2_t& screen_out ) noexcept;

	bool is_sphere_visible( const camera_basis_t& basis, const vec3_t& center, double radius ) noexcept;
}
