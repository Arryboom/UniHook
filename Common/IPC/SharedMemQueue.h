#pragma once
#include "SharedMemMutex.h"
#include "SharedSignal.h"
#include <mutex>

DWORD WINAPI MsgThread(LPVOID lparam);
struct MemMessage
{
	MemMessage(BYTE* Data,DWORD Size)
	{
		m_DataSize = Size;
		m_Data.resize(Size);
		memcpy(&m_Data[0], Data, m_DataSize);
	}
	MemMessage(const char* Frmt, ...)
	{
		va_list args;
		va_start(args, Frmt);
		char szBuffer[512];
		int nBuf = vsnprintf_s(szBuffer, 511,Frmt, args);
		va_end(args);

		m_DataSize = nBuf + 1;
		m_Data.resize(m_DataSize);
		memcpy(&m_Data[0], szBuffer, m_DataSize);
	}
	MemMessage(const std::string& Msg)
	{
		m_DataSize = Msg.size();
		m_Data.resize(m_DataSize);
		memcpy(&m_Data[0], Msg.c_str(), m_DataSize);
	}
	MemMessage()
	{
		
	}
	//Vector data is written into shared Mem
	std::vector<BYTE> m_Data;
	DWORD m_DataSize;
};

struct SharedMemQHeader
{
	DWORD m_OffsetToEndOfLastMessage;
	DWORD m_MessageCount;
};

class SharedMemQueue
{
public:
	typedef void(*tMsgReceivedCallback)();
	enum class Mode
	{
		Server,
		Client
	};
	SharedMemQueue(const std::string& ServerName, const DWORD BufSize, Mode Type);
	~SharedMemQueue();

	void SetCallback(tMsgReceivedCallback Callback);

	//For batch operations
	void ManualLock();
	void ManualUnlock();

	//Call with true to use manual lock features
	bool PushMessage(MemMessage Msg,bool ManualLocking = false);
	bool PopMessage(MemMessage& Msg);
	bool IsMessageAvailable();
	void WaitForMessage();
	DWORD GetOutMessageCount() const;
	DWORD GetInMessageCount() const;
private:
	mutable SharedMemMutex m_Mutex;
	SharedMemQHeader* m_OutHeader;
	SharedMemQHeader* m_InHeader;
	DWORD m_BufSize;
	BYTE* m_Buffer;
	HANDLE m_hMappedFile;
	HANDLE m_hMsgThread;
	SharedSignal m_ServerToClientSignal;
	SharedSignal m_ClientToServerSignal;
	Mode m_Type;
	bool m_InitOk;
	tMsgReceivedCallback m_ReceivedCallback;
};

SharedMemQueue::SharedMemQueue(const std::string& ServerName, const DWORD BufSize, Mode Type) :
	m_Mutex(std::string(ServerName + "_MTX"), (Type == Mode::Server) ? SharedMemMutex::Mode::Server : SharedMemMutex::Mode::Client),
	m_ServerToClientSignal(std::string(ServerName+"_SC_SGNL")),m_ClientToServerSignal(std::string(ServerName+"_CS_SGNL"))
{
	m_InitOk = true;
	m_BufSize = BufSize;
	m_Type = Type;

	if (Type == Mode::Server)
	{
		m_hMappedFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
			PAGE_READWRITE, 0, BufSize, ServerName.c_str());
	}
	else if (Type == Mode::Client) {
		m_hMappedFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, ServerName.c_str());
	}

	if (m_hMappedFile == NULL)
	{
		m_InitOk = false;
		return;
	}

	m_Buffer = (BYTE*)MapViewOfFile(m_hMappedFile, FILE_MAP_ALL_ACCESS, 0, 0, BufSize);
	if (m_Buffer == NULL)
	{
		CloseHandle(m_hMappedFile);
		m_InitOk = false;
		return;
	}

	if (Type == Mode::Server)
	{
		m_OutHeader = (SharedMemQHeader*)m_Buffer;
		m_OutHeader->m_MessageCount = 0;
		m_OutHeader->m_OffsetToEndOfLastMessage = 0;

		m_InHeader = (SharedMemQHeader*)(m_Buffer + (BufSize/2));
		m_InHeader->m_MessageCount = 0;
		m_InHeader->m_OffsetToEndOfLastMessage = 0;
	}

	if (Type == Mode::Client)
	{
		m_InHeader = (SharedMemQHeader*)m_Buffer;
		m_OutHeader = (SharedMemQHeader*)(m_Buffer + (BufSize/2));
	}
	m_hMsgThread = CreateThread(NULL, NULL, MsgThread, this, NULL, NULL);
}

