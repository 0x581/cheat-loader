#include "ui.hh"

#include <thread>

#include "../imgui/imgui_internal.h"

#include "../globals.hh"
#include "../keyauth/utils.hpp"


static void TextCentered(const char* text, ImVec2 position)
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 textSize = ImGui::CalcTextSize(text);

    float x_pos = position.x - textSize.x * 0.5f;
    float y_pos = position.y - textSize.y * 0.5f;

    float min_x_pos = 20.0f;
    if (x_pos <= min_x_pos) {
        x_pos = min_x_pos;
    }

    float min_y_pos = 20.0f;
    if (y_pos <= min_y_pos) {
        y_pos = min_y_pos;
    }

    ImGui::SetCursorPos(ImVec2(x_pos, y_pos));

    ImGui::PushTextWrapPos(windowSize.x - x_pos);
    ImGui::TextWrapped(text);
    ImGui::PopTextWrapPos();
}


static void TextCentered(const std::string text, ImVec2 position)
{
    TextCentered(text.c_str(), position);
}


void ui::renderLogin(KeyAuth::api& KeyAuthApp)
{
    TextCentered(skCrypt("KEY").decrypt(), ImVec2(window_size.x * 0.5f, window_size.y * 0.25f));

    ImGui::PushItemWidth(window_size.x * 0.7f);
    {
        ImGui::SetCursorPos(ImVec2(window_size.x * 0.15f, window_size.y * 0.366666f));
        ImGui::InputText("", globals.key, IM_ARRAYSIZE(globals.key));
    }
    ImGui::PopItemWidth();

    const auto input_height = UIDefines::font_size + ImGui::GetStyle().FramePadding.y * 2.0f;

    static std::atomic<bool> license_done = false;

    ImGui::SetCursorPos(ImVec2(window_size.x * 0.15f, window_size.y * 0.5f));
    if (ImGui::Button(skCrypt("LOGIN"), ImVec2(window_size.x * 0.7f, input_height))) {
        static std::thread license_thread{
            [&]() {
                KeyAuthApp.license(globals.key);
                license_done = true;
            }
        };
    }

    if (!license_done)
        return;

    if (KeyAuthApp.response.success) {
        globals.form_state = main;
    }
    else
    {
        globals.error_message = KeyAuthApp.response.message;
        globals.form_state = failed;
    }
}


