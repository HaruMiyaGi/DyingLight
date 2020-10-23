#include <Windows.h>
#include "Log.h"
#include "Memory.h"
#include <vector>
#include <thread>

#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

#include "D3D11.h"

Logger Log;
D3D11 draw;

uintptr_t GAMEDLL;
DWORD64 g_matrix_offset = 0xC463AE70;

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
		Sleep(10000);
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

bool w2s(Vec3 pos, Vec2& screen, float view_matrix[16], int WIDTH, int HEIGHT)
{
	Vec4 clip;

	clip.x = pos.x * view_matrix[0] + pos.y * view_matrix[1] + pos.z * view_matrix[2] + view_matrix[3];
	clip.y = pos.x * view_matrix[4] + pos.y * view_matrix[5] + pos.z * view_matrix[6] + view_matrix[7];
	clip.z = pos.x * view_matrix[8] + pos.y * view_matrix[9] + pos.z * view_matrix[10] + view_matrix[11];
	clip.w = pos.x * view_matrix[12] + pos.y * view_matrix[13] + pos.z * view_matrix[14] + view_matrix[15];

	if (clip.w < 0.1f)
		return false;

	Vec3 NDC;
	NDC.x = clip.x / clip.w;
	NDC.y = clip.y / clip.w;
	NDC.z = clip.z / clip.w;

	screen.x = NDC.x;
	screen.y = NDC.y;

	screen.x = (WIDTH / 2 * NDC.x) + (NDC.x + WIDTH / 2);
	screen.y = -(HEIGHT / 2 * NDC.y) + (NDC.y + HEIGHT / 2);

	return true;
}

using fnD3DPresent = HRESULT(__stdcall*)(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
fnD3DPresent PresentHook;
HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	draw.Init(pSwapChain);

	//POINT p;
	//GetCursorPos(&p);
	//ScreenToClient(g_HWND, &p);
	//draw.Line(Vec2(0, 0), Vec2(p.x, p.y));
	//draw.Line(Vec2(fWidth, 0), Vec2(p.x, p.y));
	//draw.Line(Vec2(0, fHeight), Vec2(p.x, p.y));
	//draw.Line(Vec2(fWidth, fHeight), Vec2(p.x, p.y));
	float x, y, z;
	if (GAMEDLL)
	{
		x = *(float*)(GAMEDLL + 0x1C3D460 - 8);
		z = *(float*)(GAMEDLL + 0x1C3D460 - 4);
		y = *(float*)(GAMEDLL + 0x1C3D460 - 0);

		uintptr_t matrix_address = (uintptr_t)g_matrix_offset;// + i * 4;
		float view_matrix[16];
		view_matrix[0] = *(float*)(matrix_address + 0);
		view_matrix[1] = *(float*)(matrix_address + 4);
		view_matrix[2] = *(float*)(matrix_address + 8);
		view_matrix[3] = *(float*)(matrix_address + 12);
		view_matrix[4] = *(float*)(matrix_address + 16);
		view_matrix[5] = *(float*)(matrix_address + 20);
		view_matrix[6] = *(float*)(matrix_address + 24);
		view_matrix[7] = *(float*)(matrix_address + 28);
		view_matrix[8] = *(float*)(matrix_address + 32);
		view_matrix[9] = *(float*)(matrix_address + 36);
		view_matrix[10] = *(float*)(matrix_address + 40);
		view_matrix[11] = *(float*)(matrix_address + 44);
		view_matrix[12] = *(float*)(matrix_address + 48);
		view_matrix[13] = *(float*)(matrix_address + 52);
		view_matrix[14] = *(float*)(matrix_address + 56);
		view_matrix[15] = *(float*)(matrix_address + 60);
		Vec2 screen;
		if (w2s(Vec3(x, z, y), screen, view_matrix, fWidth, fHeight))
			draw.Line(Vec2(fWidth / 2.0f, fHeight), screen);
	}




	if (showImGui)
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		if (ImGui::Begin("[F1]", 0, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("uwu");
			ImGui::Text("x: %.1f, y: %.1f, z: %.1f", x, y, z);
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

	MODULEINFO mInfo = GetModuleInfo("gamedll_x64_rwdi.dll");
	GAMEDLL = (uintptr_t)mInfo.lpBaseOfDll;

	PresentHook = HookFunction<fnD3DPresent>(draw.GetPresentFunction(), (uintptr_t)Present, 14);

	// Enemy Move function hook	
	/*uintptr_t enemy_move_function = FindPattern("engine_x64_rwdi.dll",
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
	}*/




	while (!GetAsyncKeyState(VK_F12) & 1)
	{
		if (GetAsyncKeyState(VK_F1))
			showImGui = true;

		if (GetAsyncKeyState(VK_F2))
			showImGui = false;

	}

	FreeLibraryAndExitThread(hinstDLL, 0);

	PresentHook = UnHookFunction<fnD3DPresent>(draw.GetPresentFunction(), PresentHook, 14);
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