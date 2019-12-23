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

// variables needed for assembly file
extern "C" LPVOID* jmp_addr = (LPVOID*)0;
extern "C" float speed = 5;
extern "C" LPVOID* ret_speed = (LPVOID*)0;

// assembly file functions
extern "C" void my_jump();
extern "C" void speed_hack();

// hook definitions
typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
static EndScene oEndScene = NULL;
static WNDPROC oWndProc = nullptr;

// random value to use for dll import with CFF Explorer (or any other PE editor)
extern "C" __declspec(dllexport) bool my_import = true;

// values to edit render state (viewable with console)
extern "C" __declspec(dllexport) uint8_t t1 = 8;
extern "C" __declspec(dllexport) uint8_t t2 = 2;
extern "C" __declspec(dllexport) uint8_t t1_prev = 1;
extern "C" __declspec(dllexport) uint8_t t2_prev = 4;
D3DRENDERSTATETYPE r_state = (D3DRENDERSTATETYPE)t1;
DWORD r_value = t2;
static int change = 1;
static bool update = false;
static bool t1_update = false;
static bool t2_update = false;
static bool change_update = false;

// addresses (base, instructions, data)
static LPVOID* base = (LPVOID*)GetModuleHandle(NULL);
static LPVOID* addr;
static LPVOID* addr_spell_change;
static LPVOID* addr_player_1;
static uint8_t* addr_player_1_spell;
static LPVOID* addr_player_2;
static uint8_t* addr_player_2_spell;
static int* money;
static LPVOID* addr_speed;

// writing to memory
static DWORD procID;
static HANDLE handle;

