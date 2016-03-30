#pragma once

#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <string>
#include <algorithm>
static BOOL WINAPI MyConsoleCtrlHandler(DWORD dwCtrlEvent) { return dwCtrlEvent == CTRL_C_EVENT; }

static HANDLE hConsoleWrite;
static HANDLE hConsoleRead;
void CreateConsole()
{
	AllocConsole();
	hConsoleWrite = GetStdHandle(STD_OUTPUT_HANDLE);
	hConsoleRead = GetStdHandle(STD_INPUT_HANDLE);
}

//Waits for enter
std::string GetConsoleInput()
{
	char Buffer[1024];
	DWORD num;
	ReadConsoleA(hConsoleRead, Buffer, 1023, &num, NULL);
	Buffer[num] = '\0';
	std::string stdbuff(Buffer);
	stdbuff.erase(std::remove(stdbuff.begin(), stdbuff.end(), '\n'), stdbuff.end());
	stdbuff.erase(std::remove(stdbuff.begin(), stdbuff.end(), '\r'), stdbuff.end());
	return stdbuff;
}

void cPrint(char* lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);
	int nBuf;
	char szBuffer[512];
	nBuf = _vsnprintf_s(szBuffer, 511, lpszFormat, args);
	va_end(args);
#if !USE_OUTPUT
	return;
#endif
	DWORD CharsWritten = 0;
	WriteConsole(hConsoleWrite, szBuffer, nBuf, &CharsWritten, NULL);
}
