#ifndef REGIONS_H
#define REGIONS_H

#include <windows.h>
#include <QString>
#include <QVector>

class MemoryRegion
{
public:
	HANDLE      m_ModuleHandle;
	QString		m_ModuleName;
	QString		m_ModulePath;
	QString		m_SectionName;
	uintptr_t	m_BaseAddress;
	SIZE_T		m_RegionSize;
	DWORD		m_State;
	DWORD		m_Type;
	DWORD		m_Protect;
};

#endif

