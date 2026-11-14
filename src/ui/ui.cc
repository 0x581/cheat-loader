#include "ui.hh"

#include <thread>
#include <cmath>

#include "imgui_internal.h"
#include "custom_ui.hpp"

#include "globals.hh"
#include "utils.hpp"
#include "crypto/crypto.hpp"

// Get directory where the .exe is located
static std::string GetExeDirectory()
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string fullPath(path);
    size_t lastSlash = fullPath.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        return fullPath.substr(0, lastSlash + 1);
    return "";
}

static void TextCentered(const char *text, ImVec2 position)
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 textSize = ImGui::CalcTextSize(text);

    float x_pos = position.x - textSize.x * 0.5f;
    float y_pos = position.y - textSize.y * 0.5f;

    float min_x_pos = 20.0f;
    if (x_pos <= min_x_pos)
    {
        x_pos = min_x_pos;
    }

    float min_y_pos = 20.0f;
    if (y_pos <= min_y_pos)
    {
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

void ui::renderLogin(KeyAuth::api &KeyAuthApp)
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
    if (ImGui::Button(skCrypt("LOGIN"), ImVec2(window_size.x * 0.7f, input_height)))
    {
        static std::thread license_thread{
            [&]()
            {
                KeyAuthApp.license(globals.key);
                license_done = true;
            }};
    }

    if (!license_done)
        return;

    if (KeyAuthApp.response.success)
    {
        globals.form_state = main;
    }
    else
    {
        globals.error_message = KeyAuthApp.response.message;
        globals.form_state = failed;
    }
}

