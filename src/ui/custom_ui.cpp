#include "custom_ui.hpp"
#include "imgui_internal.h"

using ImGui::BeginListBox;
using ImGui::EndListBox;
using ImGui::GetTextLineHeightWithSpacing;
using ImGui::MarkItemEdited;
using ImGui::PopID;
using ImGui::PushID;
using ImGui::Selectable;
using ImGui::SetItemDefaultFocus;

bool CustomListBox(const char *label, int *current_item, const char *const items[], int items_count, ImVec2 size)
{
    ImGuiContext &g = *GImGui;

    if (!BeginListBox(label, size))
        return false;

    // Assume all items have even height (= 1 line of text). If you need items of different height,
    // you can create a custom version of ListBox() in your code without using the clipper.
    bool value_changed = false;

    ImGuiListClipper clipper;
    clipper.Begin(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.

    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            PushID(i);
            const bool item_selected = (i == *current_item);
            if (Selectable(items[i], item_selected))
            {
                *current_item = i;
                value_changed = true;
            }
            if (item_selected)
                SetItemDefaultFocus();
            PopID();
        }
    EndListBox();

    if (value_changed)
        MarkItemEdited(g.CurrentWindow->DC.LastItemId);

    return value_changed;
}