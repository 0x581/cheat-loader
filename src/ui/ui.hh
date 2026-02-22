#pragma once
#include <d3d9.h>
#include "imgui.h"
#include "auth.hpp"

#include "defines.hpp"

namespace ui
{
	void init(LPDIRECT3DDEVICE9);
	void renderLogin(KeyAuth::api &);
	void renderMain(KeyAuth::api &);
	void renderLoadingProduct(KeyAuth::api &);
	void renderError(KeyAuth::api &);
	void render(KeyAuth::api &);
}

namespace ui
{
	inline LPDIRECT3DDEVICE9 dev;
	inline const char *window_title = "Merkur loader";
}

namespace ui
{
	inline ImVec2 screen_res{000, 000};
	inline ImVec2 window_pos{0, 0};
	inline ImVec2 window_size{480 * UIDefines::window_scaling, 360 * UIDefines::window_scaling};
	inline DWORD window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
}