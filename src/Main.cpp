#include <Windows.h>
#include <thread>

void* MainThread(HINSTANCE hinstDLL)
{

	while (!GetAsyncKeyState(VK_END) && 1);
	FreeLibraryAndExitThread(hinstDLL, 0);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		if (DisableThreadLibraryCalls(hinstDLL))
			static std::thread* main_thread = new std::thread(MainThread, hinstDLL);
	} break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_DETACH:
	case DLL_THREAD_ATTACH:
		break;
	}
	return TRUE;
}