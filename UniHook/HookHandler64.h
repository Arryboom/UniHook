#pragma once
#include "ittnotify.h"

__itt_domain* g_domain = __itt_domain_createA("PolyHook");

typedef void(__stdcall* tGeneric)();
__declspec(noinline) void PrologInterupt(void* pOriginal)
{
	cPrint("[+] In Prolog, pOriginal:[%I64X]\n",pOriginal);
    __itt_task_begin_fn(g_domain, __itt_null, __itt_null, pOriginal); //needs only function pointer and gets the info from symbols
}

__declspec(noinline) void PostlogInterupt(PLH::IHook* pHook)
{
    __itt_task_end(g_domain);
    if (pHook->GetType() == PLH::HookType::VEH)
	{
		auto ProtectionObject = ((PLH::VEHHook*)pHook)->GetProtectionObject();
	}
	cPrint("[+] In Postlog\n");
}

BYTE ABS_JMP_ASM[] = { 0x50, 0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x48, 0x87, 0x04, 0x24, 0xC3 };
volatile int WriteAbsoluteJMP(BYTE* Destination, DWORD64 JMPDestination)
{
	/*push rax
	mov rax ...   //Address to original
	xchg qword ptr ss:[rsp], rax
	ret*/
	memcpy(Destination, ABS_JMP_ASM, sizeof(ABS_JMP_ASM));
	*(DWORD64*)&((BYTE*)Destination)[3] = JMPDestination;
	return sizeof(ABS_JMP_ASM);
}

volatile int WriteAbsoluteCall(BYTE* Destination, DWORD64 JMPDestination)
{
	/*
	mov rax, ...
	call rax
	*/
	BYTE call[] = {0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xFF, 0xD0 };
	memcpy(Destination, call, sizeof(call));
	*(DWORD64*)&((BYTE*)Destination)[2] = JMPDestination;
	return sizeof(call);
}

volatile int WritePUSHA(BYTE* Address)
{
	/*
	push	rbx
	push	rsp
	push	rax
	push	rcx
	push	rdx
	push	r8
	push	r9
	push	r10
	push	r11
	sub	rsp, 0x20
	*/
	BYTE X64PUSHFA[] = { 0x53, 0x54, 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53, 0x48, 0x83, 0xEC, 0x20 };
	memcpy(Address, X64PUSHFA, sizeof(X64PUSHFA));
	return sizeof(X64PUSHFA);
}

volatile int WritePOPA(BYTE* Address)
{
	/*
	add rsp,0x20
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdx
	pop rcx
	pop rax
	pop rsp
	pop rbx
	*/
	BYTE X64POPFA[] = { 0x48, 0x83, 0xC4, 0x20, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58, 0x5C, 0x5B };
	memcpy(Address, X64POPFA, sizeof(X64POPFA));
	return sizeof(X64POPFA);
}

volatile int WritePUSHA_WPARAM(BYTE* Address, __int64 RCXVal)
{
	/*
	PUSHA From above 
	+
	movabs rcx,0xCCCCCCCCCCCCCCCC
	sub rsp, 0x20
	*/
	BYTE X64PUSHFA[] = { 0x53, 0x54, 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 
		0x41, 0x52, 0x41, 0x53, 0x48, 0xB9, 
		0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 
		0x48, 0x83, 0xEC, 0x20 };
	memcpy(Address, X64PUSHFA, sizeof(X64PUSHFA));
	*(DWORD64*)&((BYTE*)Address)[15] = RCXVal;
	return sizeof(X64PUSHFA);
}

#define LODWORD(_qw)    ((DWORD)(_qw))
#define HIDWORD(_qw)    ((DWORD)(((_qw) >> 32) & 0xffffffff))
BYTE PushRet[] = { 0x48, 0x83, 0xEC, 0x08, 0xC7, 0x04, 0x24, 0xCC, 0xCC, 0xCC, 0xCC, 0xC7, 0x44, 0x24, 0x04, 0xCC, 0xCC, 0xCC, 0xCC };
volatile int WriteRetAddress(BYTE* Address, DWORD64 Destination)
{
	/*
	sub rsp,8
	mov dword ptr [rsp],0xCCCCCCCC
	mov dword ptr [rsp + 0x4],0xCCCCCCCC
	*/
	memcpy(Address, PushRet, sizeof(PushRet));
	*(DWORD*)(Address + 7) = LODWORD(Destination);
	*(DWORD*)(Address + 15) = HIDWORD(Destination);
	return sizeof(PushRet);
}

