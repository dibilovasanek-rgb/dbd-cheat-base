#pragma once
#include "../../core/settings/settings.h"

#include "../../../thirdparty/imgui/imgui.h"

class c_render
{
public:
	static c_render* get( ) noexcept
	{
		static c_render instance;
		return &instance;
	}

	void on_render_menu( bool& visible ) noexcept;

private:
	c_render( ) = default;
};

inline c_render* g_render = c_render::get( );
