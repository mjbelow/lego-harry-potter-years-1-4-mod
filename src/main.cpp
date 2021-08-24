// directx hooking
#include "kiero.h"
#include <d3d9.h>

// debugging
#include <stdio.h>
#include <iostream>

// imgui menu
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#include <math.h>

const float pi = 3.14159265358979323846264;
const float my_pi = 32768.0;
int reverse = -1;

// variables needed for assembly file
extern "C" LPVOID* jmp_addr = (LPVOID*)(size_t)0;
extern "C" float speed = 5;
extern "C" LPVOID* ret_speed = (LPVOID*)(size_t)0;
extern "C" float jump = 1.9;
extern "C" LPVOID* ret_jump = (LPVOID*)(size_t)0;
extern "C" float gravity = .0001;
extern "C" LPVOID* ret_gravity = (LPVOID*)(size_t)0;
extern "C" int tank = 0;
extern "C" LPVOID* ret_tank_controls = (LPVOID*)(size_t)0;
extern "C" int pitch = 0;
extern "C" LPVOID* ret_camera_pitch = (LPVOID*)(size_t)0;
extern "C" int yaw = 0;
extern "C" LPVOID* ret_camera_yaw = (LPVOID*)(size_t)0;
extern "C" float adjust_position = 1.0;
extern "C" float adjust_height = 0;
extern "C" int adjust_rotate = 0;
extern "C" int adjust_rotate_prev = adjust_rotate;
extern "C" LPVOID* ret_camera_position_x = (LPVOID*)(size_t)0;
extern "C" LPVOID* ret_camera_position_y = (LPVOID*)(size_t)0;
extern "C" LPVOID* ret_camera_position_z = (LPVOID*)(size_t)0;
extern "C" LPVOID* ret_all_access = (LPVOID*)(size_t)0;

int sensitivity = 0;

// functions need for assembly file
extern "C" __declspec(dllexport) float x_cam(uint16_t rotate, float x)
{

    rotate += adjust_rotate;
    return x+(cos(rotate/my_pi*pi)/adjust_position)*reverse;

}

extern "C" __declspec(dllexport) float y_cam(uint16_t rotate, float y)
{

    rotate += adjust_rotate;
    return y+(sin(rotate/my_pi*pi)/adjust_position)*reverse;

}

extern "C" __declspec(dllexport) float test_x = 0;
extern "C" __declspec(dllexport) float test_y = 0;

extern "C" __declspec(dllexport) void f_x()
{

    test_x = x_cam(16383, 4.0);

}

extern "C" __declspec(dllexport) void f_y()
{

    test_y = y_cam(16383, 4.0);

}

// assembly file functions
extern "C" void my_jump();
extern "C" void speed_hack();
extern "C" void jump_hack();
extern "C" void gravity_hack();
extern "C" void tank_controls();
extern "C" void camera_pitch();
extern "C" void camera_yaw();
extern "C" void camera_position_x();
extern "C" void camera_position_y();
extern "C" void camera_position_z();
extern "C" void all_access();

// hook definitions
typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
static EndScene oEndScene = NULL;
static WNDPROC oWndProc = nullptr;

// random value to use for dll import with CFF Explorer (or any other PE editor)
extern "C" __declspec(dllexport) bool my_import = true;

// addresses (base, instructions, data)
extern "C" LPVOID* base = (LPVOID*)(size_t)GetModuleHandle(NULL);
static LPVOID* addr;
static LPVOID* addr_spell_change;
extern "C" __declspec(dllexport) LPVOID* addr_player_1 = (LPVOID*)(size_t)0;
static uint8_t* addr_player_1_spell;
extern "C" __declspec(dllexport) LPVOID* addr_player_2 = (LPVOID*)(size_t)0;
static uint8_t* addr_player_2_spell;
static unsigned long* money;
static LPVOID* addr_speed;
static LPVOID* addr_jump;
static LPVOID* addr_gravity;
static LPVOID* addr_tank_controls;
static LPVOID* addr_camera_pitch;
static LPVOID* addr_camera_yaw;
static LPVOID* addr_camera_position_x;
static LPVOID* addr_camera_position_y;
static LPVOID* addr_camera_position_z;
static LPVOID* addr_all_access;
static LPVOID* addr_adult_check;

// writing to memory
static DWORD procID;
static HANDLE handle;

