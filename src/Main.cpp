#include <Windows.h>
#include "Log.h"
#include "Memory.h"
#include <vector>
#include <thread>

#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

#include "imgui\imgui_impl_dx11.h"
#include "imgui\imgui_impl_win32.h"


Logger Log;

static bool showImGui = true;

std::vector<uintptr_t> entity_array;
std::vector<uintptr_t> banned_entity_array;

bool AddEntity(uintptr_t entity_address)
{
	float x = *(float*)(entity_address + 12);
	float y = *(float*)(entity_address + 44);
	float z = *(float*)(entity_address + 28);

	// need to see which values stay the same for entities and do a filter on that value
	if (x != 0.0f && y != 0.0f && z != 0.0f) // do we need these - yes we do
	{
		if (-1000.0f <= x && x <= 1000.0f && -1000.0f <= y && y <= 1000.0f && -1000.0f <= z && z <= 1000.0f)
		{
			auto banned_entity_array_read = banned_entity_array;
			auto result = std::find(std::begin(banned_entity_array_read), std::end(banned_entity_array_read), entity_address); // check if it bans correctly
			if (result == std::end(banned_entity_array_read))
			{
				x = *(float*)(entity_address + 12);
				y = *(float*)(entity_address + 44);
				z = *(float*)(entity_address + 28);

				if (x != 0.0f && y != 0.0f && z != 0.0f)
				{
					auto result2 = std::find(std::begin(entity_array), std::end(entity_array), entity_address);
					if (result2 == std::end(entity_array))
					{
						entity_array.push_back(entity_address);
						return true;
					}
				}
				else
				{
					std::cout << "Baited.\n";
				}
			}
		}
	}



	return false;
}

void LoopThrough()
{
	auto entity_array_read = entity_array;
	auto banned_entity_array_read = banned_entity_array;
	size_t start_size = entity_array_read.size();


	for (auto entity = banned_entity_array_read.begin(); entity != banned_entity_array_read.end(); )
	{
		std::cout << "[BAN] Entity: " << std::hex << *entity._Ptr << std::dec << std::endl;
		++entity;
	}

	for (auto entity = entity_array_read.begin(); entity != entity_array_read.end(); )
	{
		uintptr_t base = *entity._Ptr;

		float x = *(float*)(*entity._Ptr + 12);
		float y = *(float*)(*entity._Ptr + 44);
		float z = *(float*)(*entity._Ptr + 28);

		if (x != 0.0f && y != 0.0f && z != 0.0f)
		{
			if (-1000.0f <= x && x <= 1000.0f && -1000.0f <= y && y <= 1000.0f && -1000.0f <= z && z <= 1000.0f)
			{
				++entity;
				std::cout << "[POS] x: " << x << ", y: " << y << ", z: " << z << "\t" << std::hex << base << std::dec << std::endl;
			}
			else
			{
				banned_entity_array.push_back(base);
				entity = entity_array_read.erase(entity);
			}
		}
		else
		{
			banned_entity_array.push_back(base);
			entity = entity_array_read.erase(entity);
		}
	}

	std::cout << "Entity array size: [ " << entity_array_read.size() << " ] [ " << start_size - entity_array_read.size() << " removed ]\n";
	std::cout << "BANNED: [ " << banned_entity_array.size() << " ]\n";

	entity_array = entity_array_read;
}
void UpdateProcedure()
{
	std::cout << "owo\n";

	while (!GetAsyncKeyState(VK_F12) & 1)
	{
		Sleep(25000);
		LoopThrough();
	}
}

using fnEnemyMove = int(__fastcall*)(uintptr_t* pThis, uintptr_t entity_address, DWORD* param_2);
fnEnemyMove EnemyMoveHook;
void EnemyMove(uintptr_t* pThis, uintptr_t entity_address, DWORD* param_2)
{
	if (AddEntity(entity_address))
		std::cout << "[HOOK] Added Entity: 0x" << std::hex << entity_address << std::dec << std::endl;

	//float* x = (float*)(entity_address + 12);
	//float* y = (float*)(entity_address + 44);
	//float* z = (float*)(entity_address + 28);
	//std::cout << "Pos: (x: " << *x << ", y: " << *y << ", z: " << *z << ")\n";

	EnemyMoveHook(pThis, entity_address, param_2);
}




ID3D11Device* pDevice = nullptr;
IDXGISwapChain* pSwapChain = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* pTarget = nullptr;
enum class IDXGISwapChainVMT {
	QueryInterface,
	AddRef,
	Release,
	SetPrivateData,
	SetPrivateDataInterface,
	GetPrivateData,
	GetParent,
	GetDevice,
	Present,
	GetBuffer,
	SetFullscreenState,
	GetFullscreenState,
	GetDesc,
	ResizeBuffers,
	ResizeTarget,
	GetContainingOutput,
	GetFrameStatistics,
	GetLastPresentCount,
};

