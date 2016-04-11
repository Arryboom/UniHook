#pragma once
#include "../PolyHook/Capstone/include/capstone.h"
#pragma comment(lib,"capstone.lib")
#include <vector>
#include <Windows.h>
enum class INSType
{
	GENERIC,
	CALL
};

class SearchResult
{
public:
	SearchResult(DWORD_PTR InsAddress, DWORD_PTR CallAddress)
	{
		m_InstructionAddress = InsAddress;
		m_CallDestination = CallAddress;
	}

	DWORD_PTR GetCallDestination() const
	{
		return m_CallDestination;
	}

	bool operator==(const SearchResult& rhs) const
	{
		if (GetCallDestination() == rhs.GetCallDestination())
			return true;
		return false;
	}
protected:
	DWORD_PTR m_InstructionAddress;
	DWORD_PTR m_CallDestination;
};

class ASMHelper
{
public:
	bool IsCall(const BYTE* bytes, const uint16_t Size)
	{
		if (Size < 1)
			return false;

		if (bytes[0] == 0xE8)
			return true;
	}
	template<typename T>
	T GetDisplacement(BYTE* Instruction, const uint32_t Offset)
	{
		T Disp;
		memset(&Disp, 0x00, sizeof(T));
		memcpy(&Disp, &Instruction[Offset], sizeof(T));
		return Disp;
	}
	DWORD_PTR GetCallDestination(cs_insn* CurIns, const uint8_t DispSize, const uint8_t DispOffset)
	{
		if (DispSize == 1)
		{
			int8_t Disp = GetDisplacement<int8_t>(CurIns->bytes, DispOffset);
			return CurIns->address + Disp + CurIns->size;
		}
		else if (DispSize == 2) {
			int16_t Disp = GetDisplacement<int16_t>(CurIns->bytes, DispOffset);
			return CurIns->address + Disp + CurIns->size;
		}else if (DispSize == 4) {
			int32_t Disp = GetDisplacement<int32_t>(CurIns->bytes, DispOffset);
			return CurIns->address + Disp + CurIns->size;
		}
	}
};

class InstructionSearcher
{
public:
	InstructionSearcher();
	~InstructionSearcher();
	std::vector<SearchResult> SearchForInstruction(INSType Type, DWORD_PTR StartRange, DWORD_PTR EndRange);
protected:
	void InitCapstone(cs_mode Mode);
private:
	csh m_CapstoneHandle;
	std::vector<SearchResult> m_Results;
	ASMHelper m_ASMHelper;
};