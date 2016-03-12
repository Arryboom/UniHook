#pragma once
#include <string>
#include <vector>
#include <Windows.h>
struct Process
{
	std::wstring m_Name;
	DWORD m_PID;
};

class Injector
{
public:
	Injector();
	~Injector();
	bool OpenTarget(DWORD PID);
	bool OpenTarget(const std::wstring& ProcessName);
	bool Inject(const std::wstring& DllPath);
private:
	std::vector<Process> GetProcessList();
	HANDLE m_Target;
};

template<typename Func>
class FinalAction {
public:
	FinalAction(Func f) :FinalActionFunc(std::move(f)) {}
	~FinalAction()
	{
		FinalActionFunc();
	}
private:
	Func FinalActionFunc;

	/*Uses RAII to call a final function on destruction
	C++ 11 version of java's finally (kindof)*/
};

template <typename F>
FinalAction<F> finally(F f) {
	return FinalAction<F>(f);
}
