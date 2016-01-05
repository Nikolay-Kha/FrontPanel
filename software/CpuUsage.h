#pragma once
#include <windows.h>

class CpuUsage
{
public:
	CpuUsage(void);
	
	int  getUsage();
	static int getMemoryUsage();
private:
	ULARGE_INTEGER	ul_sys_idle_old;
	ULARGE_INTEGER  ul_sys_kernel_old;
	ULARGE_INTEGER  ul_sys_user_old;
};
