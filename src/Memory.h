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