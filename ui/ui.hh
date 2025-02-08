#pragma once
#include <d3d9.h>
#include "../imgui/imgui.h"
#include "../keyauth/auth.hpp"


namespace ui {
	void init(LPDIRECT3DDEVICE9);
	void renderLogin(KeyAuth::api&);
	void renderMain(KeyAuth::api&);
	void renderLoadingProduct(KeyAuth::api&);
	void renderError(KeyAuth::api&);
	void render(KeyAuth::api&);
}

namespace ui {
	inline LPDIRECT3DDEVICE9 dev;
	inline const char* window_title = "https://discord.gg/gYAV66mbsr";
}

namespace ui {
	inline ImVec2 screen_res{ 000, 000 };
	inline ImVec2 window_pos{ 0, 0 };
	inline ImVec2 window_size{ 400, 300 };
	inline DWORD  window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
}