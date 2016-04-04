#pragma once
#include <string>
class SharedSignal
{
public:
	SharedSignal(const std::string& Name);
	~SharedSignal();
	bool Signal();
	bool Wait();
	bool Reset();
private:
	HANDLE m_hEvent;
	bool m_InitOk;
};

SharedSignal::SharedSignal(const std::string& Name)
{
	m_InitOk = true;

	m_hEvent = CreateEventA(NULL, TRUE, FALSE, Name.c_str());
	if (m_hEvent == NULL)
		m_InitOk = false;
}

SharedSignal::~SharedSignal()
{
	CloseHandle(m_hEvent);
}

bool SharedSignal::Signal()
{
	if (!m_InitOk)
		return false;

	return SetEvent(m_hEvent) == NULL ? false:true;
}

bool SharedSignal::Wait()
{
	if (!m_InitOk)
		return false;

	if (WaitForSingleObject(m_hEvent, INFINITE) == WAIT_OBJECT_0)
		return true;
	return false;
}

bool SharedSignal::Reset()
{
	return ResetEvent(m_hEvent) == NULL ? false : true;
}