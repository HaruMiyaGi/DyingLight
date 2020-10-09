#include <Windows.h>
#include "Log.h"
Logger Log;

void MainThread(HINSTANCE hinstDLL)
{
	std::cout << "Welcome! c:\n";

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