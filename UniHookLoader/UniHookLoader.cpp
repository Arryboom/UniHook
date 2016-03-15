// UniHookLoader.cpp : Defines the entry point for the console application.
//
#include <Windows.h>
#include "Injector.h"
#include "CmdLineParser.h"
#include "../Common/IPC/SharedMemQueue.h"
#include "../Common/Utilities.h"
#include <iostream>
enum Options
{
	OpenProc,
	Inject,
	ListSubroutines,
	HookSubroutineAtIndex,
	HookSubAtAddress,
	Exit,
	Help
};

//Shared memory IPC mechanism to talk to our injected DLL
SharedMemQueue MemServer("Local\\UniHook_IPC", 4096, SharedMemQueue::Mode::Server);
Injector WindowsInjector;
bool ShouldExit = false;
void ExecuteCommands(std::vector<Command>& Commands)
{
	for (Command Cmd : Commands)
	{
		if (Cmd.m_EnumID == Options::OpenProc)
			WindowsInjector.OpenTarget(StringToWString(Cmd.m_ParamOut));

		if (Cmd.m_EnumID == Options::Inject)
			WindowsInjector.Inject(StringToWString(Cmd.m_ParamOut));

		if (Cmd.m_EnumID == Options::Help)
			printf("-openproc -p <Name of process.exe>\n"
				"-inject   -i <Path to dll, include .dll>\n");

		if (Cmd.m_EnumID == Options::Exit)
			ShouldExit = true;

		//Commands below here are sent to our dll
		if (Cmd.m_EnumID == Options::ListSubroutines)
		{
			printf("Sending Message to Dll: ListSubs\n");
			MemServer.PushMessage((BYTE*)"ListSubs");
		}

		if (Cmd.m_EnumID == Options::HookSubAtAddress)
		{
			printf("Sending Message to Dll: Hook At Address\n");
			MemServer.PushMessage((BYTE*) (std::string("HookAtAddr:") + Cmd.m_ParamOut).c_str() );
		}

		if (Cmd.m_EnumID == Options::HookSubroutineAtIndex)
		{
			printf("Sending Message to Dll: Hook At Index\n");
			MemServer.PushMessage((BYTE*)(std::string("HookAtIndex:") + Cmd.m_ParamOut).c_str());
		}
	}
}

int main(int argc,char* argv[])
{
	MemServer.PushMessage((BYTE*)"IPC Mechanism Initialized!");

	//Read Command Line Arguments, then execute if found
	CmdLineParser Parser(argc, argv);
	Parser.RegisterArgs(Options::OpenProc,"-p", "-openproc", Parameter::STRING);
	Parser.RegisterArgs(Options::Inject,"-i", "-inject", Parameter::STRING);
	Parser.RegisterArgs(Options::Help, "-h", "-help", Parameter::NONE);
	Parser.RegisterArgs(Options::ListSubroutines, "-ls", "-listsubs", Parameter::NONE);
	Parser.RegisterArgs(Options::HookSubAtAddress, "-hsa", "-hooksuba", Parameter::STRING);
	Parser.RegisterArgs(Options::HookSubroutineAtIndex, "-hsi", "-hooksubi", Parameter::STRING);
	Parser.RegisterArgs(Options::Exit, "-x", "-exit", Parameter::NONE);
	Parser.Parse();
	ExecuteCommands(Parser.GetFoundArgs());

	do 
	{
		std::string Input;
		std::cout << "Enter Command: ";
		std::getline(std::cin, Input);

		Parser.ResetArguments(split(Input, " "));
		Parser.Parse();
		ExecuteCommands(Parser.GetFoundArgs());

		MemServer.WaitForMessage();
		MemMessage Msg;
		while (MemServer.PopMessage(Msg))
		{
			printf("From Client:%s\n", Msg.m_Data);
		}
	} while (!ShouldExit);
    return 0;
}

