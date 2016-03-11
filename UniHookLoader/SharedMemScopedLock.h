#pragma once
#include "SharedMemMutex.h"
class SharedMemScopedLock
{
public:
	SharedMemScopedLock(SharedMemMutex Mutex);
	~SharedMemScopedLock();
private:
	bool m_LockedStatus;
	SharedMemMutex m_Mutex;
};

SharedMemScopedLock::SharedMemScopedLock(SharedMemMutex Mutex):
	m_Mutex(Mutex)
{
	m_LockedStatus = m_Mutex.Lock();
}

SharedMemScopedLock::~SharedMemScopedLock()
{
	if (!m_LockedStatus)
		return;
	m_Mutex.Release();
}



