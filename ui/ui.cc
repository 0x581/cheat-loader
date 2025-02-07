#include "ui.hh"
#include "../globals.hh"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include <thread>

std::string key_input = "";

bool IsInternetConnected() {
    // its a ghetto method but idc tbh, also needs admin rights
    int result = system("ping -c 1 google.com");
    return result == 0;
}

void TextCentered(std::string text) {
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

    float horizontalIndentation = (windowSize.x - textSize.x) * 0.5f;
    float verticalIndentation = (windowSize.y - textSize.y) * 0.5f;

    float minHorizontalIndentation = 20.0f;
    if (horizontalIndentation <= minHorizontalIndentation) {
        horizontalIndentation = minHorizontalIndentation;
    }

    float minVerticalIndentation = 20.0f;
    if (verticalIndentation <= minVerticalIndentation) {
        verticalIndentation = minVerticalIndentation;
    }

    ImGui::SetCursorPosX(horizontalIndentation);
    ImGui::SetCursorPosY(verticalIndentation);

    ImGui::PushTextWrapPos(windowSize.x - horizontalIndentation);
    ImGui::TextWrapped(text.c_str());
    ImGui::PopTextWrapPos();
}


int hello_world() {
    const char* command = "cmd /c @echo off && title https://discord.gg/96Xpa8XX4t && color b && echo Hello World! && pause >NUL";
    int result = system(command);
    return true;
}

