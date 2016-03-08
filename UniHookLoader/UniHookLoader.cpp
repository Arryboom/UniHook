// UniHookLoader.cpp : Defines the entry point for the console application.
//
#include <Windows.h>
#include "Injector.h"
#include "CmdLineParser.h"

enum Options
{
	OpenProc,
	Inject,
	Help
};

int main(int argc,char* argv[])
{
	Injector WindowsInjector;
	CmdLineParser Parser(argc, argv);
	Parser.RegisterArgs(Options::OpenProc,"-p", "-openproc", Parameter::STRING);
	Parser.RegisterArgs(Options::Inject,"-i", "-inject", Parameter::STRING);
	Parser.RegisterArgs(Options::Help, "-h", "-help", Parameter::NONE);
	Parser.Parse();
	auto Found = Parser.GetFoundArgs();
	for (Command Cmd : Found)
	{
		if (Cmd.m_EnumID == Options::OpenProc)
			WindowsInjector.OpenTarget(StringToWString(Cmd.m_ParamOut));

		if (Cmd.m_EnumID == Options::Inject)
			WindowsInjector.Inject(StringToWString(Cmd.m_ParamOut));

		if (Cmd.m_EnumID == Options::Help)
			printf("-openproc -p <Name of process.exe>\n"
				"-inject   -i <Path to dll, include .dll>\n");
	}
    return 0;
}

