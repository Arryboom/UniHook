// dllmain.cpp : Defines the entry point for the DLL application.
// git subtree pull --prefix=PolyHook PolyHook master --squash
#include <Windows.h>
#include <stdio.h>
#include "Tools.h"
#include "../PolyHook/PolyHook/PolyHook.h"
#include "Dissassembly/DissasemblyRoutines.h"
#include "PDB Query/PDBReader.h"
#include "../Common/IPC/SharedMemQueue.h"
#include "../Common/Utilities.h"

InstructionSearcher m_InsSearcher;
std::vector<std::shared_ptr<PLH::IHook>> m_Hooks;
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
SharedMemQueue* MemClient;
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
		cPrint("[+] Found Section: %s [%p - %p]\n", SectionHeader[i].Name, SectionStart, SectionEnd);

		Results = m_InsSearcher.SearchForInstruction(INSType::CALL, SectionStart, SectionEnd);
		for (int j = 0; j < Results.size();j++)
		{
			SearchResult SubRoutine = Results[j];
			
			std::string ResolvedName;
			if (m_PDBReader.Enumerate(SubRoutine.GetCallDestination(), ResolvedName))
			{
				cPrint("[+] Found Subroutine [%d] at: [%p] [%s]\n", j, SubRoutine.GetCallDestination(), ResolvedName.c_str());
				
				char Buf[64];
				snprintf(Buf, 64, "[%d] at: [%p] [%s]", j, SubRoutine.GetCallDestination(), ResolvedName.c_str());
				MemClient->PushMessage((BYTE*)Buf);
			}else {
				cPrint("[+] Found Subroutine [%d] at: [%p] [%s]\n", j, SubRoutine.GetCallDestination(), " ");

				char Buf[64];
				snprintf(Buf, 64, "[%d] at: [%p] [%s]", j, SubRoutine.GetCallDestination(), " ");
				MemClient->PushMessage((BYTE*)Buf);
			}
		}
		cPrint("[+] Found: %d Subroutines\n", Results.size());
		char Buf[64];
		snprintf(Buf, 64, "Found %d Subroutines", Results.size());
		MemClient->PushMessage((BYTE*)Buf);
	}
}

DWORD WINAPI InitThread(LPVOID lparam)
{
	CreateConsole();
	MemClient = new SharedMemQueue("Local\\UniHook_IPC", 4096, SharedMemQueue::Mode::Client);
	MemMessage Msg;
	if (MemClient->PopMessage(Msg))
		printf("%s\n", Msg.m_Data);
	else
		printf("[+] IPC FAILED\n");

	//m_PDBReader.LoadFile("C:\\Users\\Steve\\Desktop\\Testing.pdb");

	do 
	{
		MemClient->WaitForMessage();
		MemMessage Msg;
		if (!MemClient->PopMessage(Msg))
			continue;

		//Convert message to string
		std::string Cmd((char*)Msg.m_Data, sizeof(MemMessage::m_Data));
		if (strcmp(Cmd.c_str(), "ListSubs") == 0)
		{
			cPrint("[+] Executing Command: %s\n", Cmd.c_str());
			FindSubRoutines();
		}

		//These types of messages have two parts, split by :
		std::vector<std::string> SplitCmd = split(Cmd, ":");
		if (SplitCmd.size() == 2)
		{
			if (strcmp(SplitCmd[0].c_str(), "HookAtIndex") == 0)
			{
				if (Results.size() < 1)
					FindSubRoutines();

				int Index = atoi(SplitCmd[1].c_str());
				cPrint("[+] Hooking Function at index:%d\n", Index);
				HookFunctionAtRuntime((BYTE*)Results[Index].GetCallDestination(), HookMethod::INLINE);

				char Buf[64];
				snprintf(Buf, 64, "Hooking Function at:%d", Index);
				MemClient->PushMessage((BYTE*)Buf);
			}
			else if (strcmp(SplitCmd[0].c_str(), "HookAtAddr") == 0)
			{
#ifdef _WIN64
				DWORD64 Address = strtoll(SplitCmd[1].c_str(), NULL, 16);
#else
				DWORD Address = strtol(SplitCmd[1].c_str(), NULL, 16);
#endif
				cPrint("[+] Hooking Function:%p\n", Address);
				HookFunctionAtRuntime((BYTE*)Address, HookMethod::INLINE);

				char Buf[64];
				snprintf(Buf, 64, "Hooking Function at:%p", Address);
				MemClient->PushMessage((BYTE*)Buf);
			}
		}
	} while (1);
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
		break;
	}
	return TRUE;
}

