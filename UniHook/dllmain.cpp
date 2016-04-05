// dllmain.cpp : Defines the entry point for the DLL application.
// git subtree pull --prefix=PolyHook PolyHook master --squash
#include <Windows.h>
#include <stdio.h>
#include <fstream>
#include <sstream>

#define USE_OUTPUT 1

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
#define StrToAddress(x) strtoll(x,NULL,16)
#else
#define StrToAddress(x) strtol(x,NULL,16)
	#include "HookHandler86.h"
#endif
PDBReader m_PDBReader;
std::unique_ptr<SharedMemQueue> MemClient;
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
	}
}

void PrintFoundSubs()
{
	MemClient->ManualLock();
	auto CloseLock = PLH::finally([&] {
		//Ensures we close lock even with exceptions
		MemClient->ManualUnlock();
	});
	for (int j = 0; j < Results.size(); j++)
	{
		SearchResult SubRoutine = Results[j];

		std::string ResolvedName;
		if (m_PDBReader.Enumerate(SubRoutine.GetCallDestination(), ResolvedName))
		{
			cPrint("[+] Found Subroutine [%d] at: [%p] [%s]\n", j, SubRoutine.GetCallDestination(), ResolvedName.c_str());

			MemMessage Msg("[%d] at: [%p] [%s]", j, SubRoutine.GetCallDestination(), ResolvedName.c_str());
			MemClient->PushMessage(Msg, true);
		}else {
			cPrint("[+] Found Subroutine [%d] at: [%p] [%s]\n", j, SubRoutine.GetCallDestination(), " ");

			MemMessage Msg("[%d] at: [%p] [%s]", j, SubRoutine.GetCallDestination(), " ");
			MemClient->PushMessage(Msg, true);
		}
	}
	cPrint("[+] Found: %d Subroutines\n", Results.size());
	MemMessage Msg("Found %d Subroutines", Results.size());
	MemClient->PushMessage(Msg, true);
}

void ParseAndExecuteCommands()
{
	MemMessage Msg;
	if (!MemClient->PopMessage(Msg))
		return;

	//Convert message to string
	std::string Cmd((char*)&Msg.m_Data[0], Msg.m_DataSize);
	if (strcmp(Cmd.c_str(), "ListSubs") == 0)
	{
		cPrint("[+] Executing Command: %s\n", Cmd.c_str());
		FindSubRoutines();
		PrintFoundSubs();
	}

	//These types of messages have two parts, split by :
	std::vector<std::string> SplitCmd = split(Cmd, "[:.");
	if (SplitCmd.size() == 2)
	{
		if (strcmp(SplitCmd[0].c_str(), "HookAtIndex") == 0)
		{
			if (Results.size() < 1)
				FindSubRoutines();

			int Index = atoi(SplitCmd[1].c_str());
			cPrint("[+] Hooking Function at index:%d\n", Index);
			HookFunctionAtRuntime((BYTE*)Results[Index].GetCallDestination(), HookMethod::INLINE);

			MemMessage Msg("Hooking Function at:%d", Index);
			MemClient->PushMessage(Msg);
		}
		else if (strcmp(SplitCmd[0].c_str(), "HookAtAddr") == 0)
		{
			DWORD_PTR Address = StrToAddress(SplitCmd[1].c_str());

			cPrint("[+] Hooking Function:%p\n", Address);
			HookFunctionAtRuntime((BYTE*)Address, HookMethod::INLINE);

			MemMessage Msg("Hooking Function at:%p", Address);
			MemClient->PushMessage(Msg);
		}
		else if (strcmp(SplitCmd[0].c_str(), "HookMultiple") == 0)
		{
			//Remove quotes from path
			std::string path = SplitCmd[1];
			path.erase(std::remove(path.begin(), path.end(), '"'), path.end());

			std::ifstream File(path);
			std::string line;
			while (std::getline(File, line))
			{
				std::vector<std::string> Split = split(line, " ");
				if (Split.size() != 2)
					continue;

				if (strcmp(Split[0].c_str(), "Index") == 0)
				{
					int Index = atoi(Split[1].c_str());
					if (Results.size() < 1)
						FindSubRoutines();

					cPrint("Index:%d\n", Index);
					HookFunctionAtRuntime((BYTE*)Results[Index].GetCallDestination(), HookMethod::INLINE);
				}
				else if (strcmp(Split[0].c_str(), "Address") == 0)
				{
					cPrint("Address:%p\n", StrToAddress(Split[1].c_str()));
					HookFunctionAtRuntime((BYTE*)StrToAddress(Split[1].c_str()), HookMethod::INLINE);
				}
			}
		}
	}
}

void ReceivedSharedMsg()
{
	ParseAndExecuteCommands();
}

DWORD WINAPI InitThread(LPVOID lparam)
{
#if USE_OUTPUT
	CreateConsole();
#endif
	MemClient.reset(new SharedMemQueue("Local\\UniHook_IPC", 100000, SharedMemQueue::Mode::Client));
	MemClient->SetCallback(ReceivedSharedMsg);
	
	//m_PDBReader.LoadFile("C:\\Users\\Steve\\Desktop\\Testing.pdb");
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

