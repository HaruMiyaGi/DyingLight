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

uintptr_t global_matrix_offset;
uintptr_t global_player_pos_offset;
uintptr_t global_clock_address;


bool global_cursor_show = false;


void HideMouse()
{
	while (::ShowCursor(TRUE) >= 0);
}

void ShowMouse()
{
	while (::ShowCursor(TRUE) < 0);
}

void ConfineCursor(HWND hwnd)
{
	RECT rect;
	GetClientRect(hwnd, &rect);
	MapWindowPoints(hwnd, nullptr, reinterpret_cast<POINT*>(&rect), 2);
	ClipCursor(&rect);
}

void FreeCursor()
{
	ClipCursor(nullptr);
}




float imgui_zombie_height = 2.0f;
float max_dist = 40.0f;
uintptr_t location = 0;

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



bool w2s(Vec3 pos, Vec2& screen, int WIDTH, int HEIGHT)
{
	float view_matrix[16];
	memcpy(view_matrix, (void*)global_matrix_offset, sizeof(float) * 16);

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



void OnFade(int fade_type)
{
	if (fade_type == 1)
	{
		// pause scripts
	}

	if (fade_type == 3)
	{
		// update ptrs
		// unpause scripts
	}
}


struct Zombie
{
	float x, y, z;

	Zombie()
		: x(0), y(0), z(0)
	{}

	Zombie(float x, float y, float z)
		: x(x), y(y), z(z)
	{}
};

std::vector<Zombie> zombie_array;
std::vector<uintptr_t*> zombie_ptr_array;

void AddZombie(uintptr_t* pThis)
{
	auto res = std::find(std::begin(zombie_ptr_array), std::end(zombie_ptr_array), pThis);
	if (res == std::end(zombie_ptr_array))
		zombie_ptr_array.push_back(pThis);

	//float* x = (float*)((size_t)pThis + 0x138); // 0x138
	//float* z = (float*)((size_t)pThis + 0x13C); // 0x13C
	//float* y = (float*)((size_t)pThis + 0x140); // 0x140
	//Zombie zombie(*x, *y, *z);
	//zombie_array.push_back(zombie);
}



std::vector<uintptr_t*> ent_ptrs;
float GetDistance(const Vec3& point1, const Vec3& point2)
{
	float distance = sqrt((point1.x - point2.x) * (point1.x - point2.x) +
		(point1.y - point2.y) * (point1.y - point2.y) +
		(point1.z - point2.z) * (point1.z - point2.z));
	return distance;
}

using fnD3DPresent = HRESULT(__stdcall*)(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
fnD3DPresent PresentHook;
HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	draw.Init(pSwapChain);

	///*
	Vec2 screen;
	auto entities = ent_ptrs;
	for (auto& entity : entities)
	{
		//if (entity == nullptr)
		//{
		//	std::remove(ent_ptrs.begin(), ent_ptrs.end(), entity);
		//	continue;
		//}

		Vec3 ent; 
		ent.x = *(float*)((size_t)entity + 0x138);
		ent.z = *(float*)((size_t)entity + 0x13C);
		ent.y = *(float*)((size_t)entity + 0x140);

		Vec3 me;
		me.x = *(float*)global_player_pos_offset;
		me.z = *(float*)(global_player_pos_offset + 4);
		me.y = *(float*)(global_player_pos_offset + 8);

		float distance = GetDistance(me, ent);

		if (distance <= max_dist)
		{
			if (w2s(Vec3(ent.x, ent.z, ent.y), screen, fWidth, fHeight))
			{
				Vec2 screen2;
				if (w2s(Vec3(ent.x, ent.z + imgui_zombie_height, ent.y), screen2, fWidth, fHeight))
					draw.Line(screen, screen2);
				//draw.Line(Vec2(fWidth / 2.0f, fHeight), screen);
			}
		}
		else
		{
			auto ent_ptrs_remove = ent_ptrs;
			ent_ptrs_remove.erase(std::remove(ent_ptrs_remove.begin(), ent_ptrs_remove.end(), entity), ent_ptrs_remove.end());
			ent_ptrs = ent_ptrs_remove;
		}

	}

	//*/

	/*if (GAMEDLL)
	{
		x = *(float*)(GAMEDLL + 0x1C3D460 - 8);
		z = *(float*)(GAMEDLL + 0x1C3D460 - 4);
		y = *(float*)(GAMEDLL + 0x1C3D460 - 0);

		if (w2s(Vec3(x, z, y), screen, view_matrix, fWidth, fHeight))
			draw.Line(Vec2(fWidth / 2.0f, fHeight), screen);
	}*/

	if (showImGui)
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();


		if (ImGui::Begin("Time", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::SliderFloat("Time", &(*(float*)global_clock_address), 0.0f, 1.0f);
			ImGui::End();
		}


		if (ImGui::TreeNode("Entities"))
		{
			float myx = *(float*)global_player_pos_offset;
			float myz = *(float*)(global_player_pos_offset + 4);
			float myy = *(float*)(global_player_pos_offset + 8);
			ImGui::Text("My Pos: x: %f, z: %f, y: %f", myx, myz, myy);


			for (int i = 0; i < ent_ptrs.size(); i++)
			{
				auto ent = ent_ptrs.at(i);
				if (ImGui::TreeNode((void*)(uintptr_t)i, "%x", ent))
				{
					float x = *(float*)((size_t)ent + 0x138);
					float z = *(float*)((size_t)ent + 0x13C);
					float y = *(float*)((size_t)ent + 0x140);

					ImGui::Text("x: %f, z: %f, y: %f", x, y, z);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}


		if (ImGui::Begin("[F1]", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Offsets: %d", ent_ptrs.size());
			ImGui::Text("Max Distance");
			ImGui::SliderFloat("Max Distance", &max_dist, 1.0f, 100.0f, "%.1f");
			ImGui::Text("Zombie Height");
			ImGui::SliderFloat("Zombie Height", &imgui_zombie_height, 0.01f, 2.0f, "%.1f");
		}
		ImGui::End();



		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	return PresentHook(pSwapChain, SyncInterval, Flags);
}



using fnMoveFunc3 = int(__fastcall*)(uintptr_t* pThis);
fnMoveFunc3 MoveFuncHook3;
int MyMoveFunc3(uintptr_t* pThis)
{
	Vec3 entity;
	entity.x = *(float*)((size_t)pThis + 0x138);
	entity.z = *(float*)((size_t)pThis + 0x13C);
	entity.y = *(float*)((size_t)pThis + 0x140);

	Vec3 me;
	me.x = *(float*)global_player_pos_offset;
	me.z = *(float*)(global_player_pos_offset + 4);
	me.y = *(float*)(global_player_pos_offset + 8);

	float dist = GetDistance(me, entity);

	if (dist <= max_dist)
	{
		// draw.Line(me, entity);

		/*std::vector<Vec2> draw_stack;
		Vec2 screen;
		if (w2s(entity, screen, fWidth, fHeight));
		{
			draw_stack.push_back(screen);
		}*/

		auto ent_ptrs_search = ent_ptrs;
		if (std::find(ent_ptrs_search.begin(), ent_ptrs_search.end(), pThis) == ent_ptrs_search.end())
			ent_ptrs.push_back(pThis);
	}
	//else
	//{
	//	std::remove(ent_ptrs.begin(), ent_ptrs.end(), pThis);

	//	//for (auto& ent : ent_ptrs)
	//	//{
	//	//	float* ex = (float*)((size_t)ent + 0x138);
	//	//	float* ez = (float*)((size_t)ent + 0x13C);
	//	//	float* ey = (float*)((size_t)ent + 0x140);
	//	//	Vec3 ent_pos(*ex, *ez, *ey);
	//	//	float ent_dist = GetDistance(me, ent_pos);
	//	//	if (ent_dist <= max_dist)
	//	//		std::remove(ent_ptrs.begin(), ent_ptrs.end(), ent);
	//	//}
	//}
	return MoveFuncHook3(pThis);
}


void MainThread(HINSTANCE hinstDLL)
{
	std::cout << "uwu\n";

	global_matrix_offset = FindDMAAddy("engine_x64_rwdi.dll", 0xA2F238, { 0x78, 0x60 });
	std::cout << "[DLL] Matrix Offset: 0x" << std::hex << global_matrix_offset << std::dec << "\n";

	auto mInfo = GetModuleInfo("gamedll_x64_rwdi.dll");
	global_player_pos_offset = ((uintptr_t)mInfo.lpBaseOfDll + 0x1DA3BC0);
	std::cout << "[DLL] Player Pos Offset: 0x" << std::hex << global_player_pos_offset << std::dec << "\n";

	global_clock_address = FindDMAAddy("gamedll_x64_rwdi.dll", 0x01CA4AC0, { 0xA4 });


	
	PresentHook = HookFunction<fnD3DPresent>(draw.GetPresentFunction(), (uintptr_t)Present, 14);
	/// we need minimum 12 opcodes


	/*uintptr_t move_func_1 = FindPattern("gamedll_x64_rwdi.dll", "\x40\x55\x41\x54\x48\x8D\x6C\x24\x00\x48\x81\xEC\x00\x00\x00\x00\x4C\x8B\xE1\xE8\x00\x00\x00\x00\x4D\x8B\x1C\x24\x49\x8B\xCC\x41\xFF\x93\x00\x00\x00\x00\x84\xC0\x0F\x84\x00\x00\x00\x00\x49\x8B\xCC\x44\x0F\x29\x8C\x24\x00\x00\x00\x00\x44\x0F\x29\x94\x24\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x45\x0F\x57\xC9\x44\x0F\x28\xD0\x45\x0F\x2E\xD1\x0F\x84\x00\x00\x00\x00", "xxxxxxxx?xxx????xxxx????xxxxxxxxxx????xxxx????xxxxxxxx????xxxxx????xx????xxxxxxxxxxxxxx????");
	std::cout << "[DLL] move_func_1 address: 0x" << std::hex << move_func_1 << std::dec << "\n";
	uintptr_t move_func_2 = FindPattern("gamedll_x64_rwdi.dll", "\x48\x89\x5C\x24\x00\x55\x48\x8D\x6C\x24\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x01\x48\x8B\xD9\xFF\x90\x00\x00\x00\x00\x83\xB8\x00\x00\x00\x00\x00\x0F\x85\x00\x00\x00\x00\x48\x8B\x03\x48\x89\xBC\x24\x00\x00\x00\x00\x0F\x29\xB4\x24\x00\x00\x00\x00\x0F\x29\xBC\x24\x00\x00\x00\x00\x44\x0F\x29\x84\x24\x00\x00\x00\x00\x44\x0F\x29\x8C\x24\x00\x00\x00\x00\x48\x8B\xCB\x44\x0F\x29\x54\x24\x00\x44\x0F\x29\x5C\x24\x00", "xxxx?xxxxx?xxx????xxxxxxxx????xx?????xx????xxxxxxx????xxxx????xxxx????xxxxx????xxxxx????xxxxxxxx?xxxxx?");
	std::cout << "[DLL] move_func_2 address: 0x" << std::hex << move_func_2 << std::dec << "\n";*/
	//uintptr_t move_func_4 = FindPattern("gamedll_x64_rwdi.dll", "\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x70\x18\x48\x89\x78\x20\x55\x48\x8D\x68\xA1\x48\x81\xEC\x00\x00\x00\x00\x0F\x29\x70\xE8\x0F\x29\x78\xD8\x44\x0F\x29\x40\x00\x48\x8B\xD9\x49\x8B\xF0\x48\x8B\xFA\x44\x0F\x29\x50\x00\x44\x0F\x29\x58\x00\x44\x0F\x29\x60\x00\x44\x0F\x29\x6C\x24\x00\x44\x0F\x29\x74\x24\x00\x44\x0F\x29\x7C\x24\x00\x4C\x89\x60\x08\x44\x8B\xA1\x00\x00\x00\x00\x48\x8B\x49\x08\x48\x8B\x41\x40\xF3\x0F\x10\xB1\x00\x00\x00\x00", "xxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxx?xxxxxxxxxxxxx?xxxx?xxxx?xxxxx?xxxxx?xxxxx?xxxxxxx????xxxxxxxxxxxx????");
	//std::cout << "[DLL] move_func_4 address: 0x" << std::hex << move_func_4 << std::dec << "\n";

	uintptr_t move_func_3 = FindPattern("gamedll_x64_rwdi.dll", "\x40\x53\x48\x83\xEC\x40\x8B\x81\x00\x00\x00\x00\x48\x8B\xD9\x0F\x29\x74\x24\x00\x89\x81\x00\x00\x00\x00\x8B\x81\x00\x00\x00\x00\x0F\x29\x7C\x24\x00\x0F\x28\xF9\x89\x81\x00\x00\x00\x00\x8B\x81\x00\x00\x00\x00\x89\x81\x00\x00\x00\x00\x48\x8B\x49\x50\x48\x83\xC1\x18\xFF\x15\x00\x00\x00\x00\x48\x8B\xC8\x48\x8B\x10\xFF\x92\x00\x00\x00\x00\x48\x85\xC0\x74\x21\x48\x8B\x10\x48\x8B\xC8\xFF\x92\x00\x00\x00\x00\x84\xC0\x74\x11\x48\x8B\xCB\xFF\x15\x00\x00\x00\x00\xF3\x0F\x11\x83\x00\x00\x00\x00", "xxxxxxxx????xxxxxxx?xx????xx????xxxx?xxxxx????xx????xx????xxxxxxxxxx????xxxxxxxx????xxxxxxxxxxxxx????xxxxxxxxx????xxxx????");
	//std::cout << "[DLL] move_func_3 address: 0x" << std::hex << move_func_3 << std::dec << "\n";
	if (move_func_3)
	{
		MoveFuncHook3 = HookFunction<fnMoveFunc3>(move_func_3, (uintptr_t)MyMoveFunc3, 20);

		if (MoveFuncHook3 != nullptr)
		{
			std::cout << "[HOOK] MyMoveFunc3() address: 0x" << std::hex << move_func_3 << std::dec << "\n";
		}
	}


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
		{
			if (SetForegroundWindow(GetDesktopWindow()))
				showImGui = true;
			else
				showImGui = false;
			//showImGui = true;
			//SetForegroundWindow(GetDesktopWindow());
		}

		if (GetAsyncKeyState(VK_F2))
		{
			showImGui = false;
			SetForegroundWindow(GetDesktopWindow());
		}
	}

	PresentHook = UnHookFunction<fnD3DPresent>(draw.GetPresentFunction(), PresentHook, 14);
	MoveFuncHook3 = UnHookFunction<fnMoveFunc3>(move_func_3, MoveFuncHook3, 20);

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