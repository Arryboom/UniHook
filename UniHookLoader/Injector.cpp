#include "Injector.h"
#include <TlHelp32.h>
Injector::Injector()
{

}

bool Injector::OpenTarget(DWORD PID)
{
	m_Target = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
	if (m_Target == NULL)
		return false;
	return true;
}

bool Injector::OpenTarget(const std::wstring& ProcessName)
{
	auto Processes = GetProcessList();
	for (Process Proc : Processes)
	{
		if(ProcessName != Proc.m_Name)
			continue;

		return OpenTarget(Proc.m_PID);
	}
	return false;
}

bool Injector::Inject(const std::wstring& DllPath)
{
	if (m_Target == NULL)
		return false;

	//Allocate space in target to hold dll path
	size_t PathLength = wcslen(DllPath.c_str()) * sizeof(wchar_t);
	void* Mem = VirtualAllocEx(m_Target, NULL, PathLength, MEM_COMMIT, PAGE_READWRITE);
	auto FreePathMem = finally([&]() {
		VirtualFreeEx(m_Target, Mem, PathLength, MEM_RELEASE);
	});
	if (Mem == NULL)
		return false;

	//Write dll path into the allocated mem
	if (WriteProcessMemory(m_Target, Mem, DllPath.c_str(), PathLength, NULL) == FALSE)
		return false;

	void* LoadLibAddr = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
	if (LoadLibAddr == NULL)
		return false;

	/*Make a thread that calls LoadLibrary, and pass it the path of the dll to load, this
	works because Windows ASLR puts LoadLibrary at the same location in every process*/
	DWORD ThreadId;
	HANDLE hThread = CreateRemoteThread(m_Target, NULL, NULL,(LPTHREAD_START_ROUTINE)LoadLibAddr, Mem, NULL, &ThreadId);
	if (hThread == NULL)
		return false;

	DWORD WaitState =WaitForSingleObject(hThread, 5000);
	CloseHandle(hThread);
	return (WaitState == WAIT_OBJECT_0) ? true:false;
}

std::vector<Process> Injector::GetProcessList()
{
	PROCESSENTRY32 pe;
	HANDLE thSnapShot;
	BOOL CurProc, ProcFound = false;
	std::vector<Process> Processes;

	thSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (thSnapShot == INVALID_HANDLE_VALUE)
	{
		return Processes;
	}
	pe.dwSize = sizeof(PROCESSENTRY32);

	for (CurProc = Process32First(thSnapShot, &pe); CurProc; CurProc = Process32Next(thSnapShot, &pe))
	{
		Process Proc;
		Proc.m_PID = pe.th32ProcessID;
		Proc.m_Name = pe.szExeFile;

		Processes.push_back(Proc);
	}
	return Processes;
}