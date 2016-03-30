// UniHookLoader.cpp : Defines the entry point for the console application.
//
#include <Windows.h>
#include "Injector.h"
#include "CmdLineParser.h"
#include "../Common/IPC/SharedMemQueue.h"
#include "../Common/Utilities.h"
#include <iostream>

#define USE_STDIN 1
enum Options
{
	OpenProc, //Open an existing process
	Inject,   //Inject into the process opened by OpenProc
	OpenProcPath, //Launch not yet open process
	ProcExit, //Kills the process that was opened
	ListSubroutines,  //List subroutines in the injected process
	HookSubroutineAtIndex,  //Hook subroutine at index from ListSubs
	HookSubAtAddress, //Hook subroutine at address
	HookSubMultiple, //Hook multiple subroutines, pass it path to .txt file
	Exit, //Close injector
	Help
};

//Shared memory IPC mechanism to talk to our injected DLL
SharedMemQueue MemServer("Local\\UniHook_IPC", 100000, SharedMemQueue::Mode::Server);
Injector WindowsInjector;
bool ShouldExit = false;
void PrintDllMessages()
{
	MemServer.WaitForMessage();
	MemMessage Msg;
	while (MemServer.PopMessage(Msg))
	{
		printf("From Client:%s\n", &Msg.m_Data[0]);
	}
}

void ExecuteCommands(std::vector<Command>& Commands)
{
	bool WaitForMsg = true;
	for (Command Cmd : Commands)
	{
		if (Cmd.m_EnumID == Options::OpenProc)
		{
			WindowsInjector.OpenTarget(StringToWString(Cmd.m_ParamOut));
			WaitForMsg = false;
		}

		else if (Cmd.m_EnumID == Options::OpenProcPath)
		{
			WindowsInjector.OpenTargetPath(StringToWString(Cmd.m_ParamOut));
			WaitForMsg = false;
		}

		else if (Cmd.m_EnumID == Options::ProcExit)
		{
			WindowsInjector.KillTarget();
			WaitForMsg = false;
		}

		else if (Cmd.m_EnumID == Options::Inject)
		{
			WindowsInjector.Inject(StringToWString(Cmd.m_ParamOut));
			WaitForMsg = false;
			MemServer.WaitForMessage();
			MemMessage Msg;
			if (MemServer.PopMessage(Msg))
				printf("[+] %s\n", &Msg.m_Data[0]);
			else
				printf("[+] Dll IPC Failed\n");
		}

		else if (Cmd.m_EnumID == Options::Help)
		{
			printf("-openproc -p <Name of process.exe>\n"
				"-inject   -i <Path to dll, include .dll>\n");
			WaitForMsg = false;
		}

		else if (Cmd.m_EnumID == Options::Exit)
		{
			ShouldExit = true;
			WaitForMsg = false;
		}

		//Commands below here are sent to our dll
		else if (Cmd.m_EnumID == Options::ListSubroutines)
		{
			printf("Sending Message to Dll: ListSubs\n");
			MemServer.PushMessage(MemMessage("ListSubs"));
		}

		else if (Cmd.m_EnumID == Options::HookSubAtAddress)
		{
			printf("Sending Message to Dll: Hook At Address\n");
			std::string Msg(std::string("HookAtAddr[:.") + Cmd.m_ParamOut);
			MemServer.PushMessage(MemMessage(Msg));
		}

		else if (Cmd.m_EnumID == Options::HookSubroutineAtIndex)
		{
			printf("Sending Message to Dll: Hook At Index\n");
			std::string Msg(std::string("HookAtIndex[:.") + Cmd.m_ParamOut);
			MemServer.PushMessage(MemMessage(Msg));
		}

		else if (Cmd.m_EnumID == Options::HookSubMultiple)
		{
			printf("Sending Message to Dll: Hook Multiple Subroutines\n");
			std::string Msg(std::string("HookMultiple[:.") + Cmd.m_ParamOut);
			MemServer.PushMessage(MemMessage(Msg));
		}else {
			WaitForMsg = false; //No Command sent, so no messages
		}
	}
	if (WaitForMsg)
		PrintDllMessages();
}

int main(int argc,char* argv[])
{
	MemServer.PushMessage(MemMessage("IPC Connection Initialized!"));

	//Read Command Line Arguments, then execute if found
	CmdLineParser Parser(argc, argv);
	Parser.RegisterArgs(Options::OpenProc,"-p", "-openproc", Parameter::STRING);
	Parser.RegisterArgs(Options::Inject,"-i", "-inject", Parameter::STRING);
	Parser.RegisterArgs(Options::ProcExit, "-pe", "-procexit", Parameter::NONE);
	Parser.RegisterArgs(Options::Help, "-h", "-help", Parameter::NONE);
	Parser.RegisterArgs(Options::ListSubroutines, "-ls", "-listsubs", Parameter::NONE);
	Parser.RegisterArgs(Options::HookSubAtAddress, "-hsa", "-hooksuba", Parameter::STRING);
	Parser.RegisterArgs(Options::HookSubroutineAtIndex, "-hsi", "-hooksubi", Parameter::STRING);
	Parser.RegisterArgs(Options::Exit, "-x", "-exit", Parameter::NONE);
	Parser.RegisterArgs(Options::OpenProcPath, "-pp", "-openprocpath", Parameter::STRING);
	Parser.RegisterArgs(Options::HookSubMultiple, "-hsm", "-hooksubm", Parameter::STRING);
	Parser.Parse();
	ExecuteCommands(Parser.GetFoundArgs());

#if !USE_STDIN
	return 0;
#endif

	do 
	{
		std::string Input;
		std::cout << "Enter Command: ";
		std::getline(std::cin, Input);

		Parser.ResetArguments(split(Input, " "));
		Parser.Parse();
		ExecuteCommands(Parser.GetFoundArgs());
	} while (!ShouldExit);
    return 0;
}

