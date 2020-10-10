#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <Psapi.h>

using fnHook = int(__fastcall*)(uintptr_t* pThis, uintptr_t entity_address, DWORD* param_2);
fnHook hook;

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

// can return hook function instead, eg (fnHook)hook = SetUp();
bool SetUp(uintptr_t func_address, uintptr_t hook_address, size_t byte_size)
{
	void* page = nullptr;
	page = VirtualAlloc(NULL, 1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	hook = (fnHook)page;
	if (!page) return false;


	// copy bytes
	memcpy(page, reinterpret_cast<void*>(func_address), byte_size);
	page = reinterpret_cast<void*>((uintptr_t)page + byte_size);

	// add jmp to hook
	const char jump_instruction[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
	memcpy(page, jump_instruction, 6);
	page = reinterpret_cast<void*>((uintptr_t)page + 6);

	size_t func_address_after_bytes = func_address + byte_size;
	memcpy(page, &func_address_after_bytes, 8);
	page = reinterpret_cast<void*>((uintptr_t)page + 8);
	void* page_after = page; // label


	// jump to our hook
	memcpy(page, jump_instruction, 6);
	page = reinterpret_cast<void*>((uintptr_t)page + 6);

	hook_address = GetRealAddress((BYTE*)hook_address);
	size_t hook_func = hook_address;
	memcpy(page, &hook_func, 8);
	page = reinterpret_cast<void*>((uintptr_t)page + 8);


	// patch main functions bytes
	DWORD protecc;
	VirtualProtect((size_t*)func_address, byte_size, PAGE_EXECUTE_READWRITE, &protecc);

	memcpy((size_t*)func_address, jump_instruction, 6);
	size_t page_reutnr = reinterpret_cast<size_t>(page_after);
	memcpy((size_t*)(func_address + 6), &page_reutnr, 8);

	const char nop[] = { 0x90 };
	for (size_t i = 0; i < byte_size - 14; i++)
		memcpy((size_t*)(func_address + 14 + i), nop, 1);

	VirtualProtect((size_t*)func_address, byte_size, protecc, &protecc);

	return true;
}