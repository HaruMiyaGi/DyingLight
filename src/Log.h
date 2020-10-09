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
	}
	~Logger()
	{
		std::cout << "Bye Bye\n";
		FreeConsole();
	}
private:
	FILE* pFile = nullptr;
};