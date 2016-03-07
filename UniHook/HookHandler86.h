#pragma once

__declspec(noinline) void __stdcall Interupt1()
{
	cPrint("[+] In Interupt1\n");
}

__declspec(noinline) void __stdcall Interupt2()
{
	cPrint("[+] In Interupt2\n");
}

template<typename T>
T CalculateRelativeDisplacement(DWORD64 From, DWORD64 To, DWORD InsSize)
{
	if (To < From)
		return 0 - (From - To) - InsSize;
	return To - (From + InsSize);
}

volatile int WriteRelativeCALL(DWORD Destination, DWORD JMPDestination)
{
	*(BYTE*)Destination = 0xE8; 
	*(long*)(Destination + 1) = CalculateRelativeDisplacement<long>(Destination, JMPDestination, 5);
	return 5;
}

volatile int WritePUSHA(BYTE* Address)
{
	BYTE PUSHA[] = { 0x60, 0x9C };
	memcpy(Address, PUSHA, sizeof(PUSHA));
	return sizeof(PUSHA);
}

volatile int WritePOPA(BYTE* Address)
{
	BYTE POPA[] = { 0x9D, 0x61 };
	memcpy(Address, POPA, sizeof(POPA));
	return sizeof(POPA);
}

volatile int WriteRET(BYTE* Address)
{
	BYTE ret[] = { 0xC3 };
	memcpy(Address, ret, sizeof(ret));
	return sizeof(ret);
}

void HookFunctionAtRuntime(BYTE* SubRoutineAddress,HookMethod Method)
{
	BYTE* Callback = new BYTE[1024];
	DWORD Old;
	VirtualProtect(Callback, 1024, PAGE_EXECUTE_READWRITE, &Old);

	std::shared_ptr<PLH::IHook> Hook;
	DWORD Original;
	if (Method == HookMethod::INLINE)
	{
		Hook.reset(new PLH::Detour, [](PLH::Detour* Hook) {
			Hook->UnHook();
			delete Hook;
		});
		((PLH::Detour*)Hook.get())->SetupHook((BYTE*)SubRoutineAddress, (BYTE*)Callback);
		Hook->Hook();
		Original =((PLH::Detour*)Hook.get())->GetOriginal<DWORD>();
	}else if (Method == HookMethod::INT3_BP){
		Hook.reset(new PLH::VEHHook, [](PLH::VEHHook* Hook) {
			Hook->UnHook();
			delete Hook;
		});
		((PLH::VEHHook*)Hook.get())->SetupHook((BYTE*)SubRoutineAddress, (BYTE*)Callback,PLH::VEHHook::VEHMethod::INT3_BP);
		Hook->Hook();
		Original = ((PLH::VEHHook*)Hook.get())->GetOriginal<DWORD>();
	}
	
	int WriteOffset = 0;
	WriteOffset += WritePUSHA(Callback);
	WriteOffset += WriteRelativeCALL((DWORD)Callback + WriteOffset, (DWORD)&Interupt1);
	WriteOffset += WritePOPA(Callback + WriteOffset);

	WriteOffset += WriteRelativeCALL((DWORD)Callback + WriteOffset, Original);

	WriteOffset += WritePUSHA(Callback+WriteOffset);
	WriteOffset += WriteRelativeCALL((DWORD)Callback + WriteOffset, (DWORD)&Interupt2);
	WriteOffset += WritePOPA(Callback + WriteOffset);

	WriteOffset += WriteRET(Callback + WriteOffset);

	cPrint("[+] Callback at: %p\n", Callback);
	m_Hooks.push_back(Hook);
	m_Callbacks.push_back(Callback);
}