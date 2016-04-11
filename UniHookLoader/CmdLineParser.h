#pragma once
#include <vector>
#include <string>
#include <codecvt>
enum class Parameter
{
	STRING,
	NONE
};

std::string WStringToString(std::wstring& str)
{
	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;
	return converter.to_bytes(str);
}

std::wstring StringToWString(std::string& str)
{
	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;
	return converter.from_bytes(str);
}

struct Command
{
	std::string m_ShortName;
	std::string m_LongName;
	Parameter m_Param;
	std::string m_ParamOut;
	DWORD m_EnumID;
};

class CmdLineParser
{
public:
	CmdLineParser(int ArgCount, char** Args);
	void ResetArguments(int ArgCount, char** Args);
	void ResetArguments(const std::vector<std::string>& Args);
	void RegisterArgs(DWORD EnumID,const std::string& ShortName, const std::string& LongName, Parameter Param);
	void Parse();
	std::vector<Command> GetFoundArgs();
private:
	std::vector<std::string> m_Args;
	std::vector<Command> m_RegisteredArgs;
	std::vector<Command> m_FoundArgs;
};

CmdLineParser::CmdLineParser(int ArgCount, char** Args)
{
	for (int i = 0; i < ArgCount; i++)
	{
		m_Args.push_back(Args[i]);
	}
}

void CmdLineParser::ResetArguments(int ArgCount, char** Args)
{
	m_Args.clear();
	m_FoundArgs.clear();
	for (int i = 0; i < ArgCount; i++)
	{
		m_Args.push_back(Args[i]);
	}
}

void CmdLineParser::ResetArguments(const std::vector<std::string>& Args)
{
	m_Args.clear();
	m_FoundArgs.clear();
	for (std::string Arg : Args)
	{	
		m_Args.push_back(Arg);
	}
}

void CmdLineParser::RegisterArgs(DWORD EnumID,const std::string& ShortName, const std::string& LongName, Parameter Param)
{
	Command Cmd;
	Cmd.m_ShortName = ShortName;
	Cmd.m_LongName = LongName;
	Cmd.m_Param = Param;
	Cmd.m_EnumID = EnumID;
	m_RegisteredArgs.push_back(Cmd);
}

void CmdLineParser::Parse()
{
	for (int i = 0; i < m_Args.size();i++)
	{
		std::string Arg = m_Args[i];
		for (int j = 0; j < m_RegisteredArgs.size();j++)
		{
			std::string ShortName = m_RegisteredArgs[j].m_ShortName;
			std::string LongName = m_RegisteredArgs[j].m_LongName;
			if(ShortName != Arg && LongName != Arg)
				continue;
		
			//Make a copy
			Command FoundCommand = m_RegisteredArgs[j];

			if (FoundCommand.m_Param == Parameter::NONE)
			{
				m_FoundArgs.push_back(FoundCommand);
				continue;
			}

			//Next Index is a parameter
			if (++i >= m_Args.size())
			{
				FoundCommand.m_ParamOut = "";
				m_FoundArgs.push_back(FoundCommand);
				continue;
			}
			Arg = m_Args[i];

			FoundCommand.m_ParamOut = Arg;
			m_FoundArgs.push_back(FoundCommand);
		}
	}
}

std::vector<Command> CmdLineParser::GetFoundArgs()
{
	return m_FoundArgs;
}