void ui::render() {
    if (!globals.active) return;

#if _DEBUG
    globals.checked_connection = true;
    globals.connection_message = ("Connected");
#endif

    ImGui::SetNextWindowPos(ImVec2(window_pos.x, window_pos.y), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(window_size.x, window_size.y));
    ImGui::SetNextWindowBgAlpha(1.0f);

    ImGui::Begin(window_title, &globals.active, window_flags);
    {
        ImGuiStyle& style = ImGui::GetStyle();

        if (!globals.checked_connection)
        {
            if (IsInternetConnected()) {
                globals.connection_message = ("Connected");
            }
            else {
                globals.connection_message = ("Failed to check connection!");
                globals.fail_message = "Failed to check connection!";
                globals.failed = true;
            }
            globals.checked_connection = true;
        }

        if (globals.failed)
        {
            TextCentered(globals.fail_message.c_str());
            
            globals.tick_count++;

            if (globals.tick_count > 500)
                exit(1337);
        }

        if (globals.login_form && !globals.failed)
        {
            int status_width = ImGui::CalcTextSize("KEY").x * 0.5;
            ImGui::SetCursorPos(ImVec2(200 - status_width, 70));
            ImGui::Text("KEY");

            
            ImGui::SetCursorPos(ImVec2(60, 110));
            ImGui::InputText("", globals.key, IM_ARRAYSIZE(globals.key));

            key_input = globals.key;

            ImGui::SetCursorPos(ImVec2(60, 150));
            if (ImGui::Button("LOGIN", ImVec2(260, 20))) {
                if (key_input == "key")
                {
                    globals.login_form = false;
                    globals.login_loading = true;
                }
                // just for example
                else if (key_input == "hwid")
                {
                    globals.fail_message = "Invalid HWID. Please contact support.";
                    globals.failed = true;
                }
                else
                {
                    globals.fail_message = "Wrong key";
                    globals.failed = true;
                }
            }
        }
        else if (globals.login_loading && !globals.failed)
        {
            static float 
                progress = 0.0f, 
                progress_dir = 1.0f, 
                limitter = 1.0f;

            if (progress < limitter)
                progress += progress_dir * 3.5f * ImGui::GetIO().DeltaTime;
            else
            {
                globals.main_form = true;
                globals.login_loading = false;
            }

            ImGui::SetCursorPos(ImVec2(7, 130));

            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            ImGui::ProgressBar(progress, ImVec2(385.0f, 20.0f));
        }
        else if (globals.main_form && !globals.failed)
        {
            auto p = ImGui::GetWindowPos();
            ImGuiContext& g = *GImGui;
            ImFont* font = g.Font;
            // Convert to std::string and concatenate
            
            window_title = "https://discord.gg/96Xpa8XX4t | premium panel";

            //ImGui::SetCursorPos(ImVec2(10, 25));
            //ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 15, p.y + 35), ImVec2(p.x + 205, p.y + 180), ImColor(0, 0, 0, int(55)), 0);
            //ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x + 15, p.y + 35), ImVec2(p.x + 205, p.y + 180), ImColor(97, 173, 255, int(25)), 0);

            // Products tab
            const char* items[] = { "Hello World!", "product 2", "product 3", "product 4", "product 5" };
            static int item_current = 0;
            ImGui::SetCursorPos(ImVec2(15, 30));
            ImGui::Text("Products");
            ImGui::SetCursorPos(ImVec2(15, 50));
            ImGui::ListBox("", &item_current, items, IM_ARRAYSIZE(items), 4);
            ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x + 15, p.y + 50), ImVec2(p.x + 185, p.y + 145), ImColor(97, 173, 255, int(25)), 0);

            // Load tab
            ImGui::SetCursorPos(ImVec2(200, 30));
            ImGui::Text("Load");
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 200, p.y + 50), ImVec2(p.x + 385, p.y + 145), ImColor(19, 19, 19, int(255)), 0);
            ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x + 200, p.y + 50), ImVec2(p.x + 385, p.y + 145), ImColor(97, 173, 255, int(25)), 0);
            std::string current_product = "";
            const char* selectedText = items[item_current];
            current_product = selectedText;
            ImGui::SetCursorPos(ImVec2(210, 60));
            ImGui::Text(current_product.c_str());
            ImGui::SetCursorPos(ImVec2(210, 80));
            if (ImGui::Button("LOAD", ImVec2(165, 20))) {
                globals.final_selected_product = item_current;
                globals.loading_product = true;
                globals.main_form = false;
            }
            ImGui::SetCursorPos(ImVec2(210, 110));
            if (ImGui::Button("EXIT", ImVec2(165, 20))) {
                exit(1337);
            }        

            // Information tab
            ImGui::SetCursorPos(ImVec2(15, 160));
            ImGui::Text("Information");
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 15, p.y + 35 + 35 + 180 + 30), ImVec2(p.x + 385, p.y + 180), ImColor(19, 19, 19, int(255)), 0);
            ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x + 15, p.y + 35 + 35 + 180 + 30), ImVec2(p.x + 385, p.y + 180), ImColor(97, 173, 255, int(25)), 0);
            ImGui::SetCursorPos(ImVec2(25, 190));
            ImGui::Text("Status: ");
            int status_width = ImGui::CalcTextSize("Status: ").x;
            ImColor status_color;
            // ofc u should initialize strings as globals.version / globals.last_uptade / globals.status in loading screen or somewhere else... this is a ghetto example
            if (item_current == 0)
            {
                status_color = ImColor(89, 255, 0, int(255));
                globals.status = "undetected"; // ofc ;)
                globals.last_uptade = "29.09.2023";
                globals.version = "3.3.0";
            }
            else if (item_current == 1)
            {
                status_color = ImColor(201, 204, 0, int(255));
                globals.status = "testing";
                globals.last_uptade = "12.09.2023";
                globals.version = "0.9.7";
            }
            else if (item_current == 2)
            {
                status_color = ImColor(200, 0, 0, int(255));
                globals.status = "detected";
                globals.last_uptade = "15.09.2023";
                globals.version = "0.2.7";
            }
            else // and so on...
            {
                globals.status = "unknown";
                globals.last_uptade = "unknown";
                globals.version = "unknown";
                status_color = ImColor(252, 186, 3, int(255));
            }
            ImGui::GetWindowDrawList()->AddText(font, 13, ImVec2(p.x + 25 + status_width, p.y + 190), status_color, globals.status.c_str());
            ImGui::SetCursorPos(ImVec2(25, 210));
           

            ImGui::Text("Last Update: ");
            int last_update_width = ImGui::CalcTextSize("Last Update: ").x;
            ImGui::GetWindowDrawList()->AddText(font, 13, ImVec2(p.x + 25 + last_update_width, p.y + 210), ImColor(99, 182, 255, int(255)), globals.last_uptade.c_str());
            ImGui::SetCursorPos(ImVec2(25, 230));
            ImGui::Text("Version: ");
            int version_width = ImGui::CalcTextSize("Version: ").x;
            ImGui::GetWindowDrawList()->AddText(font, 13, ImVec2(p.x + 25 + version_width, p.y + 230), ImColor(99, 182, 255, int(255)), globals.version.c_str());
            ImGui::SetCursorPos(ImVec2(25, 250));
            ImGui::Text("Connection: ");
            int connection_width = ImGui::CalcTextSize("Connection: ").x;
            ImGui::GetWindowDrawList()->AddText(font, 13, ImVec2(p.x + 25 + connection_width, p.y + 250), ImColor(89, 255, 0, int(255)), globals.connection_message.c_str());
        }
        else if (globals.loading_product && !globals.failed)
        {
            static float
                progress = 0.0f,
                progress_dir = 1.0f,
                limitter = 1.0f;

            if (progress < limitter)
            {
                ImGui::SetCursorPos(ImVec2(7, 130));
                style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
                ImGui::ProgressBar(progress, ImVec2(385.0f, 20.0f));

                if (progress < 0.25 && globals.final_selected_product != 0)
                {
                    progress += progress_dir * 0.5f * ImGui::GetIO().DeltaTime;
                    globals.fail_message = "Product is currently under maintenance";
                    globals.tick_count2++;

                    if (globals.tick_count2 > 50)
                        globals.failed = true;
                }
                else
                    progress += progress_dir * 0.5f * ImGui::GetIO().DeltaTime;
            }      
            else
            {
                if (globals.final_selected_product == 0)
                {
                    if (!globals.loaded_product)
                    {
                        std::thread t1(hello_world);
                        t1.detach();
                    }

                    globals.loaded_product = true;
                    TextCentered("Loaded!");
                    globals.tick_count++;

                    if (globals.tick_count > 500)
                        exit(1337);
                }
            }

        }
    }
    ImGui::End();
}

void ui::init(LPDIRECT3DDEVICE9 device) {
    dev = device;
	
    // colors
    ImGui::StyleColorsDark();
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

	if (window_pos.x == 0) {
		RECT screen_rect{};
		GetWindowRect(GetDesktopWindow(), &screen_rect);
		screen_res = ImVec2(float(screen_rect.right), float(screen_rect.bottom));
		window_pos = (screen_res - window_size) * 0.5f;
	}
}