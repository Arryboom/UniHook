// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include <stdio.h>
#include "Tools.h"
#include "Dissassembly/DissasemblyRoutines.h"
#include "../PolyHook/PolyHook/PolyHook.h"
#include "PDB Query/PDBReader.h"

InstructionSearcher m_InsSearcher;
std::vector<std::shared_ptr<PLH::IHook>> m_Hooks;
std::vector<BYTE*> m_Callbacks;
std::vector<SearchResult> Results;

enum class HookMethod
{
	INLINE,
	INT3_BP
};

#ifdef _WIN64
	#include "HookHandler64.h"
#else
	#include "HookHandler86.h"
#endif
PDBReader m_PDBReader;
__declspec(noinline) volatile void FindSubRoutines()
{
	HANDLE hMod = GetModuleHandle(NULL); //Get Current Process (Base Address)
	IMAGE_DOS_HEADER* DosHeader = (IMAGE_DOS_HEADER*)hMod;
	IMAGE_NT_HEADERS* NTHeader = (IMAGE_NT_HEADERS*)((BYTE*)DosHeader + DosHeader->e_lfanew);
	IMAGE_FILE_HEADER* FileHeader = (IMAGE_FILE_HEADER*)&NTHeader->FileHeader;
	IMAGE_SECTION_HEADER* SectionHeader = (IMAGE_SECTION_HEADER*)IMAGE_FIRST_SECTION(NTHeader);

	//Find all executable code sections
	for (int i = 0; i < FileHeader->NumberOfSections; i++)
	{
		IMAGE_SECTION_HEADER pThisSection = SectionHeader[i];

		//Skip sections that aren't code, and are not executable
		if (!(pThisSection.Characteristics & IMAGE_SCN_CNT_CODE) && !(pThisSection.Characteristics & IMAGE_SCN_MEM_EXECUTE))
			continue;

		DWORD_PTR SectionStart = (DWORD_PTR)((BYTE*)DosHeader + pThisSection.VirtualAddress);
		DWORD_PTR SectionEnd = (DWORD_PTR)((BYTE*)DosHeader + pThisSection.VirtualAddress + pThisSection.SizeOfRawData);
		XTrace("[+] Found Section: %s [%p - %p]\n", SectionHeader[i].Name, SectionStart, SectionEnd);

		Results = m_InsSearcher.SearchForInstruction(INSType::CALL, SectionStart, SectionEnd);
		for (int j = 0; j < Results.size();j++)
		{
			SearchResult SubRoutine = Results[j];
			
			std::string ResolvedName;
			if(m_PDBReader.Enumerate(SubRoutine.GetCallDestination(),ResolvedName))
				XTrace("[+] Found Subroutine [%d] at: [%p] [%s]\n",j,SubRoutine.GetCallDestination(), ResolvedName.c_str());
			else
				XTrace("[+] Found Subroutine [%d] at: [%p] [%s]\n", j, SubRoutine.GetCallDestination(), " ");
		}
		XTrace("[+] Found: %d Subroutines\n", Results.size());
	}
}

DWORD WINAPI InitThread(LPVOID lparam)
{
	CreateConsole();

	//m_PDBReader.LoadFile("C:\\Users\\Steve\\Desktop\\Testing.pdb");

	FindSubRoutines();
	XTrace("[+] 1) Enter an index to hook that function\n");
	XTrace("[+] or 2) Enter the address of a function in hex format 0x........\n");
	XTrace("Make a Selection (Enter 1 or 2): ");
	int Option = atoi(GetConsoleInput().c_str());
	if (Option == 1)
	{
		XTrace("[+] Enter the index of the function:");
		int Selection = atoi(GetConsoleInput().c_str());
		if (Selection > Results.size())
			XTrace("[+] Input index is invalid\n");
		else
			XTrace("[+] Hooking Function:%d\n", Selection);

		HookFunctionAtRuntime((BYTE*)Results[Selection].GetCallDestination(),HookMethod::INLINE);
	}else if (Option == 2) {
		XTrace("[+] Enter the address of the function:");
#ifdef _WIN64
		DWORD64 Address = strtoll(GetConsoleInput().c_str(), NULL, 16);
#else
		DWORD Address = strtol(GetConsoleInput().c_str(), NULL, 16);
#endif
		XTrace("[+] Hooking Function:%p\n",Address);
		HookFunctionAtRuntime((BYTE*)Address,HookMethod::INLINE);
	}
	XTrace("[+] Function is hooked\n");
	return 1;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CloseHandle(CreateThread(NULL, NULL, InitThread, NULL, NULL, NULL));
	case DLL_THREAD_ATTACH:
		
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		for (int i = 0; i < m_Callbacks.size();i++)
		{
			delete[] m_Callbacks[i];
		}
		m_Callbacks.clear();
		break;
	}
	return TRUE;
}

