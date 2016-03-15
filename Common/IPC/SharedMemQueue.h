#pragma once
#include "SharedMemMutex.h"
#include <mutex>
struct MemMessage
{
	MemMessage(BYTE* Data)
	{
		memcpy(m_Data, Data, sizeof(m_Data));
	}
	MemMessage()
	{

	}
	BYTE m_Data[64];
};

struct SharedMemQHeader
{
	DWORD m_OffsetToEndOfLastMessage;
	DWORD m_MessageCount;
};

class SharedMemQueue
{
public:
	enum class Mode
	{
		Server,
		Client
	};
	SharedMemQueue(const std::string& ServerName, const DWORD BufSize, Mode Type);
	~SharedMemQueue();
	bool PushMessage(MemMessage Msg);
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
	bool m_InitOk;
};

SharedMemQueue::SharedMemQueue(const std::string& ServerName, const DWORD BufSize, Mode Type) :
	m_Mutex(std::string(ServerName + "_MTX"), (Type == Mode::Server) ? SharedMemMutex::Mode::Server : SharedMemMutex::Mode::Client)
{
	m_InitOk = true;
	m_BufSize = BufSize;

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

	printf("%X\n", m_Buffer);
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
}

SharedMemQueue::~SharedMemQueue()
{
	UnmapViewOfFile(m_Buffer);
	CloseHandle(m_hMappedFile);
}

bool SharedMemQueue::PushMessage(MemMessage Msg)
{
	if (!m_InitOk)
		return false;

	std::lock_guard<SharedMemMutex> Lock(m_Mutex);

	BYTE* WriteLocation = ((BYTE*)m_OutHeader) + sizeof(SharedMemQHeader) + m_OutHeader->m_OffsetToEndOfLastMessage;

	DWORD_PTR Delta = (WriteLocation + sizeof(MemMessage)) - m_Buffer;
	if (Delta >= m_BufSize)
		return false;

	memcpy(WriteLocation, Msg.m_Data, sizeof(MemMessage));
	m_OutHeader->m_OffsetToEndOfLastMessage += sizeof(MemMessage);
	m_OutHeader->m_MessageCount++;
	return true;
}

bool SharedMemQueue::PopMessage(MemMessage& Msg)
{
	if (!m_InitOk)
		return false;

	std::lock_guard<SharedMemMutex> Lock(m_Mutex);
	if (m_InHeader->m_MessageCount < 1)
		return false;

	BYTE* ReadLocation = ((BYTE*)m_InHeader) + sizeof(SharedMemQHeader) + m_InHeader->m_OffsetToEndOfLastMessage - sizeof(MemMessage);

	memcpy(Msg.m_Data, ReadLocation, sizeof(MemMessage));
	m_InHeader->m_OffsetToEndOfLastMessage -= sizeof(MemMessage);
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
	while (!IsMessageAvailable())
	{
		Sleep(20);
	}
}