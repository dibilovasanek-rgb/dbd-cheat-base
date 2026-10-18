#include "c_gui.hpp"

void c_render::on_render_menu( bool& visible ) noexcept
{
	ImGui::SetNextWindowSize( { 300, 300 }, ImGuiCond_FirstUseEver );
	ImGui::Begin( "dbd", &visible, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar );

	ImGui::Text( "ESP" );
	ImGui::Separator( );
	ImGui::Checkbox( "Box", &g_settings->box_esp );
	ImGui::Checkbox( "Snaplines", &g_settings->snaplines );
	ImGui::Checkbox( "Skeleton", &g_settings->skeleton );
	ImGui::Checkbox( "Name", &g_settings->name_esp );
	ImGui::Checkbox( "Health", &g_settings->health_esp );

	ImGui::Spacing( );
	ImGui::Text( "World" );
	ImGui::Separator( );
	ImGui::Checkbox( "Map Wireframe", &g_settings->map_wireframe );
	ImGui::Checkbox( "Visible Check", &g_settings->visible_check );
	if ( ImGui::IsItemHovered( ) )
	{
		ImGui::SetTooltip( "Traces the ESP against the cached map geometry.\n"
			"Occluded players are drawn grey.\n"
			"Keeps the geometry cache running - costs CPU." );
	}

	ImGui::End( );
}