void ui::renderMain(KeyAuth::api& KeyAuthApp)
{
    static bool check_started = true;
    if (!check_started)
    {
        /*
       * Do NOT remove this checkAuthenticated() function.
       * It protects you from cracking, it would be NOT be a good idea to remove it
       */
        std::thread run(checkAuthenticated, KeyAuthApp.ownerid);

        // do NOT remove checkAuthenticated(), it MUST stay for security reasons
        std::thread check(sessionStatus, std::ref(KeyAuthApp)); // do NOT remove this function either.

        check_started = true;
    }
    // Products tab

    globals.products[0] = ProductInfo{
        .valid = true,
        .name = "CounterStrike 2",
        .status = undetected,
        .version = "0.1.0",
        .last_update = "05.02.2025",
        .key = globals.key,
    };

    globals.products[1] = ProductInfo{
        .valid = true,
        .name = "Product placeholder 1",
        .status = detected,
        .version = "1.0.2",
        .last_update = "04.01.2024",
        .key = "No valid key provided",
    };

    globals.products[2] = ProductInfo{
        .valid = true,
        .name = "Product placeholder 2",
        .status = testing,
        .version = "1.2.1",
        .last_update = "05.07.2024",
        .key = "No valid key provided",
    };

    int product_count = 0;

    for (; product_count < 16 && globals.products[product_count].valid; ++product_count);

    char const** items = new char const* [product_count];
    
    for (int i = 0; i < product_count; i++)
    {
        items[i] = globals.products[i].name.c_str();
    }

    auto p = ImGui::GetWindowPos();
    ImGuiContext& g = *GImGui;
    ImFont* font = g.Font;

    auto& drawlist = *ImGui::GetWindowDrawList();

    //  Products tab
    ImGui::SetCursorPos(ImVec2(window_size.x * 0.0375f, window_size.y * 0.1f));
    ImGui::Text(skCrypt("Cheats"));

    ImGui::SetCursorPos(ImVec2(window_size.x * 0.0375f, window_size.y * 0.16666f));

    ImGui::ListBox("", &globals.selectedProduct, items, product_count, 4);

    ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x + 15, p.y + 50), ImVec2(p.x + 185, p.y + 145), ImColor(97, 173, 255, int(25)), 0);

    // Load tab
    ImGui::SetCursorPos(ImVec2(200, 30));
    ImGui::Text(skCrypt("Load"));

    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 200, p.y + 50), ImVec2(p.x + 385, p.y + 145), ImColor(19, 19, 19, int(255)), 0);
    ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x + 200, p.y + 50), ImVec2(p.x + 385, p.y + 145), ImColor(97, 173, 255, int(25)), 0);

    ProductInfo& selectedProduct = globals.products[globals.selectedProduct];

    ImGui::SetCursorPos(ImVec2(210, 60));
    ImGui::Text(selectedProduct.name.c_str());

    ImGui::SetCursorPos(ImVec2(210, 80));
    if (ImGui::Button(skCrypt("LOAD"), ImVec2(165, 20))) {
        globals.final_selected_product = globals.selectedProduct;
        globals.form_state = loading_product;
    }

    ImGui::SetCursorPos(ImVec2(210, 110));
    if (ImGui::Button(skCrypt("EXIT"), ImVec2(165, 20))) {
        exit(1337);
    }

    // Information tab
    ImGui::SetCursorPos(ImVec2(15, 160));
    ImGui::Text(skCrypt("Information"));

    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 15, p.y + 35 + 35 + 180 + 30), ImVec2(p.x + 385, p.y + 180), ImColor(19, 19, 19, int(255)), 0);
    ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x + 15, p.y + 35 + 35 + 180 + 30), ImVec2(p.x + 385, p.y + 180), ImColor(97, 173, 255, int(25)), 0);

    ImGui::SetCursorPos(ImVec2(25, 190));
    ImGui::Text("Status: ");
    const auto status_width = ImGui::CalcTextSize(skCrypt("Status: ")).x;

    std::string status_string = "";
    ImColor status_color{ 0 };

    switch (selectedProduct.status)
    {
    case detected:
        status_string = skCrypt("detected").decrypt();
        status_color = ImColor(200, 0, 0, int(255));    //red
        break;
    case undetected:
        status_string = skCrypt("undetected").decrypt();
        status_color = ImColor(89, 255, 0, int(255));   //green
        break;
    case testing:
        status_string = skCrypt("testing").decrypt();
        status_color = ImColor(201, 204, 0, int(255));  //yellow
        break;
    default:
        exit(6969);
    }


    ImGui::GetWindowDrawList()->AddText(font, 13, ImVec2(p.x + 25 + status_width, p.y + 190), status_color, status_string.c_str());
    ImGui::SetCursorPos(ImVec2(25, 210));


    ImGui::Text(skCrypt("Last Update: "));
    const auto last_update_width = ImGui::CalcTextSize(skCrypt("Last Update: ")).x;
    ImGui::GetWindowDrawList()->AddText(font, 13, ImVec2(p.x + 25 + last_update_width, p.y + 210), ImColor(99, 182, 255, int(255)), selectedProduct.last_update.c_str());
    ImGui::SetCursorPos(ImVec2(25, 230));
    ImGui::Text(skCrypt("Version: "));
    const auto version_width = ImGui::CalcTextSize(skCrypt("Version: ")).x;
    ImGui::GetWindowDrawList()->AddText(font, 13, ImVec2(p.x + 25 + version_width, p.y + 230), ImColor(99, 182, 255, int(255)), selectedProduct.version.c_str());
    ImGui::SetCursorPos(ImVec2(25, 250));
    //ImGui::Text("Connection: ");
    //int connection_width = ImGui::CalcTextSize("Connection: ").x;
    //ImGui::GetWindowDrawList()->AddText(font, 13, ImVec2(p.x + 25 + connection_width, p.y + 250), ImColor(89, 255, 0, int(255)), globals.connection_message.c_str());

    delete[] items;
}


void ui::renderLoadingProduct(KeyAuth::api& KeyAuthApp)
{
    globals.form_state = failed;
    globals.error_message = "renderLoadingProduct not implemented";
}


void ui::renderError(KeyAuth::api& KeyAuthApp)
{
    TextCentered(globals.error_message, window_size / 2.f);

    using std::chrono::time_point;
    using std::chrono::steady_clock;
    using std::chrono::high_resolution_clock;
    using std::chrono::seconds;

    static time_point<steady_clock> exit_time = high_resolution_clock::now() + seconds(5);

    if (high_resolution_clock::now() > exit_time)
        exit(1337);
}


void ui::render(KeyAuth::api& KeyAuthApp) {
    if (!globals.active) return;

    ImGui::SetNextWindowPos(ImVec2(window_pos.x, window_pos.y), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(window_size.x, window_size.y));
    ImGui::SetNextWindowBgAlpha(1.0f);

    ImGui::Begin(window_title, &globals.active, window_flags);

    switch (globals.form_state)
    {
    case failed:
        renderError(KeyAuthApp);
        break;
    case login:
        renderLogin(KeyAuthApp);
        break;
    case main:
        renderMain(KeyAuthApp);
        break;
    case loading_product:
        renderLoadingProduct(KeyAuthApp);
        break;
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
