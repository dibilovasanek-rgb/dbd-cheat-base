#pragma once
#include <iostream>
#include <memory>
#include "../../memory/memory.h"
#include "../../types/types.h"
#include "../../render/render.h"
#include "../../bones/bones.h"
#include "../../classes/c_world/c_world.h"
#include "../../classes/c_fname/c_fname.h"
#include "../../classes/c_player_state/c_player_state.h"
#include "../../classes/c_skeletal_mesh/c_skeletal_mesh.h"
#include "../settings/settings.h"
#include "../raycast/c_raycast.hpp"

#include "../../../thirdparty/imgui/imgui.h"

class c_visuals
{
public:
	void on_render(int screen_width, int screen_height) noexcept;
	void on_render(int screen_width, int screen_height, const camera_info_t& cam_info) noexcept;
};

inline bool get_camera_info( camera_info_t& out ) noexcept
{
	auto world = c_world::get( );
	if ( !world )
		return false;

	auto game_instance = world->get_game_instance( );
	if ( !game_instance )
		return false;

	auto local_player = game_instance->get_first_local_player( );
	if ( !local_player )
		return false;

	auto controller = local_player->get_player_controller( );
	if ( !controller )
		return false;

	auto camera_manager = controller->get_player_camera_manager( );
	if ( !camera_manager )
		return false;

	out = camera_manager->get_camera_cache_private( );
	return true;
}

inline auto g_visual = std::make_unique<c_visuals>();
