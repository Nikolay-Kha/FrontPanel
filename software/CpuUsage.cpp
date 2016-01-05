#include <windows.h>
#include "CpuUsage.h"

CpuUsage::CpuUsage(void)
{

}

int CpuUsage::getUsage()
{
	ULARGE_INTEGER         ul_sys_idle;
	ULARGE_INTEGER         ul_sys_kernel;
	ULARGE_INTEGER         ul_sys_user;

	int  usage = 0;

	// we cannot directly use GetSystemTimes on C language
	/* add this line :: pfnGetSystemTimes */
	GetSystemTimes((LPFILETIME)&ul_sys_idle,    /* System idle time */
		(LPFILETIME)&ul_sys_kernel,  /* system kernel time */
		(LPFILETIME)&ul_sys_user);   /* System user time */

	usage  =(int)
		(
		(
		(
		(
		(ul_sys_kernel.QuadPart - ul_sys_kernel_old.QuadPart)+
		(ul_sys_user.QuadPart   - ul_sys_user_old.QuadPart)
		)
		-
		(ul_sys_idle.QuadPart-ul_sys_idle_old.QuadPart)
		)
		*
		(100)
		)
		/
		(
		(ul_sys_kernel.QuadPart - ul_sys_kernel_old.QuadPart)+
		(ul_sys_user.QuadPart   - ul_sys_user_old.QuadPart)
		)
		);

	ul_sys_idle_old.QuadPart   = ul_sys_idle.QuadPart;
	ul_sys_user_old.QuadPart   = ul_sys_user.QuadPart;
	ul_sys_kernel_old.QuadPart = ul_sys_kernel.QuadPart;

	return usage;
}

int CpuUsage::getMemoryUsage()
{
	MEMORYSTATUSEX ms;
	ms.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&ms);
	return  (int)((ms.ullTotalPageFile-ms.ullAvailPageFile)/1024/1024);
}