// overwriting instructions
static std::uint8_t nop_code[] {0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
static std::uint8_t jmp_speed[5] = {0xE9,0x00,0x00,0x00,0x00};
static std::uint8_t org_speed[5];
static std::uint8_t jmp_jump[5] = {0xE9,0x00,0x00,0x00,0x00};
static std::uint8_t org_jump[5];
static std::uint8_t jmp_gravity[5] = {0xE9,0x00,0x00,0x00,0x00};
static std::uint8_t org_gravity[5];
static std::uint8_t jmp_tank_controls[5] = {0xE9,0x00,0x00,0x00,0x00};
static std::uint8_t org_tank_controls[5];
static std::uint8_t jmp_camera_pitch[5] = {0xE9,0x00,0x00,0x00,0x00};
static std::uint8_t org_camera_pitch[5];
static std::uint8_t jmp_camera_yaw[5] = {0xE9,0x00,0x00,0x00,0x00};
static std::uint8_t org_camera_yaw[5];
static std::uint8_t jmp_camera_position_x[5] = {0xE9,0x00,0x00,0x00,0x00};
static std::uint8_t org_camera_position_x[5];
static std::uint8_t jmp_camera_position_y[5] = {0xE9,0x00,0x00,0x00,0x00};
static std::uint8_t org_camera_position_y[5];
static std::uint8_t jmp_camera_position_z[5] = {0xE9,0x00,0x00,0x00,0x00};
static std::uint8_t org_camera_position_z[5];
static std::uint8_t jmp_all_access[5] = {0xE9,0x00,0x00,0x00,0x00};
static std::uint8_t org_all_access[5];
static std::uint8_t asm_adult_check[6] = {0x0F,0x85,0x7F,0x02,0x00,0x00};
static std::uint8_t asm_bypass_adult_check[6] = {0xE9,0x80,0x02,0x00,0x00,0x90};

// initializing
static bool init_handle = false;
static bool init_addresses = false;
static bool init_imgui = false;

// activating menu
static bool pressed = false;
static bool menu_activated = false;

// resetting device
static LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS g_d3dpp = {};

void ResetDevice()
{

    ImGui_ImplDX9_InvalidateDeviceObjects();
    g_pd3dDevice->Reset(&g_d3dpp);
    ImGui_ImplDX9_CreateDeviceObjects();

}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK hWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    ImGuiIO& io = ImGui::GetIO();

    if(menu_activated && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    if(!init_handle)
    {

        init_handle = true;

        // get handle to write to the process's memory
        GetWindowThreadProcessId(hWnd, &procID);
        handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);

    }

    switch (msg)
    {

        case WM_DESTROY:
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            break;

        case WM_SIZE:
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
            break;

        case WM_KEYUP:
            if (wParam == VK_SHIFT)
            {
                pressed=false;
            }
            else if (wParam == 0x51)
            {
                io.MouseDown[0]=false;
            }
            break;

        case WM_KEYDOWN:
            // [SHIFT] key to toggle cheat menu
            if (wParam == VK_SHIFT && !pressed)
            {
                pressed=true;
                menu_activated = !menu_activated;
                io.MouseDrawCursor = menu_activated;
            }
            else if(wParam == 0x51)
            {
                io.MouseDown[0]=true;
            }
            // [ESC] to quickly exit the app
            else if(wParam == VK_ESCAPE)
            {
                // ImGui_ImplDX9_Shutdown();
                // ImGui_ImplWin32_Shutdown();
            }
    }
    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);

}

HHOOK hMouseHook;
LRESULT CALLBACK mouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{

    ImGuiIO& io = ImGui::GetIO();

    switch(wParam)
    {
        case 513:
            io.MouseDown[0]=true;
            break;
        case 514:
            io.MouseDown[0]=false;
            break;
    }

    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);

}
HHOOK MouseHook;


static int fillmode = 3;
static bool freeze_spells = false;
static bool freeze_spells_prev = freeze_spells;
static uint8_t orig_code_addr_spell_change[] = {0x88, 0x87, 0xC4, 0x12, 0x00, 0x00};
static int player = 1;
static int select_spell;
static int spell_placeholder = 0;

static const ImU8 spell_step = 1;
static const ImU64 money_step = 1000;

static bool speed_hack_set = false;
static bool speed_hack_set_prev = speed_hack_set;