using fnD3DPresent = HRESULT(__stdcall*)(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
fnD3DPresent PresentHook;

void* ogFunction;
bool GetPresentFunction()
{
	DXGI_SWAP_CHAIN_DESC scd = { 0 };
	scd.BufferDesc.Width = 0;
	scd.BufferDesc.Height = 0;
	scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferCount = 1;
	scd.OutputWindow = GetForegroundWindow();
	scd.Windowed = TRUE;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Flags = 0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, 0, nullptr, 0,
		D3D11_SDK_VERSION,
		&scd, &pSwapChain, &pDevice, nullptr, &pContext
	);

	if (FAILED(hr))
		return false;

	void** pVTM = *(void***)pSwapChain;
	ogFunction = (fnD3DPresent)(pVTM[(UINT)IDXGISwapChainVMT::Present]);
	if (pDevice) { pDevice->Release(); pDevice = nullptr; }
	if (pSwapChain) { pSwapChain->Release(); pSwapChain = nullptr; }
	if (pContext) { pContext->Release(); pContext = nullptr; }

	return true;
}


using fnWinProc = LRESULT(__stdcall*)(HWND, UINT, WPARAM, LPARAM);
fnWinProc WndProc;
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProcHook(const HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (showImGui)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
		return true;
	}

	return CallWindowProc(WndProc, hWnd, msg, wParam, lParam);
}



bool InitD3D(IDXGISwapChain* pSwapChain)
{
	HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice);
	if (FAILED(hr)) return false;

	pDevice->GetImmediateContext(&pContext);
	DXGI_SWAP_CHAIN_DESC sd;
	pSwapChain->GetDesc(&sd);

	WndProc = (WNDPROC)SetWindowLongPtr(sd.OutputWindow, GWLP_WNDPROC, (LONG_PTR)WndProcHook);

	pContext->OMGetRenderTargets(1, &pTarget, nullptr); // we could also get stencil view

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(sd.OutputWindow);

	ImGui_ImplDX11_Init(pDevice, pContext);
	ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureKeyboard;


	return true;
}

HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!pTarget)
	{
		if (InitD3D(pSwapChain))
			std::cout << "pTarget = " << pTarget << std::endl;
		else
			std::cout << "InitD3D() failed\n";
	}

	if (showImGui)
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		if (ImGui::Begin("owo"))
		{
			ImGui::Text("uwu");
		}
		ImGui::End();
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}


	return PresentHook(pSwapChain, SyncInterval, Flags);
}





void MainThread(HINSTANCE hinstDLL)
{
	std::cout << "uwu\n";

	if (GetPresentFunction())
		PresentHook = HookFunction<fnD3DPresent>((uintptr_t)ogFunction, (uintptr_t)Present, 14);



	// Enemy Move function hook
	/*
	uintptr_t enemy_move_function = FindPattern("engine_x64_rwdi.dll",
		"\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xFA\x48\x8B\xD9\xE8\x00\x00\x00\x00\x44\x8B\x5B\x28\x48\x8B\x43\x20\x48\x8B\x5C\x24\x00\x4C\x8B\x40\x40\x41\x81\xE3\x00\x00\x00\x00\x4B\x8D\x0C\x5B\x48\xC1\xE1\x04",
		"xxxx?xxxxxxxxxxxx????xxxxxxxxxxxx?xxxxxxx????xxxxxxxx");

	if (enemy_move_function)
	{
		EnemyMoveHook = HookFunction<fnEnemyMove>(enemy_move_function, (uintptr_t)EnemyMove, 16);

		if (EnemyMoveHook != nullptr)
			std::cout << "[HOOKED] EnemyMove() address: 0x" << std::hex << enemy_move_function << std::dec << std::endl;
		else
			std::cout << "[HOOK] Failed to hook EnemyMove.\n";
	}
	else
	{
		std::cout << "[HOOK] Failed to find EnemyMove function.\n";
	}
	*/

	while (!GetAsyncKeyState(VK_F12) & 1)
	{
		if (GetAsyncKeyState(VK_F1))
			showImGui = true;

		if (GetAsyncKeyState(VK_F2))
			showImGui = false;
	}

	FreeLibraryAndExitThread(hinstDLL, 0);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		if (DisableThreadLibraryCalls(hinstDLL))
		{
			CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hinstDLL, 0, nullptr);
			//static std::thread* nani = new std::thread(UpdateProcedure);
		}
	} break;
	case DLL_PROCESS_DETACH:
	{
		Log.~Logger();
	} break;
	case DLL_THREAD_DETACH:
	case DLL_THREAD_ATTACH:
		break;
	}
	return TRUE;
}