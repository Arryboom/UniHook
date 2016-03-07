#include "Dissassembly/DissasemblyRoutines.h"
void XTrace(char* lpszFormat, ...);
InstructionSearcher::InstructionSearcher()
{
#ifdef _WIN64
	InitCapstone(CS_MODE_64);
#else
	InitCapstone(CS_MODE_32);
#endif
}

InstructionSearcher::~InstructionSearcher()
{
	cs_close(&m_CapstoneHandle);
}

std::vector<SearchResult> InstructionSearcher::SearchForInstruction(INSType Type, DWORD_PTR StartRange, DWORD_PTR EndRange)
{
	cs_insn* InstructionInfo;
	size_t InstructionCount = cs_disasm(m_CapstoneHandle, (BYTE*)StartRange, EndRange-StartRange, (uint64_t)StartRange, 0, &InstructionInfo);

	for (int i = 0; i < InstructionCount; i++)
	{
		cs_insn* CurIns = (cs_insn*)&InstructionInfo[i];
		cs_x86* x86 = &(CurIns->detail->x86);

		for (int j = 0; j < x86->op_count; j++)
		{
			cs_x86_op* op = &(x86->operands[j]);
			if (op->type == X86_OP_IMM) 
			{
				//IMM types are like call 0xdeadbeef
				if (x86->op_count > 1) //exclude types like sub rsp,0x20
					continue;

				//Hex compare is too inaccurate, have to do strcmp
				if (strcmp(CurIns->mnemonic, "call") != 0)
					continue;

				DWORD_PTR CallDestination = m_ASMHelper.GetCallDestination(CurIns, x86->offsets.imm_size, x86->offsets.imm_offset);
				
				bool Found = false;
				for (SearchResult Item : m_Results)
				{
					if (Item.GetCallDestination() == CallDestination)
						Found = true;
				}
				if(!Found)
					m_Results.push_back(SearchResult(CurIns->address,CallDestination));
			}
		}
	}
	cs_free(InstructionInfo, InstructionCount);
	return m_Results;
}

void InstructionSearcher::InitCapstone(cs_mode Mode)
{
	if (cs_open(CS_ARCH_X86, Mode, &m_CapstoneHandle) != CS_ERR_OK)
		XTrace("[+]Error Initializing Capstone\n");
	cs_option(m_CapstoneHandle, CS_OPT_DETAIL, CS_OPT_ON);
}