// overwriting instructions
static std::uint8_t nop_code[] {0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
static std::uint8_t jmp_code[6] = {0xE9,0x00,0x00,0x00,0x00,0x00};
static std::uint8_t org_code[6];
static std::uint8_t jmp_speed[5] = {0xE9,0x00,0x00,0x00,0x00};
static std::uint8_t org_speed[5];

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
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
      }
      // O
      else if(wParam == 0x4f)
      {
        // WriteProcessMemory(handle, addr, &nop_code, sizeof(org_code), NULL);
        WriteProcessMemory(handle, addr, &jmp_code, sizeof(jmp_code), NULL);
      }
      // P
      else if(wParam == 0x50)
      {
        WriteProcessMemory(handle, addr, &org_code, sizeof(org_code), NULL);
      }
      // [S] key to set options
      else if(wParam == 0x53)
      {
        update=true;
      }
      else if(wParam == VK_UP)
      {
        t1+=change;
        update=true;
        t1_update=true;
      }
      else if(wParam == VK_DOWN)
      {
        t1-=change;
        update=true;
        t1_update=true;
      }
      else if(wParam == VK_LEFT)
      {
        t2-=change;
        update=true;
        t2_update=true;
      }
      else if(wParam == VK_RIGHT)
      {
        t2+=change;
        update=true;
        t2_update=true;
      }
      else if(wParam == 0x31)
      {
        change=1;
        update=true;
        change_update=true;
      }
      else if(wParam == 0x32)
      {
        change=8;
        update=true;
        change_update=true;
      }
      else if(wParam == 0x33)
      {
        change=16;
        update=true;
        change_update=true;
      }
      else if(wParam == 0x34)
      {
        change=32;
        update=true;
        change_update=true;
      }
      else if(wParam == 0x35)
      {
        change=64;
        update=true;
        change_update=true;
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

static bool speed_hack_set = false;
static bool speed_hack_set_prev = speed_hack_set;

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


    addr = (LPVOID*) ( ((unsigned long)base) + 0x1fc08);
    jmp_addr = (LPVOID*) ( ((unsigned long)addr) + 0x6  );
    addr_spell_change = (LPVOID*) (((unsigned long)base)+0xE493B);
    addr_player_1 = (LPVOID*) (((unsigned long)base)+0xB71DA0);
    addr_player_1_spell = (uint8_t*)(((unsigned long)*addr_player_1)+0x12c4);
    addr_player_2 = (LPVOID*) (((unsigned long)base)+0xB71DA4);
    addr_player_2_spell = (uint8_t*)(((unsigned long)*addr_player_1)+0x12c4);

    money = (int*) (*((LPVOID*)(((unsigned long)base)+0xB7181C)));

    // speed hack
    addr_speed = (LPVOID*) (((unsigned long)base) + 0x10C223);
    ret_speed = (LPVOID*) (((unsigned long)addr_speed) + 0x7);
    memcpy(org_speed, addr_speed, sizeof(org_speed));

    offset = abs((int)&speed_hack - (int)addr_speed) - 5;
    
    jmp_speed[1] = offset & 0xFF;
    jmp_speed[2] = (offset >> 8) & 0xFF;
    jmp_speed[3] = (offset >> 16) & 0xFF;
    jmp_speed[4] = (offset >> 24) & 0xFF;
    

    offset = abs((int)addr-(int)&my_jump)-5;

    jmp_code[4] = (offset >> 24) & 0xFF;
    jmp_code[3] = (offset >> 16) & 0xFF;
    jmp_code[2] = (offset >> 8) & 0xFF;
    jmp_code[1] = offset & 0xFF;


    memcpy(org_code, addr, sizeof(org_code));

    for(uint8_t val: org_code)
      printf("%02X ",val);
    std::cout << std::endl;
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
    ImGui::StyleColorsDark();
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
    addr_player_1_spell = (uint8_t*)(((unsigned long)*addr_player_1)+0x12c4);
    addr_player_2_spell = (uint8_t*)(((unsigned long)*addr_player_2)+0x12c4);

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
    ImGui::RadioButton("Player 1", &player, 1);
    ImGui::RadioButton("Player 2", &player, 2);

    // show current spell
    if(!init_players)
      ImGui::Text("Current Spell: ???");
    else if(player==1)
      ImGui::Text("Current Spell: %d", *addr_player_1_spell);
    else
      ImGui::Text("Current Spell: %d", *addr_player_2_spell);

    // money
    if(money)
    {
      ImGui::InputInt("Money", money);
      ImGui::SameLine(); HelpMarker("You can apply arithmetic operators +,*,/ on numerical values.\n  e.g. [ 100 ], input \'*2\', result becomes [ 200 ]\nUse +- to subtract.\n");
    }
    else
    {
      money = (int*) (*((LPVOID*)(((unsigned long)base)+0xB7181C)));
      ImGui::Text("Money: ???");
    }

    // speed hack
    ImGui::Checkbox("Speed Hack", &speed_hack_set);
    ImGui::InputFloat("Speed", &speed, 0.01f, 1.0f, "%.3f");
    

    ImGui::End();

    // demo window to test imgui widgets
    ImGui::ShowDemoWindow();

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

  // set fillmode according to cheat menu radio button (POINT, WIREFRAME, SOLID)
  pDevice->SetRenderState(D3DRS_FILLMODE, fillmode);

  // console info for setting render states
  if(update)
  {

    update=false;
    // clear display
    // documents/programming/ansi/ASCII Table - ANSI Escape sequences.html
    printf("\033[2J");
    printf("\033[H");

    if(t1_update)
      printf("change: %d\nstate: \033[1;30;107m%d\033[0m\nvalue: %d\n", change, t1, t2);
    else if(t2_update)
      printf("change: %d\nstate: %d\nvalue: \033[1;30;107m%d\033[0m\n", change, t1, t2);
    else if(change_update)
      printf("change: \033[1;30;107m%d\033[0m\nstate: %d\nvalue: %d\n", change, t1, t2);
    else
    {
      printf("change: %d\nstate: %d\nvalue: %d\n", change, t1, t2);

      r_state = (D3DRENDERSTATETYPE)t1;
      r_value = t2;

      pDevice->SetRenderState(r_state, r_value);
    }

    t1_update=false;
    t2_update=false;
    change_update=false;

  }

  return oEndScene(pDevice);

}

int main()
{

  if(kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success)
  {
    kiero::bind(42, (void**)&oEndScene, hkEndScene);
  }

  return 0;

}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID)
{

  DisableThreadLibraryCalls(hInstance);

  switch (fdwReason)
  {
    case DLL_PROCESS_ATTACH:
      CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, NULL, 0, NULL);
      break;
  }

  return true;

}