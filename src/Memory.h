#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <Psapi.h>


MODULEINFO GetModuleInfo(const char* szModule)
{
    MODULEINFO modinfo = { 0 };
    HMODULE hModule = GetModuleHandle(szModule);
    if (hModule == 0)
        return modinfo;

    GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
    return modinfo;
}

uintptr_t FindPattern(const char* module, const char* pattern, const char* mask)
{
    MODULEINFO mInfo = GetModuleInfo(module);
    uintptr_t base = (uintptr_t)mInfo.lpBaseOfDll;
    uintptr_t size = (uintptr_t)mInfo.SizeOfImage;

    uintptr_t patternLenght = (uintptr_t)strlen(mask);

    for (uintptr_t i = 0; i < size - patternLenght; i++)
    {
        bool found = true;

        for (DWORD64 j = 0; j < patternLenght; j++)
            found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);

        if (found)
            return base + i;
    }
    return 0;
}

uintptr_t GetRealAddress(BYTE* address)
{
	uintptr_t jmp = 0;
	memcpy(&jmp, address + 0x1, 4);
	address = address + jmp + 5;
	return reinterpret_cast<uintptr_t>(address);
}



template <typename T>
T HookFunction(uintptr_t function, uintptr_t trampoline_function, int bytes)
{
	const unsigned char jump_instruction[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
	// TODO: check if first byte is E9 and if it is then do this step
	trampoline_function = GetRealAddress((BYTE*)trampoline_function); // This is to avoid vTable

	// Block of memory
	auto page = VirtualAlloc(0, 1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!page || bytes < 14) return nullptr;
	auto page_address = page;

	// Writting function bytes
	memcpy(page, (void*)function, bytes);
	page = reinterpret_cast<void*>((uintptr_t)page + bytes);

	// JMP to function
	memcpy(page, jump_instruction, 6);
	page = reinterpret_cast<void*>((uintptr_t)page + 6);
	size_t func_address = function + bytes;
	memcpy(page, &func_address, 8);
	page = reinterpret_cast<void*>((uintptr_t)page + 8);

	// JMP to our trampoline
	size_t hook_jmp_address = reinterpret_cast<size_t>(page);
	memcpy(page, jump_instruction, 6);
	page = reinterpret_cast<void*>((uintptr_t)page + 6);
	size_t hook_func_address = trampoline_function;
	memcpy(page, &hook_func_address, 8);
	page = reinterpret_cast<void*>((uintptr_t)page + 8);

	// Patch function bytes
	DWORD protecc;
	if (VirtualProtect((size_t*)function, bytes, PAGE_EXECUTE_READWRITE, &protecc))
	{
		memcpy((size_t*)function, jump_instruction, 6);
		memcpy((size_t*)(function + 6), &hook_jmp_address, 8);

		// NOP other bytes, if any
		//memset((size_t*)function + 14, 0x90, bytes - 14)
		const unsigned char nop[] = { 0x90 };
		for (int i = 0; i < bytes - 14; i++)
			*(unsigned char*)(function + 14 + i) = 0x90;

		VirtualProtect((size_t*)function, bytes, protecc, &protecc);
	}
	else
	{
		return nullptr;
	}

	return (T)page_address;
}

template <typename T>
T UnHookFunction(uintptr_t function, T& trampoline, int bytes)
{
	DWORD protecc;
	if (VirtualProtect((size_t*)function, bytes, PAGE_EXECUTE_READWRITE, &protecc))
	{
		memcpy((size_t*)(function), trampoline, bytes);
		VirtualProtect((size_t*)function, bytes, protecc, &protecc);
		VirtualFree((void*)trampoline, 0, MEM_RELEASE);
		return nullptr;
	}

	return trampoline;
}