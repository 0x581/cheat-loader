#pragma once

#include "../imgui/imgui.h"

bool CustomListBox(const char* label, int* current_item, const char* const items[], int items_count, ImVec2 size);
