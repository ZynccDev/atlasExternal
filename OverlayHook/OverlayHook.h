#pragma once

#pragma comment (lib, "d3d11.lib")

#pragma comment(lib, "dwmapi.lib")
#include <dwmapi.h>

#include <Windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include "imgui/stb_image.h"

#include <d3d11.h>
#include <tchar.h>
#include <algorithm>
#include <string>
#include <thread>
#include <iostream>
#include <filesystem>
#include <unordered_set>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/KeyBind.h"
#include "imgui/imgui_settings.h"

#include "imgui/imgui_internal.h"

#include "font/font.h"

#include "../Globals.h"
#include "../Options.h"

#include "../Misc/Configs.h"

#include "../Hacks/esp.h"
#include "../Hacks/aimbot.h"
#include "../Hacks/hitbox.h"
#include "../Hacks/movement.h"
#include "../Hacks/Arsenal/rapidfire.h"

#include "W2S.h"

extern ID3D11Device* g_pd3dDevice;

bool LoadTextureFromMemory(
    const void* data,
    size_t data_size,
    ID3D11ShaderResourceView** out_srv,
    int* out_width,
    int* out_height)
{
    ID3D11Device* device = g_pd3dDevice;

    if (!device) return false;
    if (!data || data_size == 0) return false;

    int image_width = 0;
    int image_height = 0;
    int channels = 0;

    unsigned char* image_data = stbi_load_from_memory(
        (const stbi_uc*)data,
        (int)data_size,
        &image_width,
        &image_height,
        &channels,
        STBI_rgb_alpha);

    if (image_data == NULL) {
        return false;
    }

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;

    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;

    HRESULT hr = device->CreateTexture2D(&desc, &subResource, &pTexture);

    stbi_image_free(image_data);

    if (FAILED(hr)) {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hr = device->CreateShaderResourceView(pTexture, &srvDesc, out_srv);

    pTexture->Release();

    if (FAILED(hr)) {
        return false;
    }

    *out_width = image_width;
    *out_height = image_height;

    return true;
}

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool IsGameOnTop(const std::string& expectedTitle) {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;

    char windowTitle[256];
    int length = GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

    if (length == 0) return false;

    return expectedTitle == std::string(windowTitle);
}

void SetTransparency(HWND hwnd, bool boolean)
{
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (boolean)
    {
        exStyle |= WS_EX_TRANSPARENT;
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    }
    else
    {
        exStyle &= ~WS_EX_TRANSPARENT;
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    }
}

void DrawNode(RobloxInstance& node)
{
    const auto& children = node.GetChildren();
    if (children.empty())
    {
        ImGui::BulletText(node.Name().c_str());
    }
    else
    {
        if (ImGui::TreeNode(node.Name().c_str()))
        {
            for (auto child : children)
            {
                DrawNode(child);
            }
            ImGui::TreePop();
        }
    }
}

ImVec4 HexToColorVec4(unsigned int hex_color, float alpha)
{
    ImVec4 color;

    color.x = ((hex_color >> 16) & 0xFF) / 255.0f;
    color.y = ((hex_color >> 8) & 0xFF) / 255.0f;
    color.z = (hex_color & 0xFF) / 255.0f;
    color.w = alpha;

    return color;

}

void ShowImgui()
{
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    size_t width = (size_t)GetSystemMetrics(SM_CXSCREEN);
    size_t height = (size_t)GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"znc", nullptr };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"zyncissopro",
        WS_POPUP,
        0, 0, width + 1, height + 1,
        nullptr, nullptr, wc.hInstance, nullptr);

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS Margin = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &Margin);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();


    ImGui::StyleColorsDark();


    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 accent = HexToColorVec4(0x9896d6, 1);
    ImVec4 sumthings = HexToColorVec4(0x57579c, 1);
    style.Colors[ImGuiCol_WindowBg] = HexToColorVec4(0x0d0d0d, 1);
    style.Colors[ImGuiCol_FrameBg] = HexToColorVec4(0x2c2c2c, 1);
    style.Colors[ImGuiCol_FrameBgActive] = HexToColorVec4(0x2c2c2c, 1);
    style.Colors[ImGuiCol_FrameBgHovered] = HexToColorVec4(0x535353, 1);

    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);

    style.WindowBorderSize = 1.5;
    style.ChildBorderSize = 1.5;
    style.Colors[ImGuiCol_Border] = HexToColorVec4(0x353244, 1);

    style.Colors[ImGuiCol_CheckMark] = sumthings;
    style.Colors[ImGuiCol_Header] = sumthings;
    style.Colors[ImGuiCol_HeaderActive] = sumthings;
    style.Colors[ImGuiCol_HeaderHovered] = ImColor(35, 35, 35);
    style.Colors[ImGuiCol_SliderGrab] = HexToColorVec4(0x6d6d6d, 1);
    style.Colors[ImGuiCol_SliderGrabActive] = HexToColorVec4(0x6d6d6d, 1);
    style.Colors[ImGuiCol_Separator] = accent;
    style.Colors[ImGuiCol_SeparatorActive] = accent;
    style.Colors[ImGuiCol_SeparatorHovered] = accent;

    style.Colors[ImGuiCol_Button] = sumthings;
    style.Colors[ImGuiCol_ButtonActive] = sumthings;
    style.Colors[ImGuiCol_ButtonHovered] = sumthings;
    float menuglow = 50.0f;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    ImGui_ImplDX11_CreateDeviceObjects();

    ImVec4 clear_color = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, 0) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(30);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        static bool showMenu = true;

        static std::string spectatingSubject;

		if (GetAsyncKeyState(Options::MenuKey) & 1) // Delete key to toggle menu
        {
            SetTransparency(hwnd, showMenu);
            showMenu = !showMenu;
        }

        ImVec2 atlasPos(0, 0);
        ImVec2 atlasSize(0, 0);

        auto character = Globals::Roblox::LocalPlayer.Character();
        if (GetForegroundWindow() == FindWindowA(nullptr, "Roblox") || GetForegroundWindow() == hwnd) // find roblox window by its title
        {
            if (showMenu)
            {
                ImGui::SetNextWindowSize(ImVec2(850, 516), ImGuiCond_Once);
                ImGui::Begin("retarded ui", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
                atlasPos = ImGui::GetWindowPos();
                atlasSize = ImGui::GetWindowSize();
                ImGui::PushStyleColor(ImGuiCol_Text, accent);
                ImGui::Text("atlas / enjoy skids / made by zync");
                ImGui::PopStyleColor();

                static int selected_tab;
                const char* tabNames[] = { "aimbot", "visuals", "rage", "localplayer", "config", "explorer", "settings"};
                const int tabCount = sizeof(tabNames) / sizeof(tabNames[0]);
                for (int i = 0; i < tabCount; ++i) {
                    ImGui::PushID(i);

                    if (i == 0) {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                    }

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.7f, 0.7f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));

                    if (selected_tab == i) {
                        ImGui::PushStyleColor(ImGuiCol_Text, accent);
                    }
                    else {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertFloat4ToU32(ImColor(150, 150, 150)));
                    }

                    ImVec2 buttonSize = ImVec2(80, 25);
                    if (ImGui::Button(tabNames[i], buttonSize)) {
                        selected_tab = i;
                    }

                    if (selected_tab == i) {
                        ImVec2 pos = ImGui::GetItemRectMin();
                        ImGui::GetWindowDrawList()->AddLine(
                            ImVec2(pos.x, pos.y + buttonSize.y),
                            ImVec2(pos.x + buttonSize.x, pos.y + buttonSize.y),
                            ImGui::ColorConvertFloat4ToU32(accent), 2.0f);
                    }

                    ImGui::PopStyleColor(1);
                    ImGui::PopStyleColor(3);
                    ImGui::SameLine();
                    ImGui::PopID();
                }

                ImGui::Spacing();


				if (selected_tab == 0) { // aimbot tab
                    ImGui::BeginChild("content", ImVec2(0, 0), true);

                    ImGui::Columns(2, nullptr, false);
                    ImGui::BeginChild("child_1", ImVec2(0, 125), true);
                    {
                        ImGui::SeparatorText("aimbot");
                        ImGui::Checkbox("Aimbot", &Options::Aimbot::Aimbot);
                        ImGui::SameLine();
                        KeyBind::KeyBindButton("##AimbotKey", &Options::Aimbot::AimbotKey);
                        ImGui::Text("Toggle Type");
                        static const char* toggleType[]{ "Hold", "Toggle" };
                        if (ImGui::Combo("##ToggleType", &Options::Aimbot::ToggleType, toggleType, IM_ARRAYSIZE(toggleType)))
                        {
                            Options::Aimbot::Toggled = false;
                            Options::Aimbot::CurrentTarget = RobloxPlayer(0);
                        }
                    }
                    ImGui::EndChild();


                    ImGui::NextColumn();

                    ImGui::BeginChild("child_2", ImVec2(0, 125), true);
                    {
                        ImGui::SeparatorText("fov");
                        ImGui::ColorEdit3("##FOVColor", Options::Aimbot::FOVColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                        ImGui::SameLine();
                        ImGui::Checkbox("Show FOV", &Options::Aimbot::ShowFOV);
                        ImGui::SliderFloat("FOV", &Options::Aimbot::FOV, 10.f, 360.f, "%.0f");
                    }
                    ImGui::EndChild();


                    ImGui::Columns(1);

                    ImGui::BeginChild("child_3", ImVec2(0, 0), true);
                    {
                        ImGui::SeparatorText("aimbot config");
                        ImGui::Text("Team Check");
                        ImGui::Checkbox("Team Check", &Options::Aimbot::TeamCheck);

                        ImGui::Text("Dead Check");
                        ImGui::Checkbox("Dead Check", &Options::Aimbot::DownedCheck);

                        ImGui::Text("Aiming Method");
                        static const char* aimingMethods[]{ "Camera", "Mouse" };
                        ImGui::Combo("##AimingMethod", &Options::Aimbot::AimingType, aimingMethods, IM_ARRAYSIZE(aimingMethods));

                        ImGui::Text("Target Bone");
                        static const char* targetBones[]{ "Head", "HumanoidRootPart", "Left Arm", "Right Arm", "Left Leg", "Right Leg", "Random" };
                        ImGui::Combo("##TargetBone", &Options::Aimbot::TargetBone, targetBones, IM_ARRAYSIZE(targetBones));

                        ImGui::Checkbox("Sticky Aim", &Options::Aimbot::StickyAim);
                        ImGui::Checkbox("Transparency Check", &Options::Aimbot::TransparencyCheck);

                        ImGui::SliderFloat("Smoothness", &Options::Aimbot::Smoothness, 0.f, 1.f, "%.1f");
                        ImGui::SliderFloat("Range (Studs)", &Options::Aimbot::Range, 1.f, 1000.f, "%.0f");
                    }
                    ImGui::EndChild();

                    ImGui::EndChild();
                }

				if (selected_tab == 1) { // visuals tab
                    ImGui::BeginChild("content", ImVec2(0, 0), true);
                    ImGui::SeparatorText("visual config");
                    ImGui::Checkbox("ESP Preview", &Options::espPreview);
                    ImGui::Checkbox("Team Check", &Options::ESP::TeamCheck);
                    ImGui::Checkbox("Remove Borders", &Options::ESP::RemoveBorders);
                    //ImGui::Checkbox("Rivals Team Check", &Options::ESP::RivalsTeamCheck);
                    ImGui::SeparatorText("visuals");
                    ImGui::Text("Box ESP");
                    ImGui::SameLine();
                    ImGui::ColorEdit3("##BoxColor", Options::ESP::BoxColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Box ESP", &Options::ESP::Box);
                    static const char* boxStyles[]{ "Normal", "Rounded", "Corner" };
                    ImGui::Combo("##BoxStyle", &Options::ESP::BoxType, boxStyles, IM_ARRAYSIZE(boxStyles));

                    ImGui::Text("Skeleton ESP");
                    ImGui::SameLine();
                    ImGui::ColorEdit3("##SkeletonColor", Options::ESP::SkeletonColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Skeleton ESP", &Options::ESP::Skeleton);

                    ImGui::Text("Name ESP");
                    ImGui::SameLine();
                    ImGui::ColorEdit3("##NameColor", Options::ESP::Color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Name ESP", &Options::ESP::Name);

                    ImGui::Text("Distance ESP");
                    ImGui::SameLine();
                    ImGui::ColorEdit3("##DistanceColor", Options::ESP::DistanceColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Distance ESP", &Options::ESP::Distance);

                    ImGui::Text("Health Bar");
                    ImGui::Checkbox("Health Bar", &Options::ESP::Health);
                    static const char* healthStyle[]{ "Normal","Glow" };
                    ImGui::Combo("##HealthStyle", &Options::ESP::HealthType, healthStyle, IM_ARRAYSIZE(healthStyle));

                    ImGui::Text("3D ESP");
                    ImGui::SameLine();
                    ImGui::ColorEdit3("##ESP3DColor", Options::ESP::ESP3DColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("3D ESP", &Options::ESP::ESP3D);

                    ImGui::Text("Head Circle");
                    ImGui::SameLine();
                    ImGui::ColorEdit3("##HeadCircleColor", Options::ESP::HeadCircleColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Head Circle", &Options::ESP::HeadCircle);

                    ImGui::Text("Crosshair");
                    ImGui::SameLine();
                    ImGui::ColorEdit3("##CrosshairColor", Options::ESP::CrosshairColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Crosshair", &Options::ESP::Crosshair);

                    ImGui::Text("Tracers");
                    ImGui::SameLine();
                    ImGui::ColorEdit3("##TracerColor", Options::ESP::TracerColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Tracers", &Options::ESP::Tracers);

                    ImGui::Checkbox("Transparency Check", &Options::ESP::TransparencyCheck);

                    ImGui::SliderFloat("Tracer Thickness", &Options::ESP::TracerThickness, 1.0f, 10.0f, "%.1f");

                    static const char* tracersStarts[]{ "Bottom","Top","Mouse","Torso" };

                    ImGui::Combo("Tracers Start", &Options::ESP::TracersStart, tracersStarts, IM_ARRAYSIZE(tracersStarts));
                    ImGui::EndChild();
				}

                if (selected_tab == 2) { // rage tab
                    ImGui::BeginChild("content", ImVec2(0, 0), true);
                    ImGui::Columns(2, nullptr, false);
                    ImGui::BeginChild("child_6", ImVec2(0, 0), true);
                    {
                        ImGui::SeparatorText("rage");
						ImGui::Checkbox("Anti-Aim", &Options::AntiAim::Normal);
                        
                    }
                    ImGui::EndChild();

                    ImGui::NextColumn();

                    ImGui::BeginChild("child_7", ImVec2(0, 0), true);
                    {
                        ImGui::SeparatorText("Config");
						ImGui::Checkbox("Underground Anti-Aim", &Options::AntiAim::Underground);
                        ImGui::SetWindowFontScale(0.85f);
                        ImGui::Text("Anti-Aim Must be Off to use this!");
                        ImGui::SetWindowFontScale(1.0f);
                    }
                    ImGui::EndChild();
                    ImGui::EndChild();
                }

				if (selected_tab == 3) { // localplayer tab
                    ImGui::BeginChild("content", ImVec2(0, 0), true);

                    ImGui::Columns(2, nullptr, false);
                    ImGui::BeginChild("child_4", ImVec2(0, 0), true);
                    {
                        static auto humanoid = character.FindFirstChildWhichIsA("Humanoid");
                        ImGui::SeparatorText("localplayer");
                        ImGui::SliderFloat("Walkspeed", &Options::Misc::Walkspeed, 0.f, 500.f, "%.0f");
                        // walkspeed and jumpower toggle dont even work bro
						if (ImGui::Checkbox("Set Walkspeed", &Options::Misc::WalkspeedToggle))
                        {
							humanoid.SetWalkspeed(Options::Misc::Walkspeed);
                        }
                        ImGui::SliderFloat("JumpPower", &Options::Misc::JumpPower, 0.f, 500.f, "%.0f");
                        if (ImGui::Checkbox("Set JumpPower", &Options::Misc::JumpPowerToggle))
                        {
                            humanoid.SetJumpPower(Options::Misc::JumpPower);
                        }
                        ImGui::SliderFloat("FOV", &Options::Misc::FOV, 1.f, 120, "%.0f");
						//ImGui::Checkbox("NoClip", &Options::Misc::Noclip); // ill fix it later
                        ImGui::Checkbox("Anti-AFK", &Options::Misc::AntiAFK);
                        //ImGui::Checkbox("Desync", &Options::Misc::Desync); // this dont work
                        ImGui::Checkbox("Fly", &Options::Misc::Fly);
						ImGui::SameLine();
                        KeyBind::KeyBindButton("##FlyKey", &Options::Misc::FlyKey);
                        ImGui::SliderFloat("Fly Speed", &Options::Misc::FlySpeed, 0.f, 50., "%.0f");                 
                        ImGui::Checkbox("Headless", &Options::Misc::Headless);
                    }
                    ImGui::EndChild();

                    ImGui::NextColumn();

                    ImGui::BeginChild("child_5", ImVec2(0, 0), true);
                    {
                        ImGui::SeparatorText("Arsenal");
                        ImGui::Checkbox("Arsenal Rapid Fire", &Options::Arsenal::RapidFire); // skidded asf 
                        ImGui::SliderFloat("Delay##ArsenalRapid", &Options::Arsenal::RapidFireDelay, 0.01f, 10.f, "%.0f");
                    }
                    ImGui::EndChild();

                    ImGui::EndChild();
				}

                if (selected_tab == 4) { // config tab
                    ImGui::BeginChild("content", ImVec2(0, 0), true);
                    ImGui::SeparatorText("configs");
                    Globals::configsArray.clear();
                    for (const auto& entry : std::filesystem::directory_iterator(Globals::configsPath))
                    {
                        if (entry.path().extension() == ".json")
                        {
                            Globals::configsArray.push_back(entry.path().filename().string());
                        }
                    }

                    for (auto& config : Globals::configsArray)
                    {
                        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                        ImVec2 contentRegion = ImGui::GetContentRegionAvail();

                        float height = 30.0f;
                        float paddingX = 8.0f;

                        float buttonWidth = 50.0f;
                        float spacing = 5.0f;
                        float totalButtonWidth = (buttonWidth * 2) + spacing;
                        float labelWidth = contentRegion.x - totalButtonWidth - spacing;

                        ImVec2 rectMin = cursorPos;
                        ImVec2 rectMax = ImVec2(cursorPos.x + labelWidth - 6.f, cursorPos.y + height);

                        ImU32 rectColor = IM_COL32(46, 44, 53, 255);
                        ImU32 textColor = IM_COL32(255, 255, 255, 255);

                        ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, rectColor, 4.0f);

                        ImVec2 textSize = ImGui::CalcTextSize(config.c_str());
                        ImVec2 textPos = ImVec2(
                            rectMin.x + (labelWidth - textSize.x) / 2.0f,
                            rectMin.y + (height - textSize.y) / 2.0f
                        );
                        ImGui::GetWindowDrawList()->AddText(textPos, textColor, config.c_str());

                        ImGui::SetCursorScreenPos(ImVec2(rectMax.x + spacing, rectMin.y));

                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 6.f));

                        std::string loadId = "load";
                        std::string deleteId = "delete";

                        if (ImGui::Button(loadId.c_str(), ImVec2(buttonWidth, height)))
                        {
                            LoadConfig(config);
                        }

                        ImGui::SameLine();

                        if (ImGui::Button(deleteId.c_str(), ImVec2(buttonWidth, height)))
                        {
                            std::filesystem::remove(Globals::configsPath + "\\" + config);
                        }

                        ImGui::PopStyleVar();

                        ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, rectMax.y + 5.0f));
                        ImGui::Dummy(ImVec2(1, 1));
                    }

                    static char buf[256] = "";

                    ImGui::InputText("##SaveIconInput", buf, sizeof(buf));
                    ImGui::SameLine();

                    if (ImGui::Button("save config"))
                        CreateConfig(std::string(static_cast<std::string>(buf) + ".json"));
                    ImGui::EndChild();
				}

                if (selected_tab == 5) {
                    ImGui::BeginChild("explorer_content", ImVec2(0, 0), true);

                    static const std::unordered_set<std::string> services = {
                        "Workspace", "Players", "Lighting", "ReplicatedFirst", "ReplicatedStorage",
                        "StarterGui", "StarterPack", "StarterPlayer", "Teams", "SoundService",
                        "Chat", "TextChatService", "LocalizationService",
                        "ServerScriptService", "ServerStorage", "CoreGui"
                    };

                    auto children = Globals::Roblox::DataModel.GetChildren();

                    for (auto& child : children) {
                        std::string name = child.Name();
                        if (services.find(name) != services.end()) {
                            DrawNode(child);
                        }
                    }

                    ImGui::EndChild();
                }

                if (selected_tab == 6) { // settings tab
                    ImGui::BeginChild("content", ImVec2(0, 0), true);
                    ImGui::SeparatorText("settings");
                    ImGui::Text("Menu Keybind");
                    KeyBind::KeyBindButton("##MenuKeybind", &Options::MenuKey);
                    ImGui::Checkbox("Playerlist", &Options::playerList);
                    ImGui::Checkbox("Watermark", &Options::Misc::Watermark);
                    ImGui::Checkbox("Streamproof", &Options::Misc::Streamproof);
                    ImGui::EndChild();
                }

                ImGui::End();
            }

            ImGui::SetNextWindowSize(ImVec2(250, 516), ImGuiCond_Always);
            if (showMenu && Options::playerList)
            {

                float plx = atlasPos.x + atlasSize.x + 10.0f;
                float psy = atlasPos.y;
                ImGui::SetNextWindowPos(ImVec2(plx, psy), ImGuiCond_Always);
                ImGui::Begin("PlayerList", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
                ImGui::PushStyleColor(ImGuiCol_Text, accent);
                ImGui::Text("Player List");
                ImGui::PopStyleColor();
                for (auto& player : Globals::Caches::CachedPlayerObjects)
                {
                    ImGui::BulletText(player.Name.c_str());

                    std::string teleportID = "Teleport##" + player.Name;
                    if (ImGui::Button(teleportID.c_str()))
                    {
                        auto localCFrame = character.FindFirstChild("HumanoidRootPart").CFrame();
                        auto playerCFrame = player.Character.FindFirstChild("HumanoidRootPart").CFrame();

                        localCFrame.x = playerCFrame.x;
                        localCFrame.y = playerCFrame.y;
                        localCFrame.z = playerCFrame.z;

                        auto primitive = Memory->read<uintptr_t>(character.FindFirstChild("HumanoidRootPart").address + offsets::Primitive);

                        std::thread([=]() {
                            for (auto i = 0; i != 100; i++)
                                Memory->write<Vectors::Vector3>(primitive + offsets::Position, { localCFrame.x, localCFrame.y, localCFrame.z });
                            }).detach(); // you gotta run it in a thread or it crashes cuz it times out ImGui
                    }

                    ImGui::SameLine();

                    std::string spectateID = "Spectate##" + player.Name;
                    if (ImGui::Button(spectateID.c_str()))
                    {
                        if (spectatingSubject == player.Name)
                        {
                            spectatingSubject = "";
                            Memory->write<uintptr_t>(Globals::Roblox::Camera.address + offsets::CameraSubject, character.FindFirstChildWhichIsA("Humanoid").address);
                        }
                        else
                        {
                            Memory->write<uintptr_t>(Globals::Roblox::Camera.address + offsets::CameraSubject, player.Humanoid.address);
                            spectatingSubject = player.Name;
                        }
                    }
                }
                ImGui::End();
            }

            ImColor	color = IM_COL32(
                static_cast<int>(Options::ESP::Color[0] * 255.f),
                static_cast<int>(Options::ESP::Color[1] * 255.f),
                static_cast<int>(Options::ESP::Color[2] * 255.f),
                255);

            ImColor	boxColor = IM_COL32(
                static_cast<int>(Options::ESP::BoxColor[0] * 255.f),
                static_cast<int>(Options::ESP::BoxColor[1] * 255.f),
                static_cast<int>(Options::ESP::BoxColor[2] * 255.f),
                255);

            ImColor	boxFilledColor = IM_COL32(
                static_cast<int>(Options::ESP::BoxFillColor[0] * 255.f),
                static_cast<int>(Options::ESP::BoxFillColor[1] * 255.f),
                static_cast<int>(Options::ESP::BoxFillColor[2] * 255.f),
                static_cast<int>(Options::ESP::BoxFillColor[3] * 255.f));

            ImColor	skeletonColor = IM_COL32(
                static_cast<int>(Options::ESP::SkeletonColor[0] * 255.f),
                static_cast<int>(Options::ESP::SkeletonColor[1] * 255.f),
                static_cast<int>(Options::ESP::SkeletonColor[2] * 255.f),
                255);

            ImColor	distanceColor = IM_COL32(
                static_cast<int>(Options::ESP::DistanceColor[0] * 255.f),
                static_cast<int>(Options::ESP::DistanceColor[1] * 255.f),
                static_cast<int>(Options::ESP::DistanceColor[2] * 255.f),
                255);

            ImColor	toolColor = IM_COL32(
                static_cast<int>(Options::ESP::ToolColor[0] * 255.f),
                static_cast<int>(Options::ESP::ToolColor[1] * 255.f),
                static_cast<int>(Options::ESP::ToolColor[2] * 255.f),
                255);

            ImColor	crosshairColor = IM_COL32(
                static_cast<int>(Options::ESP::CrosshairColor[0] * 255.f),
                static_cast<int>(Options::ESP::CrosshairColor[1] * 255.f),
                static_cast<int>(Options::ESP::CrosshairColor[2] * 255.f),
                255);

            ImColor	tracerColor = IM_COL32(
                static_cast<int>(Options::ESP::TracerColor[0] * 255.f),
                static_cast<int>(Options::ESP::TracerColor[1] * 255.f),
                static_cast<int>(Options::ESP::TracerColor[2] * 255.f),
                255);

            ImColor	cornerColor = IM_COL32(
                static_cast<int>(Options::ESP::CornerColor[0] * 255.f),
                static_cast<int>(Options::ESP::CornerColor[1] * 255.f),
                static_cast<int>(Options::ESP::CornerColor[2] * 255.f),
                255);

            ImGui::SetNextWindowSize(ImVec2(325, 516), ImGuiCond_Always);
            if (showMenu && Options::espPreview)
            {
                float plx = atlasPos.x + atlasSize.x + 10.0f;
                float psy = atlasPos.y;
                if (Options::playerList && Options::espPreview)
					plx += 260.0f;
                ImGui::SetNextWindowPos(ImVec2(plx, psy), ImGuiCond_Always);
                ImGui::Begin("ESPpreview", nullptr, ImGuiWindowFlags_NoTitleBar);
                ImGui::PushStyleColor(ImGuiCol_Text, accent);
                ImGui::Text("ESP Preview");
                ImGui::PopStyleColor();

                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 window_pos = ImGui::GetWindowPos();
                ImU32 dark_grey = ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

                const float ROUNDING = 4.0f;
                const float AVATAR_SCALE = 1.3f;

                const float HEAD_SIZE = 40.0f * AVATAR_SCALE;
                const float TORSO_W = 60.0f * AVATAR_SCALE;
                const float TORSO_H = 76.0f * AVATAR_SCALE;
                const float ARM_W = 35.0f * AVATAR_SCALE;
                const float ARM_H = 75.0f * AVATAR_SCALE;
                const float LEG_W = 30.0f * AVATAR_SCALE;
                const float LEG_H = 70.0f * AVATAR_SCALE;

                float avatar_total_width = TORSO_W + (ARM_W * 2);
                float avatar_total_height = HEAD_SIZE + TORSO_H + LEG_H;
                float drawing_area_start_y = window_pos.y + ImGui::GetFrameHeightWithSpacing() * 2;
                float drawing_area_height = 516.0f - (drawing_area_start_y - window_pos.y);

                float center_x_anchor = window_pos.x + (325.0f / 2.0f) - (avatar_total_width / 2.0f);
                float start_y = drawing_area_start_y + (drawing_area_height / 2.0f) - (avatar_total_height / 2.0f);

                float torso_x_start = center_x_anchor + ARM_W;
                float torso_x_end = torso_x_start + TORSO_W;
                float torso_y_start = start_y + HEAD_SIZE;
                float torso_y_end = torso_y_start + TORSO_H;
                float head_x_start = torso_x_start + (TORSO_W / 2.0f) - (HEAD_SIZE / 2.0f);
                float legs_y_start = torso_y_end;

                drawList->AddRectFilled(ImVec2(torso_x_start, torso_y_start), ImVec2(torso_x_end, torso_y_end), dark_grey, ROUNDING, ImDrawFlags_RoundCornersAll);
                drawList->AddRectFilled(ImVec2(head_x_start, start_y), ImVec2(head_x_start + HEAD_SIZE, torso_y_start), dark_grey, ROUNDING, ImDrawFlags_RoundCornersAll);
                drawList->AddRectFilled(ImVec2(center_x_anchor, torso_y_start), ImVec2(center_x_anchor + ARM_W, torso_y_start + ARM_H), dark_grey, ROUNDING, ImDrawFlags_RoundCornersAll);
                drawList->AddRectFilled(ImVec2(torso_x_end, torso_y_start), ImVec2(torso_x_end + ARM_W, torso_y_start + ARM_H), dark_grey, ROUNDING, ImDrawFlags_RoundCornersAll);
                drawList->AddRectFilled(ImVec2(torso_x_start, legs_y_start), ImVec2(torso_x_start + LEG_W, legs_y_start + LEG_H), dark_grey, ROUNDING, ImDrawFlags_RoundCornersAll);
                drawList->AddRectFilled(ImVec2(torso_x_start + LEG_W, legs_y_start), ImVec2(torso_x_start + LEG_W + LEG_W, legs_y_start + LEG_H), dark_grey, ROUNDING, ImDrawFlags_RoundCornersAll);


                float left = center_x_anchor;
                float right = center_x_anchor + avatar_total_width;
                float top = start_y;
                float bottom = start_y + avatar_total_height;

                ImVec2 head2D = ImVec2(head_x_start + HEAD_SIZE / 2.0f, start_y + HEAD_SIZE / 2.0f);
                ImVec2 torso2D = ImVec2(torso_x_start + TORSO_W / 2.0f, torso_y_start + TORSO_H / 2.0f);
                ImVec2 lArm2D = ImVec2(center_x_anchor + ARM_W / 2.0f, torso_y_start + ARM_H / 2.0f);
                ImVec2 rArm2D = ImVec2(torso_x_end + ARM_W / 2.0f, torso_y_start + ARM_H / 2.0f);
                ImVec2 lLeg2D = ImVec2(torso_x_start + LEG_W / 2.0f, legs_y_start + LEG_H / 2.0f);
                ImVec2 rLeg2D = ImVec2(torso_x_start + LEG_W + LEG_W / 2.0f, legs_y_start + LEG_H / 2.0f);

                ImVec2 leftShoulder = ImVec2(torso_x_start, torso_y_start);
                ImVec2 rightShoulder = ImVec2(torso_x_end, torso_y_start);
                ImVec2 hipCenter = ImVec2(torso2D.x, torso_y_end);

                ImVec2 leftHand = ImVec2(lArm2D.x, torso_y_start + ARM_H);
                ImVec2 rightHand = ImVec2(rArm2D.x, torso_y_start + ARM_H);
                ImVec2 leftFoot = ImVec2(lLeg2D.x, legs_y_start + LEG_H);
                ImVec2 rightFoot = ImVec2(rLeg2D.x, legs_y_start + LEG_H);

                const float renderScale = 1.0f;
                const float demoDist = 50.0f;
                const float demoHealthPct = 0.60f;

                if (Options::ESP::Skeleton)
                {
                    if (!Options::ESP::RemoveBorders)
                        drawList->AddLine(head2D, torso2D, IM_COL32(0, 0, 0, 255), 3.5f);
                    drawList->AddLine(head2D, torso2D, skeletonColor, 2.f);

                    if (!Options::ESP::RemoveBorders)
                        drawList->AddLine(leftShoulder, lArm2D, IM_COL32(0, 0, 0, 255), 3.5f);
                    drawList->AddLine(leftShoulder, lArm2D, skeletonColor, 2.f);
                    if (!Options::ESP::RemoveBorders)
                        drawList->AddLine(lArm2D, leftHand, IM_COL32(0, 0, 0, 255), 3.5f);
                    drawList->AddLine(lArm2D, leftHand, skeletonColor, 2.f);

                    if (!Options::ESP::RemoveBorders)
                        drawList->AddLine(rightShoulder, rArm2D, IM_COL32(0, 0, 0, 255), 3.5f);
                    drawList->AddLine(rightShoulder, rArm2D, skeletonColor, 2.f);
                    if (!Options::ESP::RemoveBorders)
                        drawList->AddLine(rArm2D, rightHand, IM_COL32(0, 0, 0, 255), 3.5f);
                    drawList->AddLine(rArm2D, rightHand, skeletonColor, 2.f);

                    if (!Options::ESP::RemoveBorders)
                        drawList->AddLine(hipCenter, lLeg2D, IM_COL32(0, 0, 0, 255), 3.5f);
                    drawList->AddLine(hipCenter, lLeg2D, skeletonColor, 2.f);
                    if (!Options::ESP::RemoveBorders)
                        drawList->AddLine(lLeg2D, leftFoot, IM_COL32(0, 0, 0, 255), 3.5f);
                    drawList->AddLine(lLeg2D, leftFoot, skeletonColor, 2.f);

                    if (!Options::ESP::RemoveBorders)
                        drawList->AddLine(hipCenter, rLeg2D, IM_COL32(0, 0, 0, 255), 3.5f);
                    drawList->AddLine(hipCenter, rLeg2D, skeletonColor, 2.f);
                    if (!Options::ESP::RemoveBorders)
                        drawList->AddLine(rLeg2D, rightFoot, IM_COL32(0, 0, 0, 255), 3.5f);
                    drawList->AddLine(rLeg2D, rightFoot, skeletonColor, 2.f);

                    if (!Options::ESP::RemoveBorders)
                        drawList->AddLine(torso2D, hipCenter, IM_COL32(0, 0, 0, 255), 3.5f);
                    drawList->AddLine(torso2D, hipCenter, skeletonColor, 2.f);

                    if (!Options::ESP::RemoveBorders)
                        drawList->AddLine(leftShoulder, rightShoulder, IM_COL32(0, 0, 0, 255), 3.5f);
                    drawList->AddLine(leftShoulder, rightShoulder, skeletonColor, 2.f);
                }


                if (Options::ESP::Health)
                {
                    switch (Options::ESP::HealthType)
                    {
                    case 0:
                    {
                        float healthPercent = demoHealthPct;

                        float barWidth = std::clamp(renderScale * 4.f, 3.f, 8.f);
                        float boxHeight = bottom - top;

                        ImVec2 barTopLeft(left - barWidth - 4.f, top);
                        ImVec2 barBottomRight(left - 4.f, top + boxHeight);

                        drawList->AddRectFilled(barTopLeft, barBottomRight, IM_COL32(50, 50, 50, 180));

                        float filledHeight = boxHeight * healthPercent;
                        ImVec2 filledTopLeft(barTopLeft.x, barBottomRight.y - filledHeight);
                        ImVec2 filledBottomRight(barBottomRight.x, barBottomRight.y);

                        drawList->AddRectFilled(filledTopLeft, filledBottomRight, IM_COL32(0, 255, 0, 220));

                        drawList->AddRect(barTopLeft, barBottomRight, IM_COL32(0, 0, 0, 255));
                        break;
                    }
                    case 1:
                    {
                        float healthPercent = demoHealthPct;

                        float barWidth = std::clamp(renderScale * 4.f, 3.f, 8.f);
                        float boxHeight = bottom - top;

                        ImVec2 barTopLeft(left - barWidth - 4.f, top);
                        ImVec2 barBottomRight(left - 4.f, top + boxHeight);

                        drawList->AddRectFilled(barTopLeft, barBottomRight, IM_COL32(50, 50, 50, 180));

                        float filledHeight = boxHeight * healthPercent;

                        ImVec2 filledTopLeft(barTopLeft.x, barBottomRight.y - filledHeight);
                        ImVec2 filledBottomRight(barBottomRight.x, barBottomRight.y);

                        float spread = 4.5f;
                        float alpha = 50;

                        ImVec2 glow1_TopLeft(filledTopLeft.x - spread, filledTopLeft.y - spread);
                        ImVec2 glow1_BottomRight(filledBottomRight.x + spread, filledBottomRight.y + spread);
                        drawList->AddRectFilled(glow1_TopLeft, glow1_BottomRight, IM_COL32(0, 255, 0, alpha));

                        ImVec2 glow2_TopLeft(
                            filledTopLeft.x - (spread / 2.f),
                            filledTopLeft.y - (spread / 2.f)
                        );
                        ImVec2 glow2_BottomRight(filledBottomRight.x + (spread / 2.f), filledBottomRight.y + (spread / 2.f));
                        drawList->AddRectFilled(glow2_TopLeft, glow2_BottomRight, IM_COL32(0, 255, 0, alpha * 2));

                        drawList->AddRectFilled(filledTopLeft, filledBottomRight, IM_COL32(0, 255, 0, 220));
                        break;
                    }
                    }
                }

                if (Options::ESP::HeadCircle)
                {
                    ImColor headCircleColor = IM_COL32(
                        static_cast<int>(Options::ESP::HeadCircleColor[0] * 255.f),
                        static_cast<int>(Options::ESP::HeadCircleColor[1] * 255.f),
                        static_cast<int>(Options::ESP::HeadCircleColor[2] * 255.f),
                        180);

                    float boxWidth = right - left;
                    float maxScale = (Options::ESP::HeadCircleMaxScale > 0) ? Options::ESP::HeadCircleMaxScale : 30.0f;

                    float maxRadius = (boxWidth * 0.4f < maxScale) ? boxWidth * 0.4f : maxScale;
                    float baseRadius = 6.0f * renderScale;

                    float distanceScale = std::clamp(1.5f - (demoDist / 150.0f) * 1.2f, 0.3f, 1.5f);
                    float scaledRadius = baseRadius * distanceScale;

                    float boxHeight = bottom - top;
                    float maxCircleSize = boxHeight * 0.15f;
                    float circleRadius = (scaledRadius < 3.0f) ? 3.0f : ((scaledRadius > maxCircleSize) ? maxCircleSize : scaledRadius);

                    drawList->AddCircleFilled(head2D, circleRadius, headCircleColor);

                    if (!Options::ESP::RemoveBorders)
                        drawList->AddCircle(head2D, circleRadius, IM_COL32(0, 0, 0, 255), 0, 2.0f);
                }

                if (Options::ESP::Box)
                {
                    if (left < right && top < bottom)
                    {
                        switch (Options::ESP::BoxType)
                        {
                        case 0:
                        {
                            if (!Options::ESP::RemoveBorders)
                                drawList->AddRect(ImVec2(left, top), ImVec2(right, bottom), IM_COL32(0, 0, 0, 255), 0.0f, 0, 3.5f);
                            drawList->AddRect(ImVec2(left, top), ImVec2(right, bottom), boxColor, 0.0f, 0, 2.f);
                            drawList->AddRectFilled(ImVec2(left, top), ImVec2(right, bottom), boxFilledColor, 0.0f);
                            break;
                        }
                        case 1:
                        {
                            if (!Options::ESP::RemoveBorders)
                                drawList->AddRect(ImVec2(left, top), ImVec2(right, bottom), IM_COL32(0, 0, 0, 255), 4.0f, 0, 3.5f);
                            drawList->AddRect(ImVec2(left, top), ImVec2(right, bottom), boxColor, 4.0f, 0, 2.f);
                            drawList->AddRectFilled(ImVec2(left, top), ImVec2(right, bottom), boxFilledColor, 4.0f);
                            break;
                        }
                        case 2:
                        {
                            const float corner_length = (bottom - top) * 0.10f;

                            if (!Options::ESP::RemoveBorders)
                            {
                                drawList->AddLine(ImVec2(left, top), ImVec2(left, top + corner_length), boxFilledColor, 2.0f);
                                drawList->AddLine(ImVec2(left, top), ImVec2(left + corner_length, top), boxFilledColor, 2.0f);
                                drawList->AddLine(ImVec2(right, top), ImVec2(right, top + corner_length), boxFilledColor, 2.0f);
                                drawList->AddLine(ImVec2(right, top), ImVec2(right - corner_length, top), boxFilledColor, 2.0f);
                                drawList->AddLine(ImVec2(left, bottom), ImVec2(left, bottom - corner_length), boxFilledColor, 2.0f);
                                drawList->AddLine(ImVec2(left, bottom), ImVec2(left + corner_length, bottom), boxFilledColor, 2.0f);
                                drawList->AddLine(ImVec2(right, bottom), ImVec2(right, bottom - corner_length), boxFilledColor, 2.0f);
                                drawList->AddLine(ImVec2(right, bottom), ImVec2(right - corner_length, bottom), boxFilledColor, 2.0f);
                            }
                            drawList->AddLine(ImVec2(left, top), ImVec2(left, top + corner_length), boxColor, 1.0f);
                            drawList->AddLine(ImVec2(left, top), ImVec2(left + corner_length, top), boxColor, 1.0f);
                            drawList->AddLine(ImVec2(right, top), ImVec2(right, top + corner_length), boxColor, 1.0f);
                            drawList->AddLine(ImVec2(right, top), ImVec2(right - corner_length, top), boxColor, 1.0f);
                            drawList->AddLine(ImVec2(left, bottom), ImVec2(left, bottom - corner_length), boxColor, 1.0f);
                            drawList->AddLine(ImVec2(left, bottom), ImVec2(left + corner_length, bottom), boxColor, 1.0f);
                            drawList->AddLine(ImVec2(right, bottom), ImVec2(right, bottom - corner_length), boxColor, 1.0f);
                            drawList->AddLine(ImVec2(right, bottom), ImVec2(right - corner_length, bottom), boxColor, 1.0f);
                            break;
                        }
                        }
                    }
                }


                float yOffsetTop = top - 5.0f;

                if (Options::ESP::Name)
                {
                    const std::string nameStr = Globals::Roblox::LocalPlayer.Name();
                    const float fontSize = 14.0f;

                    ImVec2 textSize = ImGui::CalcTextSize(nameStr.c_str());

                    ImVec2 namePos(
                        left + (right - left) / 2.0f - textSize.x / 2.0f,
                        yOffsetTop - textSize.y
                    );

                    drawList->AddText(ImGui::GetFont(), fontSize, namePos, color, nameStr.c_str());
                    yOffsetTop = namePos.y - 5.0f;
                }

                float yOffsetBot = bottom;

                if (Options::ESP::Distance)
                {
                    std::string distStr = "[50 Studs]";
                    const float fontSize = 12.0f * renderScale;

                    ImFont* activeFont = ImGui::GetFont();
                    ImVec2 textSize = activeFont->CalcTextSizeA(fontSize, FLT_MAX, 0.f, distStr.c_str());

                    ImVec2 distPos(
                        left + (right - left) / 2.0f - textSize.x / 2.0f,
                        yOffsetBot + 5.0f
                    );

                    drawList->AddText(activeFont, fontSize, distPos, distanceColor, distStr.c_str());
                }

                ImGui::End();
            }
        }
        if (IsGameOnTop("Roblox") && Options::Misc::Watermark)
        {
            auto displaySize = io.DisplaySize;

            std::string str = "Atlas | " + Globals::Roblox::LocalPlayer.Name() + " | " + std::to_string(static_cast<int>(io.Framerate)) + " FPS";

            ImVec2 textSize = ImGui::CalcTextSize(str.c_str());

            ImVec2 pos = ImVec2(displaySize.x - textSize.x - 10.0f, 10.0f);

            ImDrawList* drawList = ImGui::GetBackgroundDrawList();
            ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
            ImU32 textColor = IM_COL32(152, 150, 214, 255);

            drawList->AddText(ImVec2(pos.x - 1, pos.y - 1), outlineColor, str.c_str());
            drawList->AddText(ImVec2(pos.x + 1, pos.y - 1), outlineColor, str.c_str());
            drawList->AddText(ImVec2(pos.x - 1, pos.y + 1), outlineColor, str.c_str());
            drawList->AddText(ImVec2(pos.x + 1, pos.y + 1), outlineColor, str.c_str());

            drawList->AddText(pos, textColor, str.c_str());
        }

        if (Options::ESP::Box || Options::ESP::Skeleton || Options::ESP::Tracers || Options::ESP::Name ||
            Options::ESP::Distance || Options::ESP::Health || Options::ESP::ESP3D)
        {
            if (!showMenu && IsGameOnTop("Roblox"))
                RenderESP(ImGui::GetBackgroundDrawList());
        }

        if (Options::Aimbot::Aimbot)
        {
            if (!showMenu && IsGameOnTop("Roblox"))
                RunAimbot(ImGui::GetBackgroundDrawList());
        }

        if (Options::Arsenal::RapidFire)
        {
            if (!showMenu && IsGameOnTop("Roblox"))
                ArsenalRapidFire();
        }

        if (Options::HitboxExpander::Expand)
        {
            if (!showMenu && IsGameOnTop("Roblox"))
            {
                RunHitBoxExpand();
            }
        }

        if (Options::Misc::Headless)
        {
            auto local_player = Globals::Roblox::LocalPlayer.Character();
            auto head = local_player.FindFirstChild("Head");
            auto face_decal = head.FindFirstChild("Face");
            head.SetTransparency(1.f);
            face_decal.SetTransparency(1.f);
        }

        if (Options::Misc::Fly)
        {
            if (!showMenu && IsGameOnTop("Roblox"))
                Fly();
        }

        if (Options::Misc::Streamproof)
        {
            SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
        }
        else
        {
            SetWindowDisplayAffinity(hwnd, WDA_NONE);
        }
        

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 4;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
