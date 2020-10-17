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


DWORD64 g_matrix_offset = 0xC25B8780;


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

bool toggle1 = true, toggle2 = true, toggle3 = true, toggle4 = true;


bool w2s_ac(const Vec3 pos, Vec2& screen, const Matrix4x4 ViewMatrix, int windowWidth, int windowHeight)
{
	Vec4 clipCoords;
	//clipCoords.x = pos.x * ViewMatrix.f[0] + pos.y * ViewMatrix.f[4] + pos.z * ViewMatrix.f[8] + ViewMatrix.f[12];
	clipCoords.x = pos.x * ViewMatrix.f[4] + pos.y * ViewMatrix.f[0] + pos.z * ViewMatrix.f[8] + ViewMatrix.f[12];
	///clipCoords.x = pos.x * ViewMatrix.f[4] + pos.y * ViewMatrix.f[8] + pos.z * ViewMatrix.f[0] + ViewMatrix.f[12];
	clipCoords.y = pos.x * ViewMatrix.f[1] + pos.y * ViewMatrix.f[5] + pos.z * ViewMatrix.f[9] + ViewMatrix.f[13];///
	clipCoords.z = pos.x * ViewMatrix.f[2] + pos.y * ViewMatrix.f[6] + pos.z * ViewMatrix.f[10] + ViewMatrix.f[14];
	//clipCoords.w = pos.x * ViewMatrix.f[3] + pos.y * ViewMatrix.f[7] + pos.z * ViewMatrix.f[11] + ViewMatrix.f[15];
	clipCoords.w = pos.x * ViewMatrix.f[3] + pos.y * ViewMatrix.f[7] + pos.z * ViewMatrix.f[11] + -ViewMatrix.f[15];

	if (clipCoords.w < 0.1f)
		return false;

	Vec3 NDC;
	NDC.x = clipCoords.x / clipCoords.w;
	NDC.y = clipCoords.y / clipCoords.w;
	NDC.z = clipCoords.z / clipCoords.w;
	screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
	screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
	return true;
}

Matrix4x4 tt(Matrix4x4 lol)
{
	Matrix4x4 nice;

	nice.f[0] = lol.f[2]; ///
	nice.f[1] = lol.f[6]; ///
	nice.f[2] = lol.f[10]; ///
	nice.f[3] = lol.f[14]; ///

	nice.f[4] = -lol.f[0]; ///
	nice.f[5] = -lol.f[4]; ///
	nice.f[6] = -lol.f[8]; /// -
	nice.f[7] = -lol.f[12]; /// -

	nice.f[8] = lol.f[1]; ///
	nice.f[9] = lol.f[5]; ///
	nice.f[10] = lol.f[9]; ///
	nice.f[11] = lol.f[13]; ///

	nice.f[12] = -lol.f[3]; ///
	nice.f[13] = lol.f[7]; ///
	nice.f[14] = lol.f[11]; ///
	nice.f[15] = lol.f[15]; ///

	return nice;
}


bool w2s_dl(const Vec3 pos, Vec2& screen, const Matrix4x4 ViewMatrix, int windowWidth, int windowHeight)
{
	Vec4 clipCoords;

	clipCoords.x = pos.x * ViewMatrix.f[2] + pos.y * -ViewMatrix.f[0] + pos.z * ViewMatrix.f[1] + ViewMatrix.f[3];
	clipCoords.y = pos.x * ViewMatrix.f[6] + pos.y * ViewMatrix.f[4] + pos.z * ViewMatrix.f[5] + ViewMatrix.f[7];
	clipCoords.z = pos.x * ViewMatrix.f[8] + pos.y * ViewMatrix.f[10] + pos.z * ViewMatrix.f[9] + ViewMatrix.f[11];
	clipCoords.w = pos.x * ViewMatrix.f[14] + pos.y * -ViewMatrix.f[12] + pos.z * ViewMatrix.f[13] + -ViewMatrix.f[15];

	if (clipCoords.w < 0.1f)
	{
		std::cout << "[!!!]\n";
		return false;
	}

	Vec3 NDC;
	NDC.x = clipCoords.x / clipCoords.w;
	NDC.y = clipCoords.y / clipCoords.w;
	NDC.z = clipCoords.z / clipCoords.w;

	screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
	screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
	return true;
}


