#include "c_visuals.hpp"
void c_visuals::on_render(int screen_width, int screen_height) noexcept
{
	camera_info_t cam_info{ };
	if (!get_camera_info(cam_info))
		return;

	on_render(screen_width, screen_height, cam_info);
}

void c_visuals::on_render(int screen_width, int screen_height, const camera_info_t& cam_info) noexcept
{
	auto world = c_world::get();
	if (!world)
		return;

	auto game_state = world->get_game_state();
	if (!game_state)
		return;

	auto game_instance = world->get_game_instance();
	if (!game_instance)
		return;

	auto local_player = game_instance->get_first_local_player();
	if (!local_player)
		return;

	auto controller = local_player->get_player_controller();
	if (!controller)
		return;

	auto local_pawn = controller->get_acknowledged_pawn();

	array_t players = game_state->get_player_array();
	if (players.count <= 0 || players.count > 100 || !players.data)
		return;

	for (int i = 0; i < players.count; ++i)
	{
		auto player_state = g_memory->read< c_player_state* >(players.data + i * 8);
		if (!player_state)
			continue;

		auto pawn = player_state->get_pawn();
		if (!pawn || pawn == local_pawn)
			continue;

		auto name = fname::get_name_by_id(pawn->get_name_id());

		const char* display_name = "Unknown";
		ImColor col_box = ImColor(1.0f, 0.0f, 0.0f);
		if (name.find("BP_CamperMale") != std::string::npos)
		{
			display_name = "MaleSurvivor";
			col_box = ImColor(0.0f, 1.0f, 0.0f);
		}
		else if (name.find("BP_CamperFemale") != std::string::npos)
		{
			display_name = "FemaleSurvivor";
			col_box = ImColor(0.0f, 1.0f, 0.0f);
		}
		else if (name.find("BP_Slasher") != std::string::npos)
		{
			display_name = "Killer";
			col_box = ImColor(1.0f, 0.0f, 0.0f);
		}


		auto root_comp = pawn->get_root_component();
		if (!root_comp)
			continue;

		vec3_t origin_pos = root_comp->get_relative_location();
		vec3_t feet_pos = vec3_t(origin_pos.x, origin_pos.y, origin_pos.z - 90.0);
		vec3_t head_pos = vec3_t(origin_pos.x, origin_pos.y, origin_pos.z + 90.0);

		vec2_t screen_feet, screen_head;

		bool feet_valid = render::world_to_screen(cam_info, feet_pos, screen_feet);
		bool head_valid = render::world_to_screen(cam_info, head_pos, screen_head);

		if (!feet_valid || !head_valid)
			continue;

		bool is_visible = true;
		if (g_settings->visible_check)
		{
			is_visible =
				g_raycast->is_visible(cam_info.location, head_pos) ||
				g_raycast->is_visible(cam_info.location, origin_pos) ||
				g_raycast->is_visible(cam_info.location, feet_pos);

			if (!is_visible)
				col_box = ImColor(0.45f, 0.45f, 0.45f);
		}

		float box_height = static_cast<float>(screen_feet.y - screen_head.y);
		float box_width = box_height * 0.5f;

		float center_x = static_cast<float>(screen_feet.x + screen_head.x) / 2.f;

		float top_left_x = center_x - box_width / 2.f;
		float top_left_y = static_cast<float>(screen_head.y);
		float bot_right_x = center_x + box_width / 2.f;
		float bot_right_y = static_cast<float>(screen_feet.y);

		if (g_settings->box_esp)
		{
			ImGui::GetBackgroundDrawList()->AddRect(
				ImVec2(top_left_x, top_left_y),
				ImVec2(bot_right_x, bot_right_y),
				col_box,
				0.f, 0, 1.5f);
		}

		if (g_settings->health_esp)
		{
			int32_t current_health = pawn->get_current_health();
			char health_buf[32];
			sprintf_s(health_buf, "HP: %d", current_health);
			ImVec2 health_text_size = ImGui::CalcTextSize(health_buf);
			ImGui::GetBackgroundDrawList()->AddText(
				ImVec2(center_x - health_text_size.x / 2.f, bot_right_y + 2.f),
				ImColor(1.0f, 1.0f, 0.0f),
				health_buf);
		}

		if (g_settings->name_esp)
		{
			ImVec2 text_size = ImGui::CalcTextSize(display_name);
			ImGui::GetBackgroundDrawList()->AddText(
				ImVec2(center_x - text_size.x / 2.f, top_left_y - text_size.y - 2.f),
				g_settings->visible_check && !is_visible
					? ImColor(0.45f, 0.45f, 0.45f)
					: ImColor(1.0f, 1.0f, 1.0f),
				display_name);
		}

		if (g_settings->snaplines)
		{
			ImGui::GetBackgroundDrawList()->AddLine(
				ImVec2(static_cast<float>(screen_width) / 2.f, static_cast<float>(screen_height)),
				ImVec2(center_x, bot_right_y),
				g_settings->visible_check && !is_visible
					? ImColor(0.45f, 0.45f, 0.45f)
					: ImColor(0.0f, 1.0f, 0.0f));
		}

		if (g_settings->skeleton)
		{
			
		}
	}
}