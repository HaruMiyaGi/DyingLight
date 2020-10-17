#pragma once
#include <Windows.h>
#include <iostream>

class Logger
{
public:
	Logger()
	{
		AllocConsole();
		freopen_s(&pFile, "CONOUT$", "w", stdout);
		freopen_s(&pFile, "CONIN$", "r", stdin);
	}
	~Logger()
	{
		std::cout << "Bye Bye\n";
		FreeConsole();
	}
private:
	FILE* pFile = nullptr;
};