static bool jump_hack_set = false;
static bool jump_hack_set_prev = jump_hack_set;

static bool gravity_hack_set = false;
static bool gravity_hack_set_prev = gravity_hack_set;

static bool tank_controls_set = false;
static bool tank_controls_set_prev = tank_controls_set;

static bool camera_pitch_set = false;
static bool camera_pitch_set_prev = camera_pitch_set;

static bool camera_yaw_set = false;
static bool camera_yaw_set_prev = camera_yaw_set;

static bool camera_position_set = false;
static bool camera_position_set_prev = camera_position_set;

static bool reverse_set = false;
static bool reverse_set_prev = reverse_set;

static bool all_access_set = false;
static bool all_access_set_prev = all_access_set;

static bool bypass_adult_check_set = false;
static bool bypass_adult_check_set_prev = bypass_adult_check_set;

static bool print_debug_info = true;
static bool init_players = false;

D3DDEVICE_CREATION_PARAMETERS deviceParams;

static void HelpMarker(const char* desc)
{

    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

}

long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{

    if(!init_addresses)
    {
        init_addresses = true;

        int offset;

        // position console window for debugging
        HWND consoleWindow = GetConsoleWindow();
        SetWindowPos(consoleWindow, 0, 2000, 500, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
        g_d3dpp.Windowed = TRUE;
        g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
        g_d3dpp.EnableAutoDepthStencil = TRUE;
        g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
        g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;


        addr = (LPVOID*)(size_t) ( ((unsigned long)(size_t)base) + 0x1fc08);
        jmp_addr = (LPVOID*)(size_t) ( ((unsigned long)(size_t)addr) + 0x6    );
        addr_spell_change = (LPVOID*)(size_t) (((unsigned long)(size_t)base)+0xE493B);
        addr_player_1 = (LPVOID*)(size_t) (((unsigned long)(size_t)base)+0xB71DA0);
        addr_player_1_spell = (uint8_t*)(size_t)(((unsigned long)(size_t)*addr_player_1)+0x12c4);
        addr_player_2 = (LPVOID*)(size_t) (((unsigned long)(size_t)base)+0xB71DA4);
        addr_player_2_spell = (uint8_t*)(size_t)(((unsigned long)(size_t)*addr_player_1)+0x12c4);

        money = (unsigned long*) (*((LPVOID*)(size_t)(((unsigned long)(size_t)base)+0xB7181C)));

        // speed hack
        addr_speed = (LPVOID*)(size_t) (((unsigned long)(size_t)base) + 0x10C223);
        ret_speed = (LPVOID*)(size_t) (((unsigned long)(size_t)addr_speed) + 0x7);
        memcpy(org_speed, addr_speed, sizeof(org_speed));

        offset = abs((int)(size_t)&speed_hack - (int)(size_t)addr_speed) - 5;

        jmp_speed[1] = offset & 0xFF;
        jmp_speed[2] = (offset >> 8) & 0xFF;
        jmp_speed[3] = (offset >> 16) & 0xFF;
        jmp_speed[4] = (offset >> 24) & 0xFF;

        // jump hack
        addr_jump = (LPVOID*)(size_t) (((unsigned long)(size_t)base) + 0x41CD10);
        ret_jump = (LPVOID*)(size_t) (((unsigned long)(size_t)addr_jump) + 0x9);
        memcpy(org_jump, addr_jump, 5);

        offset = abs((int)(size_t)&jump_hack - (int)(size_t)addr_jump) - 5;

        jmp_jump[1] = offset & 0xFF;
        jmp_jump[2] = (offset >> 8) & 0xFF;
        jmp_jump[3] = (offset >> 16) & 0xFF;
        jmp_jump[4] = (offset >> 24) & 0xFF;

        // gravity hack
        addr_gravity = (LPVOID*)(size_t) (((unsigned long)(size_t)base) + 0xFC017);
        ret_gravity = (LPVOID*)(size_t) (((unsigned long)(size_t)addr_gravity) + 0x6);
        memcpy(org_gravity, addr_gravity, 5);

        offset = abs((int)(size_t)&gravity_hack - (int)(size_t)addr_gravity) - 5;

        jmp_gravity[1] = offset & 0xFF;
        jmp_gravity[2] = (offset >> 8) & 0xFF;
        jmp_gravity[3] = (offset >> 16) & 0xFF;
        jmp_gravity[4] = (offset >> 24) & 0xFF;

        // tank controls (1st person camera)
        addr_tank_controls = (LPVOID*)(size_t) (((unsigned long)(size_t)base) + 0x38A985);
        ret_tank_controls = (LPVOID*)(size_t) (((unsigned long)(size_t)addr_tank_controls) + 0x6);
        memcpy(org_tank_controls, addr_tank_controls, 5);

        offset = abs((int)(size_t)&tank_controls - (int)(size_t)addr_tank_controls) - 5;

        jmp_tank_controls[1] = offset & 0xFF;
        jmp_tank_controls[2] = (offset >> 8) & 0xFF;
        jmp_tank_controls[3] = (offset >> 16) & 0xFF;
        jmp_tank_controls[4] = (offset >> 24) & 0xFF;

        // camera pitch (1st person camera)
        addr_camera_pitch = (LPVOID*)(size_t) (((unsigned long)(size_t)base) + 0xDA15);
        ret_camera_pitch = (LPVOID*)(size_t) (((unsigned long)(size_t)addr_camera_pitch) + 0x6);
        memcpy(org_camera_pitch, addr_camera_pitch, 5);

        offset = abs((int)(size_t)&camera_pitch - (int)(size_t)addr_camera_pitch) - 5;

        jmp_camera_pitch[1] = offset & 0xFF;
        jmp_camera_pitch[2] = (offset >> 8) & 0xFF;
        jmp_camera_pitch[3] = (offset >> 16) & 0xFF;
        jmp_camera_pitch[4] = (offset >> 24) & 0xFF;

        // camera yaw (1st person camera)
        addr_camera_yaw = (LPVOID*)(size_t) (((unsigned long)(size_t)base) + 0xDA09);
        ret_camera_yaw = (LPVOID*)(size_t) (((unsigned long)(size_t)addr_camera_yaw) + 0x6);
        memcpy(org_camera_yaw, addr_camera_yaw, 5);

        offset = abs((int)(size_t)&camera_yaw - (int)(size_t)addr_camera_yaw) - 5;

        jmp_camera_yaw[1] = offset & 0xFF;
        jmp_camera_yaw[2] = (offset >> 8) & 0xFF;
        jmp_camera_yaw[3] = (offset >> 16) & 0xFF;
        jmp_camera_yaw[4] = (offset >> 24) & 0xFF;

        // camera position x (1st person camera)
        addr_camera_position_x = (LPVOID*)(size_t) (((unsigned long)(size_t)base) + 0xBB25);
        ret_camera_position_x = (LPVOID*)(size_t) (((unsigned long)(size_t)addr_camera_position_x) + 0x6);
        memcpy(org_camera_position_x, addr_camera_position_x, 5);

        offset = abs((int)(size_t)&camera_position_x - (int)(size_t)addr_camera_position_x) - 5;

        jmp_camera_position_x[1] = offset & 0xFF;
        jmp_camera_position_x[2] = (offset >> 8) & 0xFF;
        jmp_camera_position_x[3] = (offset >> 16) & 0xFF;
        jmp_camera_position_x[4] = (offset >> 24) & 0xFF;

        // camera position y (1st person camera)
        addr_camera_position_y = (LPVOID*)(size_t) (((unsigned long)(size_t)base) + 0xBAF7);
        ret_camera_position_y = (LPVOID*)(size_t) (((unsigned long)(size_t)addr_camera_position_y) + 0x6);
        memcpy(org_camera_position_y, addr_camera_position_y, 5);

        offset = abs((int)(size_t)&camera_position_y - (int)(size_t)addr_camera_position_y) - 5;

        jmp_camera_position_y[1] = offset & 0xFF;
        jmp_camera_position_y[2] = (offset >> 8) & 0xFF;
        jmp_camera_position_y[3] = (offset >> 16) & 0xFF;
        jmp_camera_position_y[4] = (offset >> 24) & 0xFF;

        // camera position z (1st person camera)
        addr_camera_position_z = (LPVOID*)(size_t) (((unsigned long)(size_t)base) + 0xBB0E);
        ret_camera_position_z = (LPVOID*)(size_t) (((unsigned long)(size_t)addr_camera_position_z) + 0x6);
        memcpy(org_camera_position_z, addr_camera_position_z, 5);

        offset = abs((int)(size_t)&camera_position_z - (int)(size_t)addr_camera_position_z) - 5;

        jmp_camera_position_z[1] = offset & 0xFF;
        jmp_camera_position_z[2] = (offset >> 8) & 0xFF;
        jmp_camera_position_z[3] = (offset >> 16) & 0xFF;
        jmp_camera_position_z[4] = (offset >> 24) & 0xFF;

        // all access
        addr_all_access = (LPVOID*)(size_t) (((unsigned long)(size_t)base) + 0xFE791F);
        ret_all_access = (LPVOID*)(size_t) (((unsigned long)(size_t)addr_all_access) + 7);
        memcpy(org_all_access, addr_all_access, 5);

        offset = abs((int)(size_t)&all_access - (int)(size_t)addr_all_access) - 5;

        jmp_all_access[1] = offset & 0xFF;
        jmp_all_access[2] = (offset >> 8) & 0xFF;
        jmp_all_access[3] = (offset >> 16) & 0xFF;
        jmp_all_access[4] = (offset >> 24) & 0xFF;

        // adult check
        addr_adult_check = (LPVOID*)(size_t) (((unsigned long)(size_t)base) + 0x2DC4BA);

    }

    if (!init_imgui)
    {
        init_imgui = true;

        g_pd3dDevice = pDevice;

        // hook WndProc
        pDevice->GetCreationParameters(&deviceParams);
        oWndProc = (WNDPROC)SetWindowLongPtr(deviceParams.hFocusWindow, GWLP_WNDPROC, (LONG_PTR)hWndProc);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(deviceParams.hFocusWindow);
        ImGui_ImplDX9_Init(pDevice);

        // get low-level mouse input so we can use the mouse to interact with the imgui menu (because game doesn't supply a mouse whose event we can't intercept with hWndProc)
        MouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseProc, NULL, 0);
    }

    // make sure cheat menu always gets rendered with solid fill
    pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

    // Start the Dear ImGui frame
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // if player addresses are initialized in game, get addresses of data associated with the player addresses (keep executing this code in case you change players)
    if(addr_player_1 && addr_player_2 && *addr_player_1 && *addr_player_2)
    {

        init_players = true;
        addr_player_1_spell = (uint8_t*)(size_t)(((unsigned long)(size_t)*addr_player_1)+0x12c4);
        addr_player_2_spell = (uint8_t*)(size_t)(((unsigned long)(size_t)*addr_player_2)+0x12c4);

    }

    if(menu_activated)
    {

        // render cheat menu
        ImGui::Begin("Cheat Menu");
        ImGui::Text("Fill Mode");
        ImGui::RadioButton("POINT", &fillmode, 1); ImGui::SameLine();
        ImGui::RadioButton("WIREFRAME", &fillmode, 2); ImGui::SameLine();
        ImGui::RadioButton("SOLID", &fillmode, 3);
        ImGui::Checkbox("Freeze Red Spells", &freeze_spells);
        ImGui::Checkbox("All Access", &all_access_set);
        ImGui::SameLine();
        ImGui::Checkbox("Bypass Adult Check", &bypass_adult_check_set);
        ImGui::RadioButton("Player 1", &player, 1);
        ImGui::RadioButton("Player 2", &player, 2);

        // adjust spell
        if(init_players)
        {
            if(player==1)
                ImGui::InputScalar("Spell", ImGuiDataType_U8, addr_player_1_spell, &spell_step);
            else
                ImGui::InputScalar("Spell", ImGuiDataType_U8, addr_player_2_spell, &spell_step);
        }
        else
        {
            ImGui::InputInt("Spell", &spell_placeholder);
        }

        // money
        if(money)
        {
            ImGui::InputScalar("Money", ImGuiDataType_U64, money, &money_step);
        }
        else
        {
            money = (unsigned long*) (*((LPVOID*)(size_t)(((unsigned long)(size_t)base)+0xB7181C)));
            ImGui::Text("Money: ???");
        }

        // speed hack
        ImGui::Checkbox("Speed Hack", &speed_hack_set);
        ImGui::InputFloat("Speed", &speed, 0.1f, 1.0f, "%.6f");

        // speed hack
        ImGui::Checkbox("Jump Hack", &jump_hack_set);
        ImGui::InputFloat("Jump", &jump, 0.1f, 1.0f, "%.6f");

        // gravity hack
        ImGui::Checkbox("Gravity Hack", &gravity_hack_set);
        gravity *= 1000;
        ImGui::InputFloat("Gravity", &gravity, 0.1f, 1.0f, "%.6f");
        gravity /= 1000;

        if(ImGui::Button("Set Camera"))
        {
            tank_controls_set=true;
            camera_pitch_set=true;
            camera_yaw_set=true;
            camera_position_set=true;
        }

        ImGui::SameLine();

        if(ImGui::Button("Clear Camera"))
        {
            tank_controls_set=false;
            camera_pitch_set=false;
            camera_yaw_set=false;
            camera_position_set=false;
        }

        // tank controls
        ImGui::Checkbox("Tank Controls", &tank_controls_set);
        ImGui::SliderInt("Tank", &tank, -65536, 65536);
        tank %= 65536;
        ImGui::SameLine(); HelpMarker("CTRL+click to input value.");

        // camera pitch
        ImGui::Checkbox("Camera Pitch", &camera_pitch_set);
        ImGui::SliderInt("Pitch", &pitch, -65536, 65536);
        pitch %= 65536;
        ImGui::SameLine(); HelpMarker("CTRL+click to input value.");

        // camera yaw
        ImGui::Checkbox("Camera Yaw", &camera_yaw_set);
        ImGui::SliderInt("Yaw", &yaw, -65536, 65536);
        yaw %= 65536;
        ImGui::SameLine(); HelpMarker("CTRL+click to input value.");

        // camera position
        ImGui::Checkbox("Camera Position", &camera_position_set);
        ImGui::SameLine(); ImGui::Checkbox("Reverse", &reverse_set);
        ImGui::SliderInt("Sensitivity", &sensitivity, 0, 6);
        ImGui::SliderFloat("Distance", &adjust_position, 0, (pow(2,sensitivity)));
        ImGui::SameLine(); HelpMarker("CTRL+click to input value.");
        ImGui::SliderFloat("Height", &adjust_height, -(pow(2,sensitivity))/2, (pow(2,sensitivity))/2);
        ImGui::SameLine(); HelpMarker("CTRL+click to input value.");
        ImGui::SliderInt("Rotation", &adjust_rotate, -65536, 65536);
        adjust_rotate %= 65536;
        ImGui::SameLine(); HelpMarker("CTRL+click to input value.");

        // automatically adjust tank controls based off of camera rotation
        if(adjust_rotate != adjust_rotate_prev)
        {
            adjust_rotate_prev = adjust_rotate;
            tank = adjust_rotate * -1;

            if(reverse_set)
            {
                if(adjust_rotate < 0)
                    yaw = -32768 - adjust_rotate;
                else
                    yaw = 32768 - adjust_rotate;
            }
            else
            {
                if(adjust_rotate < 0)
                    yaw = -65536 - adjust_rotate;
                else
                    yaw = 65536 - adjust_rotate;
            }
        }

        if(ImGui::Button("Crash Game"))
        {
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
        }

        ImGui::End();

        // demo window to test imgui widgets
        // ImGui::ShowDemoWindow();

        // print debug info when menu is activated
        if(print_debug_info)
        {
            print_debug_info = false;
        }

    }
    else
    {
        print_debug_info = true;
    }

    // Rendering
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    if(freeze_spells != freeze_spells_prev)
    {
        freeze_spells_prev = freeze_spells;
        if(freeze_spells)
            WriteProcessMemory(handle, addr_spell_change, &nop_code, 6, NULL);
        else
            WriteProcessMemory(handle, addr_spell_change, &orig_code_addr_spell_change, 6, NULL);
    }

    if(speed_hack_set != speed_hack_set_prev)
    {
        speed_hack_set_prev = speed_hack_set;
        if(speed_hack_set)
            WriteProcessMemory(handle, addr_speed, &jmp_speed, 5, NULL);
        else
            WriteProcessMemory(handle, addr_speed, &org_speed, 5, NULL);
    }

    if(jump_hack_set != jump_hack_set_prev)
    {
        jump_hack_set_prev = jump_hack_set;
        if(jump_hack_set)
            WriteProcessMemory(handle, addr_jump, &jmp_jump, 5, NULL);
        else
            WriteProcessMemory(handle, addr_jump, &org_jump, 5, NULL);
    }

    if(gravity_hack_set != gravity_hack_set_prev)
    {
        gravity_hack_set_prev = gravity_hack_set;
        if(gravity_hack_set)
            WriteProcessMemory(handle, addr_gravity, &jmp_gravity, 5, NULL);
        else
            WriteProcessMemory(handle, addr_gravity, &org_gravity, 5, NULL);
    }

    if(tank_controls_set != tank_controls_set_prev)
    {
        tank_controls_set_prev = tank_controls_set;
        if(tank_controls_set)
            WriteProcessMemory(handle, addr_tank_controls, &jmp_tank_controls, 5, NULL);
        else
            WriteProcessMemory(handle, addr_tank_controls, &org_tank_controls, 5, NULL);
    }

    if(camera_pitch_set != camera_pitch_set_prev)
    {
        camera_pitch_set_prev = camera_pitch_set;
        if(camera_pitch_set)
            WriteProcessMemory(handle, addr_camera_pitch, &jmp_camera_pitch, 5, NULL);
        else
            WriteProcessMemory(handle, addr_camera_pitch, &org_camera_pitch, 5, NULL);
    }

    if(camera_yaw_set != camera_yaw_set_prev)
    {
        camera_yaw_set_prev = camera_yaw_set;
        if(camera_yaw_set)
            WriteProcessMemory(handle, addr_camera_yaw, &jmp_camera_yaw, 5, NULL);
        else
            WriteProcessMemory(handle, addr_camera_yaw, &org_camera_yaw, 5, NULL);
    }

    if(camera_position_set != camera_position_set_prev)
    {
        camera_position_set_prev = camera_position_set;
        if(camera_position_set)
        {
            WriteProcessMemory(handle, addr_camera_position_x, &jmp_camera_position_x, 5, NULL);
            WriteProcessMemory(handle, addr_camera_position_y, &jmp_camera_position_y, 5, NULL);
            WriteProcessMemory(handle, addr_camera_position_z, &jmp_camera_position_z, 5, NULL);
        }
        else
        {
            WriteProcessMemory(handle, addr_camera_position_x, &org_camera_position_x, 5, NULL);
            WriteProcessMemory(handle, addr_camera_position_y, &org_camera_position_y, 5, NULL);
            WriteProcessMemory(handle, addr_camera_position_z, &org_camera_position_z, 5, NULL);
        }
    }

    if(reverse_set != reverse_set_prev)
    {
        reverse_set_prev = reverse_set;
        if(reverse_set)
        {
            reverse = 1;

            if(adjust_rotate < 0)
                yaw = -32768 - adjust_rotate;
            else
                yaw = 32768 - adjust_rotate;
        }
        else
        {
            reverse = -1;

            if(adjust_rotate < 0)
                yaw = -65536 - adjust_rotate;
            else
                yaw = 65536 - adjust_rotate;
        }
    }

    if(all_access_set != all_access_set_prev)
    {
        all_access_set_prev = all_access_set;
        if(all_access_set)
            WriteProcessMemory(handle, addr_all_access, &jmp_all_access, 5, NULL);
        else
            WriteProcessMemory(handle, addr_all_access, &org_all_access, 5, NULL);
    }

    if(bypass_adult_check_set != bypass_adult_check_set_prev)
    {
        bypass_adult_check_set_prev = bypass_adult_check_set;
        if(bypass_adult_check_set)
            WriteProcessMemory(handle, addr_adult_check, &asm_bypass_adult_check, 6, NULL);
        else
            WriteProcessMemory(handle, addr_adult_check, &asm_adult_check, 6, NULL);
    }

    // set fillmode according to cheat menu radio button (POINT, WIREFRAME, SOLID)
    // default is "3" (solid), so prevent calling this when it's set to solid
    if (fillmode != 3)
    {
        pDevice->SetRenderState(D3DRS_FILLMODE, fillmode);
    }


    return oEndScene(pDevice);

}

void __stdcall main_entry()
{

    if(kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success)
    {
        kiero::bind(42, (void**)&oEndScene, (void*)hkEndScene);
    }

}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID)
{

    DisableThreadLibraryCalls(hInstance);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main_entry, NULL, 0, NULL);
            break;
    }

    return true;

}