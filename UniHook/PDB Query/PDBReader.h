#pragma once
//https://msdn.microsoft.com/en-us/library/t6tay6cz.aspx
#include <string>
#include <DbgHelp.h>
class PDBReader
{
public:
	PDBReader();
	bool LoadFile(const std::string& FilePath);
	bool Enumerate(DWORD_PTR Address, std::string& NameOut);
private:
	bool m_InitOk;
};

PDBReader::PDBReader()
{
	m_InitOk = SymInitialize(GetCurrentProcess(), NULL, TRUE) ? true:false;
	if(!m_InitOk)
		XTrace("SymInit Failed\n");
}

bool PDBReader::LoadFile(const std::string& FilePath)
{
	if (!m_InitOk)
		return false;

	return SymSetSearchPath(GetCurrentProcess(), FilePath.c_str()) ? true:false;
}

bool PDBReader::Enumerate(DWORD_PTR Address,std::string& NameOut)
{
	if (!m_InitOk)
		return false;

	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;

	if (SymFromAddr(GetCurrentProcess(), Address, 0, pSymbol))
	{
		NameOut = std::string((char*)pSymbol->Name);
		return true;
	}else {
		NameOut = std::string("");
		return false;
	}
}