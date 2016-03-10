#pragma once
struct MemMessage
{
	MemMessage(BYTE* Data)
	{
		memcpy(m_Data, Data, sizeof(m_Data));
	}
	MemMessage()
	{

	}
	BYTE m_Data[32];
};

struct SharedMemQHeader
{
	DWORD m_OffsetToEndOfLastMessage;
	DWORD m_MessageCount;
};

class SharedMemeQueue
{
public:
	enum class Mode
	{
		Server,
		Client
	};
	SharedMemeQueue(const std::string& ServerName,const DWORD BufSize,Mode Type);
	~SharedMemeQueue();
	bool PushMessage(MemMessage Msg);
	bool PopMessage(MemMessage& Msg);
	DWORD GetMessageCount() const;
private:
	BYTE* m_Buffer;
	HANDLE m_hMappedFile;
	bool m_InitOk;
};

SharedMemeQueue::SharedMemeQueue(const std::string& ServerName,const DWORD BufSize,Mode Type)
{
	m_InitOk = true;

	if (Type == Mode::Server)
	{
		m_hMappedFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
			PAGE_READWRITE, 0, BufSize, ServerName.c_str());
	}else if (Type == Mode::Client) {
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

	SharedMemQHeader* Queue = (SharedMemQHeader*)m_Buffer;
	Queue->m_MessageCount = 0;
	Queue->m_OffsetToEndOfLastMessage = 0;
}

SharedMemeQueue::~SharedMemeQueue()
{
	UnmapViewOfFile(m_Buffer);
	CloseHandle(m_hMappedFile);
}

bool SharedMemeQueue::PushMessage(MemMessage Msg)
{
	if (!m_InitOk)
		return false;

	SharedMemQHeader* Queue = (SharedMemQHeader*)m_Buffer;
	memcpy(m_Buffer + sizeof(SharedMemQHeader) + Queue->m_OffsetToEndOfLastMessage, Msg.m_Data, sizeof(MemMessage));
	Queue->m_OffsetToEndOfLastMessage += sizeof(MemMessage);
	Queue->m_MessageCount++;
	return true;
}

bool SharedMemeQueue::PopMessage(MemMessage& Msg)
{
	if (!m_InitOk)
		return false;

	SharedMemQHeader* Queue = (SharedMemQHeader*)m_Buffer;
	printf("%d %d\n", Queue->m_MessageCount, Queue->m_OffsetToEndOfLastMessage);
	memcpy(Msg.m_Data, m_Buffer + sizeof(SharedMemQHeader) + Queue->m_OffsetToEndOfLastMessage - sizeof(MemMessage), sizeof(MemMessage));
	Queue->m_OffsetToEndOfLastMessage -= sizeof(MemMessage);
	Queue->m_MessageCount--;

	return true;
}

DWORD SharedMemeQueue::GetMessageCount() const
{
	if (!m_InitOk)
		return 0;

	SharedMemQHeader* Queue = (SharedMemQHeader*)m_Buffer;
	return Queue->m_MessageCount;
}