// UniHookLoader.cpp : Defines the entry point for the console application.
//
#include <Windows.h>
#include "Injector.h"

int main(int argc,char* argv[])
{
	Injector WindowsInjector;
	if (WindowsInjector.OpenTarget(L"Testingx86.exe"))
		printf("Opened Target\n");
	if (WindowsInjector.Inject(L"C:\\Visual Studio 2015\\Git Source Control\\UniHook\\Release\\UniHook.dll"))
		printf("Inject Properly\n");

    return 0;
}

