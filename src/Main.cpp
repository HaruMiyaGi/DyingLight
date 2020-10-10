#include <Windows.h>
#include "Log.h"
#include "Memory.h"
#include <vector>
Logger Log;


std::vector<size_t> entity_array;



void MyHook(uintptr_t* pThis, uintptr_t entity_address, DWORD* param_2)
{
	//std::cout << "[HOOK] Entity Address: 0x" << std::hex << (size_t)entity_address << std::dec << std::endl;

	auto result = std::find(std::begin(entity_array), std::end(entity_array), (size_t)entity_address);
	if (result == std::end(entity_array))
	{
		std::cout << "[HOOK] Added Entity! Address: 0x" << std::hex << (size_t)entity_address << std::dec << " [Total: "<< entity_array.size() <<"]" << std::endl;
		float* x = (float*)(entity_address + 12);
		float* y = (float*)(entity_address + 44);
		float* z = (float*)(entity_address + 28);
		std::cout << "Pos: (x: " << *x << ", y: " << *y << ", z: " << *z << ")\n";
		entity_array.push_back((size_t)entity_address);

		//if (entity_array.size() > 1000)
		//	entity_array.pop_back();
	}

	
	hook(pThis, entity_address, param_2);
}


void MainThread(HINSTANCE hinstDLL)
{
	std::cout << "Welcome! c:\n";


	uintptr_t enemy_move_function = FindPattern("engine_x64_rwdi.dll",
		"\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xFA\x48\x8B\xD9\xE8\x00\x00\x00\x00\x44\x8B\x5B\x28\x48\x8B\x43\x20\x48\x8B\x5C\x24\x00\x4C\x8B\x40\x40\x41\x81\xE3\x00\x00\x00\x00\x4B\x8D\x0C\x5B\x48\xC1\xE1\x04",
		"xxxx?xxxxxxxxxxxx????xxxxxxxxxxxx?xxxxxxx????xxxxxxxx");
	if (enemy_move_function)
	{
		std::cout << "[HOOKED] EnemyMove()\t\taddress: 0x" << std::hex << enemy_move_function << std::dec << std::endl;

		if (!SetUp(enemy_move_function, (uintptr_t)MyHook, 16))
			std::cout << "SetUp() failed\n";
	}
	else
	{
		std::cout << "Could not find EnemyMove() pattern.\n";
	}

	while (!GetAsyncKeyState(VK_F12) && 1);
	FreeLibraryAndExitThread(hinstDLL, 0);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		if (DisableThreadLibraryCalls(hinstDLL))
			CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hinstDLL, 0, nullptr);
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