BYTE SHADOWSUB_ASM[] = { 0x48, 0x83, 0xEC, 0x28 };
volatile int WriteSubShadowSpace(BYTE* Address)
{
	//sub rsp,0x28
	memcpy(Address, SHADOWSUB_ASM, sizeof(SHADOWSUB_ASM));
	return sizeof(SHADOWSUB_ASM);
}

BYTE SHADOWADD_ASM[] = { 0x48, 0x83, 0xC4, 0x28 };
volatile int WriteAddShadowSpace(BYTE* Address)
{
	//add rsp,0x28
	memcpy(Address, SHADOWADD_ASM, sizeof(SHADOWADD_ASM));
	return sizeof(SHADOWADD_ASM);
}

volatile int WriteRET(BYTE* Address)
{
	BYTE ret[] = { 0xC3 };
	memcpy(Address, ret, sizeof(ret));
	return sizeof(ret);
}

void HookFunctionAtRuntime(BYTE* SubRoutineAddress, HookMethod Method)
{
	BYTE* Callback = new BYTE[200];
	DWORD Old;
	VirtualProtect(Callback, 200, PAGE_EXECUTE_READWRITE, &Old);

	std::shared_ptr<PLH::IHook> Hook;
	DWORD64 Original;
	if (Method == HookMethod::INLINE)
	{
		Hook.reset(new PLH::Detour, [&](PLH::Detour* Hook) {
			Hook->UnHook();
			delete Hook;
			delete[] Callback;
		});
		((PLH::Detour*)Hook.get())->SetupHook((BYTE*)SubRoutineAddress, (BYTE*)Callback);
		Hook->Hook();
		Original = ((PLH::Detour*)Hook.get())->GetOriginal<DWORD64>();
	}
	else if (Method == HookMethod::INT3_BP) {
		Hook.reset(new PLH::VEHHook, [&](PLH::VEHHook* Hook) {
			//Hook->UnHook();
			//delete Hook;
			//delete[] Callback;
		});
		((PLH::VEHHook*)Hook.get())->SetupHook((BYTE*)SubRoutineAddress, (BYTE*)Callback, PLH::VEHHook::VEHMethod::INT3_BP);
		Hook->Hook();
		Original = ((PLH::VEHHook*)Hook.get())->GetOriginal<DWORD64>();
	}

	int WriteOffset = 0;
	WriteOffset += WritePUSHA_WPARAM(Callback,(DWORD64)SubRoutineAddress);
	WriteOffset += WriteAbsoluteCall(Callback + WriteOffset, (DWORD64)&PrologInterupt);
	WriteOffset += WritePOPA(Callback + WriteOffset);

	WriteOffset += WriteSubShadowSpace(Callback+WriteOffset);
	WriteOffset += WriteRetAddress(Callback + WriteOffset,(DWORD64)(Callback+WriteOffset+sizeof(PushRet) +sizeof(ABS_JMP_ASM)));
	WriteOffset += WriteAbsoluteJMP(Callback + WriteOffset, Original);
	WriteOffset += WriteAddShadowSpace(Callback + WriteOffset);

	WriteOffset += WritePUSHA_WPARAM(Callback + WriteOffset,(DWORD64)Hook.get());
	WriteOffset += WriteAbsoluteCall(Callback + WriteOffset, (DWORD64)&PostlogInterupt);
	WriteOffset += WritePOPA(Callback + WriteOffset);

	WriteOffset += WriteRET(Callback + WriteOffset);

	cPrint("[+] Callback at: %I64X\n", Callback);
	m_Hooks.push_back(Hook);
}