bool w2s(const Vec3 pos, Vec2& screen, const Matrix4x4 ViewMatrix, int windowWidth, int windowHeight)
{
	Vec4 clipCoords;

	if (toggle1)
		clipCoords.x = pos.x * ViewMatrix.f[6] + pos.y * ViewMatrix.f[4] + pos.z * ViewMatrix.f[5] + ViewMatrix.f[7];
	else
		clipCoords.x = pos.x * ViewMatrix.f[4] + pos.y * ViewMatrix.f[6] + pos.z * ViewMatrix.f[5] + ViewMatrix.f[7];

	if (toggle2)
		clipCoords.y = pos.x * ViewMatrix.f[1] + pos.y * ViewMatrix.f[2] + pos.z * ViewMatrix.f[0] + ViewMatrix.f[3];
	else
		clipCoords.y = pos.x * ViewMatrix.f[0] + pos.y * ViewMatrix.f[2] + pos.z * ViewMatrix.f[1] + ViewMatrix.f[3];

	if (toggle3)
		clipCoords.z = pos.x * ViewMatrix.f[8] + pos.y * ViewMatrix.f[9] + pos.z * ViewMatrix.f[10] + ViewMatrix.f[11];
	else
		clipCoords.z = pos.x * ViewMatrix.f[8] + pos.y * ViewMatrix.f[10] + pos.z * ViewMatrix.f[9] + ViewMatrix.f[11];

	if (toggle4)
		clipCoords.w = pos.x * ViewMatrix.f[12] + pos.y * ViewMatrix.f[13] + pos.z * ViewMatrix.f[14] + ViewMatrix.f[15];
	else
		clipCoords.w = pos.x * ViewMatrix.f[12] + pos.y * ViewMatrix.f[14] + pos.z * ViewMatrix.f[13] + ViewMatrix.f[15];

	if (clipCoords.w < 0.1f)
		return false;

	Vec3 NDC;
	NDC.x = clipCoords.x / clipCoords.w;
	NDC.y = clipCoords.y / clipCoords.w;
	NDC.z = clipCoords.z / clipCoords.w;

	screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
	screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
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



	uintptr_t matrix_address = (uintptr_t)g_matrix_offset;// + i * 4;

	//float matrix[16];
	Matrix4x4 view_matrix;
	view_matrix.f[0] = *(float*)(matrix_address + 0);
	view_matrix.f[1] = *(float*)(matrix_address + 4);
	view_matrix.f[2] = *(float*)(matrix_address + 8);
	view_matrix.f[3] = *(float*)(matrix_address + 12);

	view_matrix.f[4] = *(float*)(matrix_address + 16);
	view_matrix.f[5] = *(float*)(matrix_address + 20);
	view_matrix.f[6] = *(float*)(matrix_address + 24);
	view_matrix.f[7] = *(float*)(matrix_address + 28);

	view_matrix.f[8] = *(float*)(matrix_address + 32);
	view_matrix.f[9] = *(float*)(matrix_address + 36);
	view_matrix.f[10] = *(float*)(matrix_address + 40);
	view_matrix.f[11] = *(float*)(matrix_address + 44);

	view_matrix.f[12] = *(float*)(matrix_address + 48);
	view_matrix.f[13] = *(float*)(matrix_address + 52);
	view_matrix.f[14] = *(float*)(matrix_address + 56);
	view_matrix.f[15] = *(float*)(matrix_address + 60);

	Matrix4x4 wow = tt(view_matrix);
	//
	
	Vec2 screen;

	if (w2s_ac(Vec3(715.0f, -62.0f, 132.0f), screen, wow, fWidth, fHeight))
		draw.Line(Vec2(fWidth / 2.0f, fHeight), screen);
	if (w2s_ac(Vec3(-62.0f, 715.0f, 132.0f), screen, wow, fWidth, fHeight))
		draw.Line(Vec2(fWidth / 2.0f, fHeight), screen);

	if (w2s_ac(Vec3(710.0f, -48.0f, 127.0f), screen, wow, fWidth, fHeight))
		draw.Line(Vec2(fWidth / 2.0f, fHeight), screen);
	if (w2s_ac(Vec3(-48.0f, 710.0f, 127.0f), screen, wow, fWidth, fHeight))
		draw.Line(Vec2(fWidth / 2.0f, fHeight), screen);



	if (showImGui)
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		if (ImGui::Begin("[F1]", 0, ImGuiWindowFlags_NoResize))
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

	PresentHook = HookFunction<fnD3DPresent>(draw.GetPresentFunction(), (uintptr_t)Present, 14);



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

		int option = 0;
		std::cout << "[1] toggle1, [2] toggle2, [3] toggle3, [4] toggle4, [5] matrix offset\n";
		std::cout << "Option: ";
		std::cin >> option;


		switch (option)
		{
		case 1:
		{
			if (toggle1)
				toggle1 = false;
			else
				toggle1 = true;
			std::cout << "Toggle " << option << " is : " << toggle1 << "\n";
		} break;
		case 2:
		{
			if (toggle2)
				toggle2 = false;
			else
				toggle2 = true;
			std::cout << "Toggle " << option << " is : " << toggle2 << "\n";
		} break;
		case 3:
		{
			if (toggle3)
				toggle3 = false;
			else
				toggle3 = true;
			std::cout << "Toggle " << option << " is : " << toggle3 << "\n";
		} break;
		case 4:
		{
			if (toggle4)
				toggle4 = false;
			else
				toggle4 = true;
			std::cout << "Toggle " << option << " is : " << toggle4 << "\n";
		} break;
		case 5:
		{
			std::cout << "Matrix offset = 0x" << std::hex << g_matrix_offset << std::dec << std::endl;
			std::cout << "New offset: ";
			std::cin >> g_matrix_offset;
		} break;
		case 0:
		{
			system("cls");
		} break;
		default: break;
		}

		//view_matrix[0][0] = *(float*)(adrez + 0);
		//view_matrix[0][1] = *(float*)(adrez + 4);
		//view_matrix[0][2] = *(float*)(adrez + 8);
		//view_matrix[0][3] = *(float*)(adrez + 12);

		//view_matrix[1][0] = *(float*)(adrez + 16);
		//view_matrix[1][1] = *(float*)(adrez + 20);
		//view_matrix[1][2] = *(float*)(adrez + 24);
		//view_matrix[1][3] = *(float*)(adrez + 28);

		//view_matrix[2][0] = *(float*)(adrez + 32);
		//view_matrix[2][1] = *(float*)(adrez + 36);
		//view_matrix[2][2] = *(float*)(adrez + 40);
		//view_matrix[2][3] = *(float*)(adrez + 44);

		//view_matrix[3][0] = *(float*)(adrez + 48);
		//view_matrix[3][1] = *(float*)(adrez + 52);
		//view_matrix[3][2] = *(float*)(adrez + 56);
		//view_matrix[3][3] = *(float*)(adrez + 60);

		//system("cls");
		//std::cout << "[ " << view_matrix[0][0] << " ]" << "[ " << view_matrix[0][1] << " ]" << "[ " << view_matrix[0][2] << " ]" << "[ " << view_matrix[0][3] << " ]\n";
		//std::cout << "[ " << view_matrix[1][0] << " ]" << "[ " << view_matrix[1][1] << " ]" << "[ " << view_matrix[1][2] << " ]" << "[ " << view_matrix[1][3] << " ]\n";
		//std::cout << "[ " << view_matrix[2][0] << " ]" << "[ " << view_matrix[2][1] << " ]" << "[ " << view_matrix[2][2] << " ]" << "[ " << view_matrix[2][3] << " ]\n";
		//std::cout << "[ " << view_matrix[3][0] << " ]" << "[ " << view_matrix[3][1] << " ]" << "[ " << view_matrix[3][2] << " ]" << "[ " << view_matrix[3][3] << " ]\n";
		//std::cout << "\n=====================================\n\n";
		//Sleep(100);

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