SharedMemQueue::~SharedMemQueue()
{
	UnmapViewOfFile(m_Buffer);
	CloseHandle(m_hMappedFile);
	CloseHandle(m_hMsgThread);
}

void SharedMemQueue::SetCallback(tMsgReceivedCallback Callback)
{
	m_ReceivedCallback = Callback;
}

void SharedMemQueue::ManualLock()
{
	m_Mutex.lock();
}

void SharedMemQueue::ManualUnlock()
{
	m_Mutex.unlock();
}

bool SharedMemQueue::PushMessage(MemMessage Msg,bool ManualLocking)
{
	if (!m_InitOk)
		return false;

	if(!ManualLocking)
		std::lock_guard<SharedMemMutex> Lock(m_Mutex);

	BYTE* WriteLocation = ((BYTE*)m_OutHeader) + sizeof(SharedMemQHeader) + m_OutHeader->m_OffsetToEndOfLastMessage;

    //sizeof(DWORD) = sizeof(MemMessage::m_DataSize), Qt doesn't like that syntax
	//Make sure we don't overrun our buffer
    intptr_t Delta = (WriteLocation + Msg.m_DataSize + sizeof(DWORD)) - ((BYTE*)m_OutHeader);
	if (Delta >= (m_BufSize/2))
		return false;

	//Write Data
	memcpy(WriteLocation, &Msg.m_Data[0], Msg.m_DataSize);
	WriteLocation +=  Msg.m_DataSize;

	//Write the message size
	*(DWORD*)WriteLocation = Msg.m_DataSize;
	
    m_OutHeader->m_OffsetToEndOfLastMessage += Msg.m_DataSize + sizeof(DWORD);
	m_OutHeader->m_MessageCount++;

	if (m_Type == Mode::Server)
		m_ServerToClientSignal.Signal();
	else
		m_ClientToServerSignal.Signal();
	return true;
}

bool SharedMemQueue::PopMessage(MemMessage& Msg)
{
	if (!m_InitOk)
		return false;

	std::lock_guard<SharedMemMutex> Lock(m_Mutex);
	if (m_InHeader->m_MessageCount < 1)
		return false;

    BYTE* ReadLocation = ((BYTE*)m_InHeader) + sizeof(SharedMemQHeader) + m_InHeader->m_OffsetToEndOfLastMessage - sizeof(DWORD);
	
	//Get size of message data
	DWORD MsgSize = *(DWORD*)ReadLocation;
	ReadLocation -= MsgSize;

	//Make room in the vector for our data
	Msg.m_Data.resize(MsgSize);

	//Read the data
	memcpy(&Msg.m_Data[0], ReadLocation, MsgSize);

	//Set the size of the message in the struct
	Msg.m_DataSize = MsgSize;

    m_InHeader->m_OffsetToEndOfLastMessage -= MsgSize + sizeof(DWORD);
	m_InHeader->m_MessageCount--;
	return true;
}

DWORD SharedMemQueue::GetOutMessageCount() const
{
	if (!m_InitOk)
		return 0;

	std::lock_guard<SharedMemMutex> Lock(m_Mutex);
	return m_OutHeader->m_MessageCount;
}

DWORD SharedMemQueue::GetInMessageCount() const
{
	if (!m_InitOk)
		return 0;

	std::lock_guard<SharedMemMutex> Lock(m_Mutex);
	return m_InHeader->m_MessageCount;
}

bool SharedMemQueue::IsMessageAvailable()
{
	if (!m_InitOk)
		return false;

	std::lock_guard<SharedMemMutex> Lock(m_Mutex);
	if (m_InHeader->m_MessageCount > 0)
		return true;

	return false;
}

void SharedMemQueue::WaitForMessage()
{
	if (m_Type == Mode::Server)
	{
		m_ClientToServerSignal.Wait();
		m_ClientToServerSignal.Reset();
	}else {
		m_ServerToClientSignal.Wait();
		m_ServerToClientSignal.Reset();
	}
	if(m_ReceivedCallback)
		m_ReceivedCallback();
}

DWORD WINAPI MsgThread(LPVOID lparam)
{
	do 
	{
		SharedMemQueue* pThis = (SharedMemQueue*)lparam;
		if(pThis)
			pThis->WaitForMessage();
	} while (1);
}
