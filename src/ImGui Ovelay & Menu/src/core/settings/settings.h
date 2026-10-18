#pragma once

struct settings_t
{
	bool box_esp = false;
	bool snaplines = false;
	bool skeleton = false;
	bool name_esp = false;
	bool health_esp = false;
	bool map_wireframe = false;
	bool visible_check = false;
};

inline settings_t* g_settings = new settings_t( );