void ui::renderMain(KeyAuth::api &KeyAuthApp)
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

    const auto border_color = ImColor(97, 173, 255, int(25));
    const auto bg_color = ImColor(19, 19, 19, int(255));

    globals.products[0] = ProductInfo{
        .valid = true,
        .name = "cs2",
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

    for (; product_count < 16 && globals.products[product_count].valid; ++product_count)
        ;

    char const **items = new char const *[product_count];

    for (int i = 0; i < product_count; i++)
    {
        items[i] = globals.products[i].name.c_str();
    }

    auto p = ImGui::GetWindowPos();
    ImGuiContext &g = *GImGui;
    ImFont *font = g.Font;

    auto &drawlist = *ImGui::GetWindowDrawList();

    constexpr auto spacing = 0.0375f;
    constexpr auto selection_box_height = 0.333333f;

    const auto titleBarHeight = ImGui::GetFrameHeight() / window_size.y;
    constexpr auto text_height = 0.09f;

    //  Products tab
    auto box_pos = ImVec2(window_size.x * spacing, window_size.y * (titleBarHeight + text_height));
    auto box_size = ImVec2(window_size.x * (1.f - 3.f * spacing) * 0.5f, window_size.y * selection_box_height);

    const auto padding = box_size.x * 0.05f;

    ImGui::SetCursorPos(box_pos + ImVec2(g.Style.FramePadding.x, -(UIDefines::font_size + g.Style.FramePadding.y)));
    ImGui::Text(skCrypt("Cheats"));

    ImGui::SetCursorPos(box_pos);
    auto &style = ImGui::GetStyle();
    const auto old_padding = style.FramePadding;
    style.FramePadding = ImVec2(padding, padding);
    CustomListBox("", &globals.selectedProduct, items, product_count, box_size);
    style.FramePadding = old_padding;
    ImGui::GetWindowDrawList()->AddRect(p + box_pos, p + box_pos + box_size, border_color, 0);

    // Load tab
    box_pos = ImVec2(window_size.x * (0.5f + 0.5f * spacing), box_pos.y);

    ImGui::SetCursorPos(box_pos + ImVec2(g.Style.FramePadding.x, -(UIDefines::font_size + g.Style.FramePadding.y)));
    ImGui::Text(skCrypt("Load"));

    ImGui::GetWindowDrawList()->AddRectFilled(p + box_pos, p + box_pos + box_size, bg_color, 0);
    ImGui::GetWindowDrawList()->AddRect(p + box_pos, p + box_pos + box_size, border_color, 0);

    ProductInfo &selectedProduct = globals.products[globals.selectedProduct];

    ImGui::SetCursorPos(box_pos + ImVec2(padding, padding));
    ImGui::Text(selectedProduct.name.c_str());

    ImGui::SetCursorPos(box_pos + ImVec2(padding, box_size.y * 0.333333f));
    if (ImGui::Button(skCrypt("LOAD"), ImVec2(box_size.x - 2.f * padding, box_size.y * 0.2f)))
    {
        globals.final_selected_product = globals.selectedProduct;
        globals.form_state = loading_product;
    }

    ImGui::SetCursorPos(box_pos + ImVec2(padding, box_size.y * 0.633333f));
    if (ImGui::Button(skCrypt("EXIT"), ImVec2(box_size.x - 2.f * padding, box_size.y * 0.2f)))
    {
        exit(1337);
    }

    // Information tab
    box_pos = ImVec2(window_size.x * spacing, window_size.y * (titleBarHeight + text_height * 2.f + selection_box_height));
    // box_pos = ImVec2(window_size.x * spacing, window_size.y * (1.f - 0.166666f * 2.f - selection_box_height));
    box_size = ImVec2(window_size.x * (1.f - 2.f * spacing), window_size.y * (1.f - spacing) - box_pos.y);

    ImGui::SetCursorPos(box_pos + ImVec2(g.Style.FramePadding.x, -(UIDefines::font_size + g.Style.FramePadding.y)));
    ImGui::Text(skCrypt("Information"));

    ImGui::GetWindowDrawList()->AddRectFilled(p + box_pos, p + box_pos + box_size, bg_color, 0);
    ImGui::GetWindowDrawList()->AddRect(p + box_pos, p + box_pos + box_size, border_color, 0);

    auto text_pos = box_pos + ImVec2(padding, padding);

    const auto info_color = ImColor(99, 182, 255, int(255)); // blue

    std::string status_string = "";
    ImColor status_color{0};

    switch (selectedProduct.status)
    {
    case detected:
        status_string = skCrypt("detected").decrypt();
        status_color = ImColor(200, 0, 0, int(255)); // red
        break;
    case undetected:
        status_string = skCrypt("undetected").decrypt();
        status_color = ImColor(89, 255, 0, int(255)); // green
        break;
    case testing:
        status_string = skCrypt("testing").decrypt();
        status_color = ImColor(201, 204, 0, int(255)); // yellow
        break;
    default:
        exit(6969);
    }

    ImGui::SetCursorPos(text_pos);
    ImGui::Text(skCrypt("Status: "));
    auto text_size = ImGui::CalcTextSize(skCrypt("Status: "));
    ImGui::SetCursorPos(text_pos + ImVec2(text_size.x, 0));
    ImGui::TextColored(status_color, status_string.c_str());

    text_pos += ImVec2(0, text_size.y);
    ImGui::SetCursorPos(text_pos);
    ImGui::Text(skCrypt("Last Update: "));
    text_size = ImGui::CalcTextSize(skCrypt("Last Update: "));
    ImGui::SetCursorPos(text_pos + ImVec2(text_size.x, 0));
    ImGui::TextColored(info_color, selectedProduct.last_update.c_str());

    text_pos += ImVec2(0, text_size.y);
    ImGui::SetCursorPos(text_pos);
    ImGui::Text(skCrypt("Version: "));
    text_size = ImGui::CalcTextSize(skCrypt("Version: "));
    ImGui::SetCursorPos(text_pos + ImVec2(text_size.x, 0));
    ImGui::TextColored(info_color, selectedProduct.version.c_str());

    // ImGui::SetCursorPos(ImVec2(25, 250));
    // ImGui::Text("Connection: ");
    // int connection_width = ImGui::CalcTextSize("Connection: ").x;
    // ImGui::GetWindowDrawList()->AddText(font, 13, ImVec2(p.x + 25 + connection_width, p.y + 250), ImColor(89, 255, 0, int(255)), globals.connection_message.c_str());

    delete[] items;
}

