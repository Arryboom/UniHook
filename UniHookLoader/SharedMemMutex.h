#pragma once
#include <string>
class SharedMemMutex
{
public:
	enum class Mode
	{
		Server ,
		Client
	};
	SharedMemMutex(const std::string& Name, Mode Type);
	~SharedMemMutex();
	bool Lock();
	bool Release();
private:
	HANDLE m_hMutex;
	bool m_InitOk;
};

SharedMemMutex::SharedMemMutex(const std::string& Name, Mode Type)
{
	m_InitOk = true;

	//Create mutex if it doesn't exist, join if it does
	m_hMutex = CreateMutex(NULL, (Type == Mode::Server) ? FALSE : TRUE, Name.c_str());
	if (m_hMutex == NULL)
	{
		m_InitOk = false;
		return;
	}
}

SharedMemMutex::~SharedMemMutex()
{
	CloseHandle(m_hMutex);
}

bool SharedMemMutex::Lock()
{
	if (!m_InitOk)
		return false;

	//Blocks until it has ownership
	DWORD Result = WaitForSingleObject(m_hMutex, INFINITE);
	if (Result == WAIT_OBJECT_0)
		return true;
	return false;
}

bool SharedMemMutex::Release()
{
	if (!m_InitOk)
		return false;

	if (ReleaseMutex(m_hMutex) != NULL)
		return true;
	return false;
}