#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#include "kiero.h"
#include <Windows.h>
#include <assert.h>
#include <iostream>

#if KIERO_INCLUDE_D3D9
# include <d3d9.h>
#endif

#if KIERO_INCLUDE_D3D10
# include <dxgi.h>
# include <d3d10_1.h>
# include <d3d10.h>
#endif

#if KIERO_INCLUDE_D3D11
# include <dxgi.h>
# include <d3d11.h>
#endif

#if KIERO_INCLUDE_D3D12
# include <dxgi.h>
# include <d3d12.h>
#endif

#if KIERO_INCLUDE_OPENGL
# include <gl/GL.h>
#endif

#if KIERO_INCLUDE_VULKAN
# include <vulkan/vulkan.h>
#endif

#if KIERO_USE_MINHOOK
# include "MinHook.h"
#endif

#ifdef _UNICODE
# define KIERO_TEXT(text) L##text
#else
# define KIERO_TEXT(text) text
#endif

#define KIERO_ARRAY_SIZE(arr) ((size_t)(sizeof(arr)/sizeof(arr[0])))

static kiero::RenderType::Enum g_renderType = kiero::RenderType::None;
static uint150_t* g_methodsTable = NULL;

extern "C" void my_jump();

kiero::Status::Enum kiero::init(RenderType::Enum _renderType)
{
	if (g_renderType != RenderType::None)
	{
		return Status::AlreadyInitializedError;
	}

	if (_renderType != RenderType::None)
	{
		if (_renderType >= RenderType::D3D9 && _renderType <= RenderType::D3D12)
		{
			WNDCLASSEX windowClass;
			windowClass.cbSize = sizeof(WNDCLASSEX);
			windowClass.style = CS_HREDRAW | CS_VREDRAW;
			windowClass.lpfnWndProc = DefWindowProc;
			windowClass.cbClsExtra = 0;
			windowClass.cbWndExtra = 0;
			windowClass.hInstance = GetModuleHandle(NULL);
			windowClass.hIcon = NULL;
			windowClass.hCursor = NULL;
			windowClass.hbrBackground = NULL;
			windowClass.lpszMenuName = NULL;
			windowClass.lpszClassName = KIERO_TEXT("Kiero");
			windowClass.hIconSm = NULL;

			::RegisterClassEx(&windowClass);

			HWND window = ::CreateWindow(windowClass.lpszClassName, KIERO_TEXT("Kiero DirectX Window"), WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

			if (_renderType == RenderType::D3D9)
			{
#if KIERO_INCLUDE_D3D9
				HMODULE libD3D9;
				if ((libD3D9 = ::GetModuleHandle(KIERO_TEXT("d3d9.dll"))) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::ModuleNotFoundError;
				}

				void* Direct3DCreate9;
				if ((Direct3DCreate9 = ::GetProcAddress(libD3D9, "Direct3DCreate9")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				LPDIRECT3D9 direct3D9;
				if ((direct3D9 = ((LPDIRECT3D9(__stdcall*)(uint32_t))(Direct3DCreate9))(D3D_SDK_VERSION)) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				D3DDISPLAYMODE displayMode;
				if (direct3D9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &displayMode) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				D3DPRESENT_PARAMETERS params;
				params.BackBufferWidth = 0;
				params.BackBufferHeight = 0;
				params.BackBufferFormat = displayMode.Format;
				params.BackBufferCount = 0;
				params.MultiSampleType = D3DMULTISAMPLE_NONE;
				params.MultiSampleQuality = NULL;
				params.SwapEffect = D3DSWAPEFFECT_DISCARD;
				params.hDeviceWindow = window;
				params.Windowed = 1;
				params.EnableAutoDepthStencil = 0;
				params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
				params.Flags = NULL;
				params.FullScreen_RefreshRateInHz = 0;
				params.PresentationInterval = 0;

				LPDIRECT3DDEVICE9 device;
				if (direct3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &params, &device) < 0)
				{
					direct3D9->Release();
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				g_methodsTable = (uint150_t*)::calloc(119, sizeof(uint150_t));
				::memcpy(g_methodsTable, *(uint150_t**)device, 119 * sizeof(uint150_t));

#if KIERO_USE_MINHOOK
				MH_Initialize();
#endif

				direct3D9->Release();
				direct3D9 = NULL;

				device->Release();
				device = NULL;

				g_renderType = RenderType::D3D9;

				::DestroyWindow(window);
				::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

				return Status::Success;
#endif
			}
			else if (_renderType == RenderType::D3D10)
			{
#if KIERO_INCLUDE_D3D10
				HMODULE libDXGI;
				HMODULE libD3D10;
				if ((libDXGI = ::GetModuleHandle(KIERO_TEXT("dxgi.dll"))) == NULL || (libD3D10 = ::GetModuleHandle(KIERO_TEXT("d3d10.dll"))) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::ModuleNotFoundError;
				}

				void* CreateDXGIFactory;
				if ((CreateDXGIFactory = ::GetProcAddress(libDXGI, "CreateDXGIFactory")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				IDXGIFactory* factory;
				if (((long(__stdcall*)(const IID&, void**))(CreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&factory) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				IDXGIAdapter* adapter;
				if (factory->EnumAdapters(0, &adapter) == DXGI_ERROR_NOT_FOUND)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				void* D3D10CreateDeviceAndSwapChain;
				if ((D3D10CreateDeviceAndSwapChain = ::GetProcAddress(libD3D10, "D3D10CreateDeviceAndSwapChain")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				DXGI_RATIONAL refreshRate;
				refreshRate.Numerator = 60;
				refreshRate.Denominator = 1;

				DXGI_MODE_DESC bufferDesc;
				bufferDesc.Width = 100;
				bufferDesc.Height = 100;
				bufferDesc.RefreshRate = refreshRate;
				bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

				DXGI_SAMPLE_DESC sampleDesc;
				sampleDesc.Count = 1;
				sampleDesc.Quality = 0;

				DXGI_SWAP_CHAIN_DESC swapChainDesc;
				swapChainDesc.BufferDesc = bufferDesc;
				swapChainDesc.SampleDesc = sampleDesc;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferCount = 1;
				swapChainDesc.OutputWindow = window;
				swapChainDesc.Windowed = 1;
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
				swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

				IDXGISwapChain* swapChain;
				ID3D10Device* device;

				if (((long(__stdcall*)(
					IDXGIAdapter*,
					D3D10_DRIVER_TYPE,
					HMODULE,
					UINT,
					UINT,
					DXGI_SWAP_CHAIN_DESC*,
					IDXGISwapChain**,
					ID3D10Device**))(D3D10CreateDeviceAndSwapChain))(adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &swapChainDesc, &swapChain, &device) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				g_methodsTable = (uint150_t*)::calloc(116, sizeof(uint150_t));
				::memcpy(g_methodsTable, *(uint150_t**)swapChain, 18 * sizeof(uint150_t));
				::memcpy(g_methodsTable + 18, *(uint150_t**)device, 98 * sizeof(uint150_t));

#if KIERO_USE_MINHOOK
				MH_Initialize();
#endif

				swapChain->Release();
				swapChain = NULL;

				device->Release();
				device = NULL;

				::DestroyWindow(window);
				::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

				g_renderType = RenderType::D3D10;

				return Status::Success;
#endif
			}
			else if (_renderType == RenderType::D3D11)
			{
#if KIERO_INCLUDE_D3D11
				HMODULE libD3D11;
				if ((libD3D11 = ::GetModuleHandle(KIERO_TEXT("d3d11.dll"))) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::ModuleNotFoundError;
				}

				void* D3D11CreateDeviceAndSwapChain;
				if ((D3D11CreateDeviceAndSwapChain = ::GetProcAddress(libD3D11, "D3D11CreateDeviceAndSwapChain")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				D3D_FEATURE_LEVEL featureLevel;
				const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0 };

				DXGI_RATIONAL refreshRate;
				refreshRate.Numerator = 60;
				refreshRate.Denominator = 1;

				DXGI_MODE_DESC bufferDesc;
				bufferDesc.Width = 100;
				bufferDesc.Height = 100;
				bufferDesc.RefreshRate = refreshRate;
				bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

				DXGI_SAMPLE_DESC sampleDesc;
				sampleDesc.Count = 1;
				sampleDesc.Quality = 0;

				DXGI_SWAP_CHAIN_DESC swapChainDesc;
				swapChainDesc.BufferDesc = bufferDesc;
				swapChainDesc.SampleDesc = sampleDesc;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferCount = 1;
				swapChainDesc.OutputWindow = window;
				swapChainDesc.Windowed = 1;
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
				swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

				IDXGISwapChain* swapChain;
				ID3D11Device* device;
				ID3D11DeviceContext* context;

				if (((long(__stdcall*)(
					IDXGIAdapter*,
					D3D_DRIVER_TYPE,
					HMODULE,
					UINT,
					const D3D_FEATURE_LEVEL*,
					UINT,
					UINT,
					const DXGI_SWAP_CHAIN_DESC*,
					IDXGISwapChain**,
					ID3D11Device**,
					D3D_FEATURE_LEVEL*,
					ID3D11DeviceContext**))(D3D11CreateDeviceAndSwapChain))(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 1, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, &featureLevel, &context) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				g_methodsTable = (uint150_t*)::calloc(205, sizeof(uint150_t));
				::memcpy(g_methodsTable, *(uint150_t**)swapChain, 18 * sizeof(uint150_t));
				::memcpy(g_methodsTable + 18, *(uint150_t**)device, 43 * sizeof(uint150_t));
				::memcpy(g_methodsTable + 18 + 43, *(uint150_t**)context, 144 * sizeof(uint150_t));

#if KIERO_USE_MINHOOK
				MH_Initialize();
#endif

				swapChain->Release();
				swapChain = NULL;

				device->Release();
				device = NULL;

				context->Release();
				context = NULL;

				::DestroyWindow(window);
				::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

				g_renderType = RenderType::D3D11;

				return Status::Success;
#endif
			}
			else if (_renderType == RenderType::D3D12)
			{
#if KIERO_INCLUDE_D3D12
				HMODULE libDXGI;
				HMODULE libD3D12;
				if ((libDXGI = ::GetModuleHandle(KIERO_TEXT("dxgi.dll"))) == NULL || (libD3D12 = ::GetModuleHandle(KIERO_TEXT("d3d12.dll"))) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::ModuleNotFoundError;
				}

				void* CreateDXGIFactory;
				if ((CreateDXGIFactory = ::GetProcAddress(libDXGI, "CreateDXGIFactory")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				IDXGIFactory* factory;
				if (((long(__stdcall*)(const IID&, void**))(CreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&factory) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				IDXGIAdapter* adapter;
				if (factory->EnumAdapters(0, &adapter) == DXGI_ERROR_NOT_FOUND)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				void* D3D12CreateDevice;
				if ((D3D12CreateDevice = ::GetProcAddress(libD3D12, "D3D12CreateDevice")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				ID3D12Device* device;
				if (((long(__stdcall*)(IUnknown*, D3D_FEATURE_LEVEL, const IID&, void**))(D3D12CreateDevice))(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				D3D12_COMMAND_QUEUE_DESC queueDesc;
				queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
				queueDesc.Priority = 0;
				queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
				queueDesc.NodeMask = 0;

				ID3D12CommandQueue* commandQueue;
				if (device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void**)&commandQueue) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				ID3D12CommandAllocator* commandAllocator;
				if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&commandAllocator) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				ID3D12GraphicsCommandList* commandList;
				if (device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&commandList) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				DXGI_RATIONAL refreshRate;
				refreshRate.Numerator = 60;
				refreshRate.Denominator = 1;

				DXGI_MODE_DESC bufferDesc;
				bufferDesc.Width = 100;
				bufferDesc.Height = 100;
				bufferDesc.RefreshRate = refreshRate;
				bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

				DXGI_SAMPLE_DESC sampleDesc;
				sampleDesc.Count = 1;
				sampleDesc.Quality = 0;

				DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
				swapChainDesc.BufferDesc = bufferDesc;
				swapChainDesc.SampleDesc = sampleDesc;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferCount = 2;
				swapChainDesc.OutputWindow = window;
				swapChainDesc.Windowed = 1;
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
				swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

				IDXGISwapChain* swapChain;
				if (factory->CreateSwapChain(commandQueue, &swapChainDesc, &swapChain) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				g_methodsTable = (uint150_t*)::calloc(150, sizeof(uint150_t));
				::memcpy(g_methodsTable, *(uint150_t**)device, 44 * sizeof(uint150_t));
				::memcpy(g_methodsTable + 44, *(uint150_t**)commandQueue, 19 * sizeof(uint150_t));
				::memcpy(g_methodsTable + 44 + 19, *(uint150_t**)commandAllocator, 9 * sizeof(uint150_t));
				::memcpy(g_methodsTable + 44 + 19 + 9, *(uint150_t**)commandList, 60 * sizeof(uint150_t));
				::memcpy(g_methodsTable + 44 + 19 + 9 + 60, *(uint150_t**)swapChain, 18 * sizeof(uint150_t));

#if KIERO_USE_MINHOOK
				MH_Initialize();
#endif

				device->Release();
				device = NULL;

				commandQueue->Release();
				commandQueue = NULL;

				commandAllocator->Release();
				commandAllocator = NULL;

				commandList->Release();
				commandList = NULL;

				swapChain->Release();
				swapChain = NULL;

				::DestroyWindow(window);
				::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

				g_renderType = RenderType::D3D12;

				return Status::Success;
#endif
			}

			::DestroyWindow(window);
			::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

			return Status::NotSupportedError;
		}
		else if (_renderType == RenderType::OpenGL)
		{
#if KIERO_INCLUDE_OPENGL
			HMODULE libOpenGL32;
			if ((libOpenGL32 = ::GetModuleHandle(KIERO_TEXT("opengl32.dll"))) == NULL)
			{
				return Status::ModuleNotFoundError;
			}

			const char* const methodsNames[] = {
				"glAccum", "glAlphaFunc", "glAreTexturesResident", "glArrayElement", "glBegin", "glBindTexture", "glBitmap", "glBlendFunc", "glCallList", "glCallLists", "glClear", "glClearAccum",
				"glClearColor", "glClearDepth", "glClearIndex", "glClearStencil", "glClipPlane", "glColor3b", "glColor3bv", "glColor3d", "glColor3dv", "glColor3f", "glColor3fv", "glColor3i", "glColor3iv",
				"glColor3s", "glColor3sv", "glColor3ub", "glColor3ubv", "glColor3ui", "glColor3uiv", "glColor3us", "glColor3usv", "glColor4b", "glColor4bv", "glColor4d", "glColor4dv", "glColor4f",
				"glColor4fv", "glColor4i", "glColor4iv", "glColor4s", "glColor4sv", "glColor4ub", "glColor4ubv", "glColor4ui", "glColor4uiv", "glColor4us", "glColor4usv", "glColorMask", "glColorMaterial",
				"glColorPointer", "glCopyPixels", "glCopyTexImage1D", "glCopyTexImage2D", "glCopyTexSubImage1D", "glCopyTexSubImage2D", "glCullFaceglCullFace", "glDeleteLists", "glDeleteTextures",
				"glDepthFunc", "glDepthMask", "glDepthRange", "glDisable", "glDisableClientState", "glDrawArrays", "glDrawBuffer", "glDrawElements", "glDrawPixels", "glEdgeFlag", "glEdgeFlagPointer",
				"glEdgeFlagv", "glEnable", "glEnableClientState", "glEnd", "glEndList", "glEvalCoord1d", "glEvalCoord1dv", "glEvalCoord1f", "glEvalCoord1fv", "glEvalCoord2d", "glEvalCoord2dv",
				"glEvalCoord2f", "glEvalCoord2fv", "glEvalMesh1", "glEvalMesh2", "glEvalPoint1", "glEvalPoint2", "glFeedbackBuffer", "glFinish", "glFlush", "glFogf", "glFogfv", "glFogi", "glFogiv",
				"glFrontFace", "glFrustum", "glGenLists", "glGenTextures", "glGetBooleanv", "glGetClipPlane", "glGetDoublev", "glGetError", "glGetFloatv", "glGetIntegerv", "glGetLightfv", "glGetLightiv",
				"glGetMapdv", "glGetMapfv", "glGetMapiv", "glGetMaterialfv", "glGetMaterialiv", "glGetPixelMapfv", "glGetPixelMapuiv", "glGetPixelMapusv", "glGetPointerv", "glGetPolygonStipple",
				"glGetString", "glGetTexEnvfv", "glGetTexEnviv", "glGetTexGendv", "glGetTexGenfv", "glGetTexGeniv", "glGetTexImage", "glGetTexLevelParameterfv", "glGetTexLevelParameteriv",
				"glGetTexParameterfv", "glGetTexParameteriv", "glHint", "glIndexMask", "glIndexPointer", "glIndexd", "glIndexdv", "glIndexf", "glIndexfv", "glIndexi", "glIndexiv", "glIndexs", "glIndexsv",
				"glIndexub", "glIndexubv", "glInitNames", "glInterleavedArrays", "glIsEnabled", "glIsList", "glIsTexture", "glLightModelf", "glLightModelfv", "glLightModeli", "glLightModeliv", "glLightf",
				"glLightfv", "glLighti", "glLightiv", "glLineStipple", "glLineWidth", "glListBase", "glLoadIdentity", "glLoadMatrixd", "glLoadMatrixf", "glLoadName", "glLogicOp", "glMap1d", "glMap1f",
				"glMap2d", "glMap2f", "glMapGrid1d", "glMapGrid1f", "glMapGrid2d", "glMapGrid2f", "glMaterialf", "glMaterialfv", "glMateriali", "glMaterialiv", "glMatrixMode", "glMultMatrixd",
				"glMultMatrixf", "glNewList", "glNormal3b", "glNormal3bv", "glNormal3d", "glNormal3dv", "glNormal3f", "glNormal3fv", "glNormal3i", "glNormal3iv", "glNormal3s", "glNormal3sv",
				"glNormalPointer", "glOrtho", "glPassThrough", "glPixelMapfv", "glPixelMapuiv", "glPixelMapusv", "glPixelStoref", "glPixelStorei", "glPixelTransferf", "glPixelTransferi", "glPixelZoom",
				"glPointSize", "glPolygonMode", "glPolygonOffset", "glPolygonStipple", "glPopAttrib", "glPopClientAttrib", "glPopMatrix", "glPopName", "glPrioritizeTextures", "glPushAttrib",
				"glPushClientAttrib", "glPushMatrix", "glPushName", "glRasterPos2d", "glRasterPos2dv", "glRasterPos2f", "glRasterPos2fv", "glRasterPos2i", "glRasterPos2iv", "glRasterPos2s",
				"glRasterPos2sv", "glRasterPos3d", "glRasterPos3dv", "glRasterPos3f", "glRasterPos3fv", "glRasterPos3i", "glRasterPos3iv", "glRasterPos3s", "glRasterPos3sv", "glRasterPos4d",
				"glRasterPos4dv", "glRasterPos4f", "glRasterPos4fv", "glRasterPos4i", "glRasterPos4iv", "glRasterPos4s", "glRasterPos4sv", "glReadBuffer", "glReadPixels", "glRectd", "glRectdv", "glRectf",
				"glRectfv", "glRecti", "glRectiv", "glRects", "glRectsv", "glRenderMode", "glRotated", "glRotatef", "glScaled", "glScalef", "glScissor", "glSelectBuffer", "glShadeModel", "glStencilFunc",
				"glStencilMask", "glStencilOp", "glTexCoord1d", "glTexCoord1dv", "glTexCoord1f", "glTexCoord1fv", "glTexCoord1i", "glTexCoord1iv", "glTexCoord1s", "glTexCoord1sv", "glTexCoord2d",
				"glTexCoord2dv", "glTexCoord2f", "glTexCoord2fv", "glTexCoord2i", "glTexCoord2iv", "glTexCoord2s", "glTexCoord2sv", "glTexCoord3d", "glTexCoord3dv", "glTexCoord3f", "glTexCoord3fv",
				"glTexCoord3i", "glTexCoord3iv", "glTexCoord3s", "glTexCoord3sv", "glTexCoord4d", "glTexCoord4dv", "glTexCoord4f", "glTexCoord4fv", "glTexCoord4i", "glTexCoord4iv", "glTexCoord4s",
				"glTexCoord4sv", "glTexCoordPointer", "glTexEnvf", "glTexEnvfv", "glTexEnvi", "glTexEnviv", "glTexGend", "glTexGendv", "glTexGenf", "glTexGenfv", "glTexGeni", "glTexGeniv", "glTexImage1D",
				"glTexImage2D", "glTexParameterf", "glTexParameterfv", "glTexParameteri", "glTexParameteriv", "glTexSubImage1D", "glTexSubImage2D", "glTranslated", "glTranslatef", "glVertex2d",
				"glVertex2dv", "glVertex2f", "glVertex2fv", "glVertex2i", "glVertex2iv", "glVertex2s", "glVertex2sv", "glVertex3d", "glVertex3dv", "glVertex3f", "glVertex3fv", "glVertex3i", "glVertex3iv",
				"glVertex3s", "glVertex3sv", "glVertex4d", "glVertex4dv", "glVertex4f", "glVertex4fv", "glVertex4i", "glVertex4iv", "glVertex4s", "glVertex4sv", "glVertexPointer", "glViewport"
			};

			size_t size = KIERO_ARRAY_SIZE(methodsNames);

			g_methodsTable = (uint150_t*)::calloc(size, sizeof(uint150_t));

			for (int i = 0; i < size; i++)
			{
				g_methodsTable[i] = (uint150_t)::GetProcAddress(libOpenGL32, methodsNames[i]);
			}

#if KIERO_USE_MINHOOK
			MH_Initialize();
#endif

			g_renderType = RenderType::OpenGL;

			return Status::Success;
#endif
		}
		else if (_renderType == RenderType::Vulkan)
		{
#if KIERO_INCLUDE_VULKAN
			HMODULE libVulkan;
			if ((libVulkan = GetModuleHandle(KIERO_TEXT("vulcan-1.dll"))) == NULL)
			{
				return Status::ModuleNotFoundError;
			}

			const char* const methodsNames[] = {
				"vkCreateInstance", "vkDestroyInstance", "vkEnumeratePhysicalDevices", "vkGetPhysicalDeviceFeatures", "vkGetPhysicalDeviceFormatProperties", "vkGetPhysicalDeviceImageFormatProperties",
				"vkGetPhysicalDeviceProperties", "vkGetPhysicalDeviceQueueFamilyProperties", "vkGetPhysicalDeviceMemoryProperties", "vkGetInstanceProcAddr", "vkGetDeviceProcAddr", "vkCreateDevice",
				"vkDestroyDevice", "vkEnumerateInstanceExtensionProperties", "vkEnumerateDeviceExtensionProperties", "vkEnumerateDeviceLayerProperties", "vkGetDeviceQueue", "vkQueueSubmit", "vkQueueWaitIdle",
				"vkDeviceWaitIdle", "vkAllocateMemory", "vkFreeMemory", "vkMapMemory", "vkUnmapMemory", "vkFlushMappedMemoryRanges", "vkInvalidateMappedMemoryRanges", "vkGetDeviceMemoryCommitment",
				"vkBindBufferMemory", "vkBindImageMemory", "vkGetBufferMemoryRequirements", "vkGetImageMemoryRequirements", "vkGetImageSparseMemoryRequirements", "vkGetPhysicalDeviceSparseImageFormatProperties",
				"vkQueueBindSparse", "vkCreateFence", "vkDestroyFence", "vkResetFences", "vkGetFenceStatus", "vkWaitForFences", "vkCreateSemaphore", "vkDestroySemaphore", "vkCreateEvent", "vkDestroyEvent",
				"vkGetEventStatus", "vkSetEvent", "vkResetEvent", "vkCreateQueryPool", "vkDestroyQueryPool", "vkGetQueryPoolResults", "vkCreateBuffer", "vkDestroyBuffer", "vkCreateBufferView", "vkDestroyBufferView",
				"vkCreateImage", "vkDestroyImage", "vkGetImageSubresourceLayout", "vkCreateImageView", "vkDestroyImageView", "vkCreateShaderModule", "vkDestroyShaderModule", "vkCreatePipelineCache",
				"vkDestroyPipelineCache", "vkGetPipelineCacheData", "vkMergePipelineCaches", "vkCreateGraphicsPipelines", "vkCreateComputePipelines", "vkDestroyPipeline", "vkCreatePipelineLayout",
				"vkDestroyPipelineLayout", "vkCreateSampler", "vkDestroySampler", "vkCreateDescriptorSetLayout", "vkDestroyDescriptorSetLayout", "vkCreateDescriptorPool", "vkDestroyDescriptorPool",
				"vkResetDescriptorPool", "vkAllocateDescriptorSets", "vkFreeDescriptorSets", "vkUpdateDescriptorSets", "vkCreateFramebuffer", "vkDestroyFramebuffer", "vkCreateRenderPass", "vkDestroyRenderPass",
				"vkGetRenderAreaGranularity", "vkCreateCommandPool", "vkDestroyCommandPool", "vkResetCommandPool", "vkAllocateCommandBuffers", "vkFreeCommandBuffers", "vkBeginCommandBuffer", "vkEndCommandBuffer",
				"vkResetCommandBuffer", "vkCmdBindPipeline", "vkCmdSetViewport", "vkCmdSetScissor", "vkCmdSetLineWidth", "vkCmdSetDepthBias", "vkCmdSetBlendConstants", "vkCmdSetDepthBounds",
				"vkCmdSetStencilCompareMask", "vkCmdSetStencilWriteMask", "vkCmdSetStencilReference", "vkCmdBindDescriptorSets", "vkCmdBindIndexBuffer", "vkCmdBindVertexBuffers", "vkCmdDraw", "vkCmdDrawIndexed",
				"vkCmdDrawIndirect", "vkCmdDrawIndexedIndirect", "vkCmdDispatch", "vkCmdDispatchIndirect", "vkCmdCopyBuffer", "vkCmdCopyImage", "vkCmdBlitImage", "vkCmdCopyBufferToImage", "vkCmdCopyImageToBuffer",
				"vkCmdUpdateBuffer", "vkCmdFillBuffer", "vkCmdClearColorImage", "vkCmdClearDepthStencilImage", "vkCmdClearAttachments", "vkCmdResolveImage", "vkCmdSetEvent", "vkCmdResetEvent", "vkCmdWaitEvents",
				"vkCmdPipelineBarrier", "vkCmdBeginQuery", "vkCmdEndQuery", "vkCmdResetQueryPool", "vkCmdWriteTimestamp", "vkCmdCopyQueryPoolResults", "vkCmdPushConstants", "vkCmdBeginRenderPass", "vkCmdNextSubpass",
				"vkCmdEndRenderPass", "vkCmdExecuteCommands"
			};

			size_t size = KIERO_ARRAY_SIZE(methodsNames);

			g_methodsTable = (uint150_t*)::calloc(size, sizeof(uint150_t));

			for (int i = 0; i < size; i++)
			{
				g_methodsTable[i] = (uint150_t)::GetProcAddress(libVulkan, methodsNames[i]);
			}

#if KIERO_USE_MINHOOK
			MH_Initialize();
#endif

			g_renderType = RenderType::Vulkan;

			return Status::Success;
#endif
		}
		else if (_renderType == RenderType::Auto)
		{
			RenderType::Enum type = RenderType::None;

			if (::GetModuleHandle(KIERO_TEXT("d3d9.dll")) != NULL)
			{
				type = RenderType::D3D9;
			}
			else if (::GetModuleHandle(KIERO_TEXT("d3d10.dll")) != NULL)
			{
				type = RenderType::D3D10;
			}
			else if (::GetModuleHandle(KIERO_TEXT("d3d11.dll")) != NULL)
			{
				type = RenderType::D3D11;
			}
			else if (::GetModuleHandle(KIERO_TEXT("d3d12.dll")) != NULL)
			{
				type = RenderType::D3D12;
			}
			else if (::GetModuleHandle(KIERO_TEXT("opengl32.dll")) != NULL)
			{
				type = RenderType::OpenGL;
			}
			else if (::GetModuleHandle(KIERO_TEXT("vulcan-1.dll")) != NULL)
			{
				type = RenderType::Vulkan;
			}

			return init(type);
		}
	}

	return Status::Success;
}

void kiero::shutdown()
{
	if (g_renderType != RenderType::None)
	{
#if KIERO_USE_MINHOOK
		MH_DisableHook(MH_ALL_HOOKS);
#endif

		::free(g_methodsTable);
		g_methodsTable = NULL;
		g_renderType = RenderType::None;
	}
}

kiero::Status::Enum kiero::bind(uint16_t _index, void** _original, void* _function)
{
	// TODO: Need own detour function

	assert(_index >= 0 && _original != NULL && _function != NULL);

	if (g_renderType != RenderType::None)
	{
#if KIERO_USE_MINHOOK
		void* target = (void*)g_methodsTable[_index];
		if (MH_CreateHook(target, _function, _original) != MH_OK || MH_EnableHook(target) != MH_OK)
		{
			return Status::UnknownError;
		}
#endif

		return Status::Success;
	}

	return Status::NotInitializedError;
}

void kiero::unbind(uint16_t _index)
{
	assert(_index >= 0);

	if (g_renderType != RenderType::None)
	{
#if KIERO_USE_MINHOOK
		MH_DisableHook((void*)g_methodsTable[_index]);
#endif
	}
}

kiero::RenderType::Enum kiero::getRenderType()
{
	return g_renderType;
}

uint150_t* kiero::getMethodsTable()
{
	return g_methodsTable;
}

// Create the type of function that we will hook
typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
static EndScene oEndScene = NULL;

// __declspec(dllexport) bool update = true;
// random value to use for dll import with CFF Explorer (or any other PE editor)
extern "C" __declspec(dllexport) bool my_import = true;
extern "C" __declspec(dllexport) uint8_t t1 = 8;
extern "C" __declspec(dllexport) uint8_t t2 = 2;
extern "C" __declspec(dllexport) uint8_t t1_prev = 1;
extern "C" __declspec(dllexport) uint8_t t2_prev = 4;
D3DRENDERSTATETYPE r_state = D3DRS_FILLMODE;
DWORD r_value = 1;



static bool init = false;
static bool update = false;
static bool t1_update = false;
static bool t2_update = false;
static bool change_update = false;

static int change = 1;

#include <stdio.h>
#include <iostream>
// typedef char my_opcodes[40];

// static LPVOID* addr = (LPVOID*)(0x00400000+0x1fc08);
static LPVOID* addr;
static LPVOID* base;
static LPVOID* addr_spell_change;
static LPVOID* addr_player_1;
static uint8_t* addr_player_1_spell;
static LPVOID* addr_player_2;
static uint8_t* addr_player_2_spell;

// static my_opcodes* bytes = (my_opcodes*)0x00400000;
// static unsigned char* bytes = (unsigned char*)(0x00400000+0x1fc08);
static std::uint8_t* bytes = (std::uint8_t*)(addr);
static std::uint8_t bytes_arr[6];
static std::uint8_t bytes_arr2[6];
// static std::int8_t* bytes = (std::int8_t*)(0x00400000+0x1fc08);

static std::uint8_t ops[6] = {0x90,0x90,0x90,0x90,0x90,0x90};
// static std::uint8_t org[6] = {0x01,0x9E,0xF8,0xD5,0x01,0x00};
static std::uint8_t org[6];

static WNDPROC OriginalWndProcHandler = nullptr;

static DWORD procID;
static bool attached=false;
static HANDLE handle;

static std::uint8_t nop_code[] {0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
// static std::uint8_t jmp_code[6] = {0xE9,0x6F,0x4E,0xEE,0x6B,0x00};
// static std::uint8_t jmp_code[6] = {0xE9,0x6F,0x4E,0x9B,0x71,0x00};
static std::uint8_t jmp_code[6] = {0xE9,0x00,0x00,0x00,0x00,0x00};
static std::uint8_t org_code[6];

static bool initial=false;

static HMODULE process;

extern "C" LPVOID* jmp_addr = (LPVOID*)0;

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

void ResetDevice()
{
  ImGui_ImplDX9_InvalidateDeviceObjects();
  HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
  // if (hr == D3DERR_INVALIDCALL)
    // IM_ASSERT(0);
  ImGui_ImplDX9_CreateDeviceObjects();
}

bool show_demo_window = true;
bool pressed = false;
bool menu_activated = false;

// LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK hWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

  ImGuiIO& io = ImGui::GetIO();

  // if(menu_activated)
  // {
    // if(msg == WM_KEYDOWN)
    // {

      // if(wParam == VK_DELETE)
      // {
        // menu_activated=false;
        // pressed=true;
      // }
      // else if(wParam == 0x51)
      // {
        // io.MouseDown[0]=true;
      // }

    // }

    // ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    // return true;
  // }

  if(menu_activated && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;


  // if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    // return true;



  // return CallWindowProc(OriginalWndProcHandler, hWnd, msg, wParam, lParam);

  if(!initial)
  {

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    // g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate

    process = GetModuleHandle(NULL);

    // addr = (LPVOID*) ( ((DWORD)process) + 0x1fc08);
    base = (LPVOID*)process;
    // addr = (LPVOID*) ( ((unsigned long)process) + 0x1fc08);
    addr = (LPVOID*) ( ((unsigned long)base) + 0x1fc08);
    jmp_addr = (LPVOID*) ( ((unsigned long)addr) + 0x6  );
    addr_spell_change = (LPVOID*) (((unsigned long)base)+0xE493B);
    // addr_player_1 = (LPVOID*) (((unsigned long)*((LPVOID*) (((unsigned long)base)+0xB71DA0))) + 0x12C4);
    // addr_player_1 = (LPVOID*)(*(LPVOID*) (((unsigned long)base)+0xB71DA0));
    addr_player_1 = (LPVOID*) (((unsigned long)base)+0xB71DA0);
    // addr_player_1_spell = (LPVOID*) (((unsigned long)*addr_player_1)+0x12C4);
    addr_player_1_spell = (uint8_t*)(((unsigned long)*addr_player_1)+0x12c4);
    addr_player_2 = (LPVOID*) (((unsigned long)base)+0xB71DA4);
    // addr_player_2_spell = (LPVOID*) (((unsigned long)*addr_player_2)+0x12C4);
    addr_player_2_spell = (uint8_t*)(((unsigned long)*addr_player_1)+0x12c4);
    
    // if(addr_player_1_spell == nullptr)
      // std::cout << "player_1 is nullptr\n";
    // else
      // std::cout << "----------------------->" << *addr_player_1 << std::endl;
    // if(addr_player_1_spell == nullptr)
      // std::cout << "player_1 spell is nullptr\n";
    // else
      // std::cout << (unsigned long)addr_player_1_spell << std::endl;

    int offset = abs((int)addr-(int)&my_jump)-5;

    jmp_code[4] = (offset >> 24) & 0xFF;
    jmp_code[3] = (offset >> 16) & 0xFF;
    jmp_code[2] = (offset >> 8) & 0xFF;
    jmp_code[1] = offset & 0xFF;

    GetWindowThreadProcessId(hWnd, &procID);
    handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
    memcpy(org_code, addr, sizeof(org_code));

    for(uint8_t val: org_code)
      printf("%02X ",val);
    std::cout << std::endl;
    initial=true;

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

/*
      HMODULE hmodule = GetModuleHandle("d3d9.dll");
      // HMODULE process = GetModuleHandle("UltimateMagicCube.console.exe");
      TCHAR szExeFileName[MAX_PATH];
      GetModuleFileName(NULL, szExeFileName, MAX_PATH);
      // HMODULE process = GetModuleHandle(szExeFileName);
      HMODULE process = GetModuleHandle(NULL);

      printf("d3d9.dll = %X\n", hmodule);
      printf("%s = %X\n", szExeFileName, process);
*/


/*

      printf("%02x %02x %02x %02x %02x %02x\n", *bytes,*(bytes+1),*(bytes+2),*(bytes+3),*(bytes+4),*(bytes+5));
      // printf("%02x %02x %02x %02x %02x %02x\n", *bytes_arr,*(bytes_arr+1),*(bytes_arr+2),*(bytes_arr+3),*(bytes_arr+4),*(bytes_arr+5));
      for(uint8_t val: bytes_arr2)
        printf("%02X ",val);
      std::cout << std::endl;


      for(uint8_t val: bytes_arr)
        printf("%02X ",val);
      std::cout << std::endl;

      for(uint8_t val: ops)
        printf("%02X ",val);
      std::cout << std::endl;

      for(uint8_t val: org)
        printf("%02X ",val);
      std::cout << std::endl;

*/

      // PMEMORY_BASIC_INFORMATION lpBuffer;
      // MEMORY_BASIC_INFORMATION info;

      // VirtualQuery(addr, &info, 40);

      // access the buffer more than ONCE, causes it to crash
      // PVOID pv = lpBuffer->BaseAddress;

      // this would crash it even though it just worked ^
      // pv = lpBuffer->BaseAddress;


      // couldn't figure out how to change page protection, but figures it would just be best to use a haand with PROCESS_ALL_ACCESS, and writing memory with that
      // but if you change page protection through Cheat Engine for instance, *bytes=0x90 for instance would work without crashing the program
      // printf("Base Address: %x\n", info.BaseAddress);
      // printf("Allocation Base: %x\n", info.AllocationBase);
      // printf("Allocation Protect: %x\n", info.AllocationProtect);
      // printf("Region Size: %d\n", info.RegionSize);
      // printf("State: %x\n", info.State);
      // printf("Protect: %x\n", info.Protect);
      // printf("Type: %x\n", info.Type);

      // PDWORD oldProtect;
      // VirtualProtect(addr,info.RegionSize,PAGE_EXECUTE_WRITECOPY,oldProtect);

      else if( wParam == VK_ESCAPE ){

        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        // ImGui::DestroyContext();

        // CleanupDeviceD3D();
        // ::DestroyWindow(hwnd);
        // ::UnregisterClass(wc.lpszClassName, wc.hInstance);

        return CallWindowProc(OriginalWndProcHandler, hWnd, WM_DESTROY, wParam, lParam);
      }

      // R
      else if(wParam == 0x52)
      {
        ReadProcessMemory(handle, addr, &org, sizeof(org), NULL);
        memcpy(bytes_arr,addr,sizeof(bytes_arr));
      }
      // O
      else if(wParam == 0x4f)
      {
        WriteProcessMemory(handle, addr, &nop_code, sizeof(org_code), NULL);
        // WriteProcessMemory(handle, addr, &jmp_code, sizeof(jmp_code), NULL);


        // memcpy(addr, ops, sizeof(ops));
        // *bytes=0x90;
        // *(bytes+1)=0x90;
        // *(bytes+2)=0x90;
        // *(bytes+3)=0x90;
        // *(bytes+4)=0x90;
        // *(bytes+5)=0x90;
        // printf("%02x %02x %02x %02x %02x %02x", *bytes,*(bytes+1),*(bytes+2),*(bytes+3),*(bytes+4),*(bytes+5));
      }
      // P
      else if(wParam == 0x50)
      {
        WriteProcessMemory(handle, addr, &org_code, sizeof(org_code), NULL);
        // *bytes=0x01;
        // *(bytes+1)=0x9e;
        // *(bytes+2)=0xf8;
        // *(bytes+3)=0xd5;
        // *(bytes+4)=0x01;
        // *(bytes+5)=0x00;
        // printf("%02x %02x %02x %02x %02x %02x", *bytes,*(bytes+1),*(bytes+2),*(bytes+3),*(bytes+4),*(bytes+5));
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
  // return ::DefWindowProc(hWnd, msg, wParam, lParam);
  return CallWindowProc(OriginalWndProcHandler, hWnd, msg, wParam, lParam);
}




HHOOK hMouseHook;
HHOOK hMsgHook;
LRESULT CALLBACK mouseProc2(int nCode, WPARAM wParam, LPARAM lParam)
{
  // POINT xy = ((tagMOUSEHOOKSTRUCT*)lParam)->pt;
  // std::cout << "x: " << xy.x << " y: " << xy.y << std::endl;

  /*
  	MSLLHOOKSTRUCT * pMouseStruct = (MSLLHOOKSTRUCT *)lParam;
    			std::cout << "Mouse: x =  " << pMouseStruct->pt.x <<	"  y =  " << pMouseStruct->pt.y <<	std::endl;

  pMouseStruct->mouseData=WM_NULL;
  */
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

  // if(wParam == 513)
  // {
    // io.MouseDown[0]=true;
  // }
  // else if
    // io.MouseDown[0]=false;

  // std::cout << "MOUSE LOW LEVEL ---> " << wParam << std::endl;
  return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}
LRESULT CALLBACK mouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (true) // bool
    {
        ImGuiIO& io = ImGui::GetIO();
        MOUSEHOOKSTRUCT * pMouseStruct = (MOUSEHOOKSTRUCT *)lParam;
        if (pMouseStruct != NULL) {
            switch (wParam)
            {
            case WM_LBUTTONDOWN:
                io.MouseDown[0] = true;
                std::cout << "left mouse button clicked!!\n";
                break;
            case WM_LBUTTONUP:
                io.MouseDown[0] = false;
                io.MouseReleased[0] = true;
                break;
            case WM_RBUTTONDOWN:
                io.MouseDown[1] = true;
                break;
            case WM_RBUTTONUP:
                io.MouseDown[1] = false;
                io.MouseReleased[1] = true;
                break;
            }
        }
    }
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
    // return 1L;
}

LRESULT CALLBACK msgProc(int code, WPARAM wParam, LPARAM lParam)
{
  ImGuiIO& io = ImGui::GetIO();

  unsigned int msg = ((tagMSG*)lParam)->message;
  std::cout << "msg: " << msg << std::endl;
  // std::cout << ((tagMSG*)lParam)->message << std::endl;

  ((tagMSG*)lParam)->message=WM_NULL;
  if(msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
  {
    // ((tagMSG*)lParam)->message=WM_NULL;
    // return false;
  }

/*
  if(msg==513)
  {
    std::cout << "mousefirst" << WM_MOUSEFIRST << " mouselast" << WM_MOUSELAST << std::endl;
    ((tagMSG*)lParam)->message=WM_NULL;
    io.MouseDown[0]=true;
    std::cout<<"yay, you clicked mouse button left!!\n";
  }
  else if(msg==514)
  {
    ((tagMSG*)lParam)->message=WM_NULL;
    io.MouseDown[0]=false;
    std::cout<<"mouse button left UP!!\n";
  }
  */
  return CallNextHookEx(hMsgHook, code, wParam, lParam);
}
/*Hooking*/
HHOOK MouseHook;
HHOOK MsgHook;

// Our state
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

static int fillmode = 3;
static bool freeze_spells = false;
static bool freeze_spells_prev = freeze_spells;
static uint8_t orig_code_addr_spell_change[] = {0x88, 0x87, 0xC4, 0x12, 0x00, 0x00};
static int player = 1;
static int player_1_spell;
static int player_2_spell;

static bool print_debug_info = true;
static bool init_players = false;

// HCURSOR cursor = LoadCursor(GetModuleHandle(NULL), IDC_ARROW);
D3DDEVICE_CREATION_PARAMETERS deviceParams;
// Declare the detour function
long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
  // ... Your magic here ...
  // printf("this should NOT show!!!!!!\n");

  // SetCursor(cursor);
  // pDevice->ShowCursor(true);
  if (!init)
  {
    init = true;


    g_pd3dDevice = pDevice;

    // MessageBox(0, "Boom! It works!!!", "Kiero", MB_OK);

    pDevice->GetCreationParameters(&deviceParams);

    // OriginalWndProcHandler = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)hWndProc);
    OriginalWndProcHandler = (WNDPROC)SetWindowLongPtr(deviceParams.hFocusWindow, GWLP_WNDPROC, (LONG_PTR)hWndProc);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // io.MouseDrawCursor = true;
    // io.WantCaptureMouse = false;

      // Enabling ImGuiConfigFlags_NavEnableSetMousePos + ImGuiBackendFlags_HasSetMousePos instructs dear imgui to move your mouse cursor along with navigation movements.
      // When enabled, the NewFrame() function may alter 'io.MousePos' and set 'io.WantSetMousePos' to notify you that it wants the mouse cursor to be moved.
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableSetMousePos;
    // io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    // io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_RendererHasVtxOffset;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(deviceParams.hFocusWindow);
    ImGui_ImplDX9_Init(pDevice);
    // MouseHook = SetWindowsHookEx(WH_MOUSE, mouseProc, 0, GetCurrentThreadId());
    // MsgHook = SetWindowsHookEx(WH_GETMESSAGE, msgProc, 0, GetCurrentThreadId());
    // MouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseProc2, 0, GetCurrentThreadId());
    MouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseProc2, NULL, 0);
  }

  // make sure menu always gets rendered with solid fill
  pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

  // Start the Dear ImGui frame
  ImGui_ImplDX9_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  if(addr_player_1 && addr_player_2 && *addr_player_1 && *addr_player_2)
  // if(true)
  {
    // std::cout << "players inititialized" << std::endl;
    init_players = true;
    // std::cout << addr_player_1 << std::endl;
    // std::cout << addr_player_2 << std::endl;
    // std::cout << "--------DBG--------" << std::endl;
    // std::cout << addr_player_1 << std::endl;
    // std::cout << "--------DBG--------" << std::endl;
    // std::cout << *addr_player_1 << std::endl;
    // std::cout << "--------DBG--------" << std::endl;
    // std::cout << (uint8_t*)(((unsigned long)*addr_player_1)+0x12c4) << std::endl;
    // std::cout << "--------DBG--------" << std::endl;
    // std::cout << addr_player_2 << std::endl;
    // std::cout << "--------DBG--------" << std::endl;
    // std::cout << *addr_player_2 << std::endl;
    // std::cout << "--------DBG--------" << std::endl;
    // std::cout << (uint8_t*)(((unsigned long)*addr_player_2)+0x12c4) << std::endl;
    // std::cout << "--------DBG--------" << std::endl;
    addr_player_1_spell = (uint8_t*)(((unsigned long)*addr_player_1)+0x12c4);
    addr_player_2_spell = (uint8_t*)(((unsigned long)*addr_player_2)+0x12c4);
  }
  
  
  /*
    addr_player_1_spell = (uint8_t*)(((unsigned long)*addr_player_1)+0x12c4);
    addr_player_2 = (LPVOID*) (((unsigned long)base)+0xB71DA4);
    // addr_player_2_spell = (LPVOID*) (((unsigned long)*addr_player_2)+0x12C4);
    addr_player_2_spell = (uint8_t*)(((unsigned long)*addr_player_1)+0x12c4);
  */
  
  
  // if(*addr_player_1 == nullptr)
    // std::cout << "player_1 is nullptr\n";
  // else
    // std::cout << "----------------------->" << *addr_player_1 << std::endl;

  if(menu_activated)
  {

    ImGui::Begin("Cheat Menu");
    ImGui::Text("Fill Mode");
    ImGui::RadioButton("POINT", &fillmode, 1); ImGui::SameLine();
    ImGui::RadioButton("WIREFRAME", &fillmode, 2); ImGui::SameLine();
    ImGui::RadioButton("SOLID", &fillmode, 3);
    ImGui::Checkbox("Freeze Red Spells", &freeze_spells);
    ImGui::RadioButton("Player 1", &player, 1);
    ImGui::RadioButton("Player 2", &player, 2);
    // if(player==1)
      // ImGui::Text("Current Spell: %d", ((int)(*addr_player_1_spell))>>24 & 0xFF);
    // else
      // ImGui::Text("Current Spell: %d", ((int)(*addr_player_2_spell))>>24 & 0xFF);
    // if(player==1)
      // ImGui::Text("Current Spell: %d", *(int*)addr_player_1_spell);
    // else
      // ImGui::Text("Current Spell: %d", *(int*)addr_player_2_spell);
    // if(player==1)
      // ImGui::Text("Current Spell: %d", ((*((uint8_t*)(((unsigned long)*addr_player_1)+0x12c4)))));
    // else
      // ImGui::Text("Current Spell: %d", ((*((uint8_t*)(((unsigned long)*addr_player_2)+0x12c4)))&0xFF));
    
    if(!init_players)
    // if(true)
      ImGui::Text("Current Spell: ???");
    else if(player==1)
      ImGui::Text("Current Spell: %d", *addr_player_1_spell);
    else
      ImGui::Text("Current Spell: %d", *addr_player_2_spell);
    
    
    // if(player==1 && *addr_player_1_spell !=)
      // ImGui::Text("Current Spell: %d", *((uint8_t*)(((unsigned long)*addr_player_1)+0x12c4)));
    // else
      // ImGui::Text("Current Spell: %d", *addr_player_2_spell);
    
    ImGui::End();


// *((uint8_t*)(((unsigned long)*addr_player_1)+0x12c4))


    ImGui::ShowDemoWindow(&show_demo_window);
    
    if(print_debug_info)
    {
      print_debug_info = false;
      
      // addr_player_1_spell = (uint8_t*)(((unsigned long)*addr_player_1)+0x12c4);
      // std::cout << addr_player_1_spell << std::endl;
      
    // addr_player_1 = (LPVOID*) (((unsigned long)base)+0xB71DA0);
    // addr_player_1_spell = (LPVOID*) (((unsigned long)*addr_player_1)+0x12C4);
      // std::cout << "Player 1: " << *((int*)(((unsigned long)*addr_player_1)+0x12c4)) << std::endl;
      
      /*
      std::cout << "Player 1: " << addr_player_1 << std::endl;
      std::cout << "Player 1: " << *addr_player_1 << std::endl;
      // std::cout << "Player 1: " << ( *addr_player_1)+0x12c4 << std::endl;
      std::cout << "Player 1: " << ((unsigned long)*addr_player_1)+0x12c4 << std::endl;
      std::cout << "Player 1: " << *((uint8_t*)(((unsigned long)*addr_player_1)+0x12c4)) << std::endl;
      std::cout << "Player 1: " << ((uint8_t*)(((unsigned long)*addr_player_1)+0x12c4)) << std::endl;
      std::cout << "Player 1: " << ((uint8_t)(((unsigned long)*addr_player_1)+0x12c4)) << std::endl;
      std::cout << "Player 1: " << *((int*)(((unsigned long)*addr_player_1)+0x12c4)) << std::endl;
      std::cout << "Player 1: " << ((int*)(((unsigned long)*addr_player_1)+0x12c4)) << std::endl;
      std::cout << "Player 1: " << ((int)(((unsigned long)*addr_player_1)+0x12c4)) << std::endl;
      */
      
      
      
      // std::cout << "Player 1 Spell: " << addr_player_1_spell << std::endl;
      
      
      
      
      
      // std::cout << "Player 2: " << addr_player_2 << std::endl;
      // std::cout << "Player 2 Spell: " << addr_player_2_spell << std::endl;
      // std::cout << "Player 1: " << *addr_player_1 << std::endl;
      // std::cout << "Player 1 Spell: " << *addr_player_1_spell << std::endl;
      // std::cout << "Player 2: " << *addr_player_2 << std::endl;
      // std::cout << "Player 2 Spell: " << *addr_player_2_spell << std::endl;
      // std::cout << "Player 1 Spell: " << *(uint8_t*)addr_player_1_spell << std::endl;
      // std::cout << "Player 2 Spell: " << *(uint8_t*)addr_player_2_spell << std::endl;
      // std::cout << "Player 1: " << *addr_player_1 << std::endl;
      // std::cout << "Player 1 Spell: " << *addr_player_1_spell << std::endl;
      // std::cout << "Player 2: " << *addr_player_2 << std::endl;
      // std::cout << "Player 2 Spell: " << *addr_player_2_spell << std::endl;
      // std::cout << "Player 1: " << (int)*addr_player_1 << std::endl;
      // std::cout << "Player 1 Spell: " << (int)*addr_player_1_spell << std::endl;
      // std::cout << "Player 2: " << (int)*addr_player_2 << std::endl;
      // std::cout << "Player 2 Spell: " << (int)*addr_player_2_spell << std::endl;
      // std::cout << "Player 1 Spell: " << (uint8_t*)addr_player_1_spell << std::endl;
      // std::cout << "Player 2 Spell: " << (uint8_t*)addr_player_2_spell << std::endl;
    }
  }
  else
  {
    print_debug_info = true;
  }

  // Rendering
  ImGui::EndFrame();

  // pDevice->SetRenderState(D3DRS_ZENABLE, false);
  // pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
  // pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
  // D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x*255.0f), (int)(clear_color.y*255.0f), (int)(clear_color.z*255.0f), (int)(clear_color.w*255.0f));
  // pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);

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

  pDevice->SetRenderState(D3DRS_FILLMODE, fillmode);

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

    // Sleep(500);
    // if(GetAsyncKeyState(VK_SHIFT))
      // Sleep(100);
    // else if(GetAsyncKeyState(VK_SHIFT) && GetAsyncKeyState(VK_CONTROL))
      // Sleep(250);
  }

  return oEndScene(pDevice);
}

int kieroExampleThread()
{

  std::cout << "am i in this thread????\n";


  if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success)
  // or
  // if (kiero::init(kiero::RenderType::Auto) == kiero::Status::Success)
  {
    std::cout << "probably didn't make it here\n";
    // define KIERO_USE_MINHOOK must be 1
    // the index of the required function can be found in the METHODSTABLE.txt
    kiero::bind(42, (void**)&oEndScene, hkEndScene);

    // If you just need to get the function address you can use the kiero::getMethodsTable function
    //oEndScene = (EndScene)kiero::getMethodsTable()[42];

    // printf("%X\n%X\n", oEndScene, &oEndScene);
  }

  return 0;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID)
{
  HWND consoleWindow = GetConsoleWindow();
  SetWindowPos( consoleWindow, 0, 2000, 500, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );	

  // std::cout << "did i at least get here?\n";

  DisableThreadLibraryCalls(hInstance);

  switch (fdwReason)
  {
    case DLL_PROCESS_ATTACH:
      std::cout << "PLEASE DON'T KEEP SHOWING THIS MESSAGE\n";
      CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)kieroExampleThread, NULL, 0, NULL);
      break;
  }

  return TRUE;
}


extern "C" __declspec(dllexport) void my_asm()
{


  __asm {

    pop ebp
    add dword ptr [esi+0x0001D5F8],0x07
    jmp jmp_addr;

  }

}