#pragma once

#include "imgui/imgui.h"

namespace UIDefines
{
	constexpr auto window_scaling = 1.5f;

	constexpr auto font_path = "fonts/ProggyVector-Regular.ttf";
	constexpr auto font_size = 17.f * window_scaling;

	const ImColor bg_color{ 97, 173, 255, int(25) };
};