// Launch executable using CreateProcess
static bool LaunchExecutable(const std::string &exePath)
{
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {0};

    // Create the process
    if (!CreateProcessA(
            exePath.c_str(), // Application path
            NULL,            // Command line
            NULL,            // Process security attributes
            NULL,            // Thread security attributes
            FALSE,           // Inherit handles
            0,               // Creation flags
            NULL,            // Environment
            NULL,            // Current directory
            &si,             // Startup info
            &pi))            // Process info
    {
        return false;
    }

    // Close handles (process continues running independently)
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return true;
}

void ui::renderLoadingProduct(KeyAuth::api &KeyAuthApp)
{
    // Loading spinner animation
    static float rotation = 0.0f;
    rotation += ImGui::GetIO().DeltaTime * 5.0f;
    if (rotation > 6.28318f)
        rotation -= 6.28318f;

    ImVec2 center = ImVec2(window_size.x / 2.f, window_size.y / 2.f - 30.f);

    // Draw loading text
    const char *loadingText = "Loading product...";
    TextCentered(loadingText, center);

    // Draw product name below
    ProductInfo &product = globals.products[globals.final_selected_product];
    if (product.valid)
    {
        ImVec2 productNamePos = ImVec2(window_size.x / 2.f, window_size.y / 2.f + 10.f);
        TextCentered(product.name.c_str(), productNamePos);
    }

    // Simple spinner dots
    ImVec2 spinnerCenter = ImVec2(window_size.x / 2.f, window_size.y / 2.f + 50.f);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();

    for (int i = 0; i < 8; i++)
    {
        float angle = rotation + (i * 0.785398f); // 45 degrees apart
        float alpha = 1.0f - (i * 0.1f);
        ImVec2 dotPos = ImVec2(
            windowPos.x + spinnerCenter.x + cosf(angle) * 15.f,
            windowPos.y + spinnerCenter.y + sinf(angle) * 15.f);
        draw_list->AddCircleFilled(dotPos, 3.f, ImColor(1.f, 1.f, 1.f, alpha));
    }

    // Launch the encrypted product executable (only once per product selection)
    static bool launchAttempted = false;

    if (!launchAttempted)
    {
        launchAttempted = true;

        // Build path to encrypted executable (.enc file) - relative to exe location
        std::string encryptedPath = GetExeDirectory() + "products\\" + product.name + ".enc";

        // Decrypt and execute
        std::string error = Crypto::DecryptAndExecuteWithError(encryptedPath);

        if (error.empty())
        {
            globals.form_state = loaded_product;
        }
        else
        {
            globals.error_message = error;
            globals.form_state = failed;
            launchAttempted = false; // Allow retry on failure
        }
    }
}

void ui::renderError(KeyAuth::api &KeyAuthApp)
{
    TextCentered(globals.error_message, window_size / 2.f);

    using std::chrono::high_resolution_clock;
    using std::chrono::seconds;
    using std::chrono::steady_clock;
    using std::chrono::time_point;

    static time_point<steady_clock> exit_time = high_resolution_clock::now() + seconds(5);

    if (high_resolution_clock::now() > exit_time)
        exit(1337);
}

void ui::render(KeyAuth::api &KeyAuthApp)
{
    if (!globals.active)
        return;

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
    case loaded_product:
        // Product loaded successfully - show success or close
        TextCentered("Product loaded!", ImVec2(window_size.x / 2.f, window_size.y / 2.f));
        break;
    }

    ImGui::End();
}

void ui::init(LPDIRECT3DDEVICE9 device)
{
    dev = device;

    // colors
    ImGui::StyleColorsDark();
    ImVec4 *colors = ImGui::GetStyle().Colors;
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

    if (window_pos.x == 0)
    {
        RECT screen_rect{};
        GetWindowRect(GetDesktopWindow(), &screen_rect);
        screen_res = ImVec2(float(screen_rect.right), float(screen_rect.bottom));
        window_pos = (screen_res - window_size) * 0.5f;
    }
}
