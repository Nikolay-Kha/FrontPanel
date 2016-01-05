#include <windows.h>
#include "CpuUsage.h"
#include "GpuUsage.h"
#include "FPControl.h"
#include "TlHelp32.h"
#include <atlbase.h>
#include <atlwin.h>
#include "VolumeWatcher.h"

int TIMEOUT = 2000;
#define REFRESHTIME 100
#define SYNCTIME 3600000
const WCHAR goodPath[]=L"C:\\Windows\\fp.exe";
const WCHAR REGISTRY_NAME[] = L"FrontPanel";
#define RETURN_OK 0
#define RETURN_ERROR -1
#define RETURN_WARNING 1
#define RETURN_ARGVERROR -2
#define DEFAULT_TIME 5
#define OKSTR "OK\n"

struct IPC{
	unsigned int time;
	UPDATE_TYPE type;
};

const LPVOID adress = (void *)0x20000000;

const char encodeToRuss[] = { // not inlucded ¸(0xB8->0xB5) and ¨(0xA8->0xA2)
	'A', // 0xC0
	(char)0xA0,
	'B',
	(char)0xA1,
	(char)0xE0,
	'E',
	(char)0xA3,
	(char)0xA4,
	(char)0xA5,
	(char)0xA6,
	'K',
	(char)0xA7,
	'M',
	'H',
	'O',
	(char)0xA8,
	'P',
	'C',
	'T',
	(char)0xA9,
	(char)0xAA,
	'X',
	(char)0xE1,
	(char)0xAB,
	(char)0xAC,
	(char)0xE2,
	(char)0xAD,
	(char)0xAE,
	(char)0xC4,
	(char)0xAF,
	(char)0xB0,
	(char)0xB1,
	'a', // 0xE0
	(char)0xB2,
	(char)0xB3,
	(char)0xB4,
	(char)0xE3,
	'e',
	(char)0xB6,
	(char)0xB7,
	(char)0xB8,
	(char)0xB9,
	(char)0xBA,
	(char)0xBB,
	(char)0xBC,
	(char)0xBD,
	'o',
	(char)0xBE,
	'p',
	'c',
	(char)0xBF,
	'y',
	(char)0xE4,
	'x',
	(char)0xE5,
	(char)0xC0,
	(char)0xC1,
	(char)0xE6,
	(char)0xC2,
	(char)0xC3,
	(char)0xC4,
	(char)0xC5,
	(char)0xC6,
	(char)0xC7 //0xFF
};


int GetThisProcess(const WCHAR *thisexe)
{
  HANDLE hProcessSnap;
  HANDLE hModuleSnap;
  PROCESSENTRY32 pe32;
  MODULEENTRY32  me32;
  WCHAR *file =(WCHAR *)( (unsigned long)thisexe+lstrlen(thisexe)*2);
  while((unsigned long)file>(unsigned long)thisexe && file[-1]!='\\')
	  file--;

  hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
  if( hProcessSnap == INVALID_HANDLE_VALUE )
  {
    return 0;
  }

  pe32.dwSize = sizeof( PROCESSENTRY32 );

  if( !Process32First( hProcessSnap, &pe32 ) )
  {
    CloseHandle( hProcessSnap );
    return 0;
  }

  // Now walk the snapshot of processes, and
  // display information about each process in turn
  do
  {
    if(pe32.th32ProcessID!=GetCurrentProcessId() && lstrcmpi(pe32.szExeFile, file)==0)
	{
		me32.dwSize = sizeof( MODULEENTRY32 );
		hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, pe32.th32ProcessID);
		if(hModuleSnap!=INVALID_HANDLE_VALUE)
		{
			if( Module32First( hModuleSnap, &me32 ) )
			{
				do
				{
					if(lstrcmpi(me32.szExePath, thisexe)==0)
					{
						CloseHandle(hProcessSnap);
						CloseHandle(hModuleSnap);
						return pe32.th32ProcessID;
					}
				}while( Module32Next( hModuleSnap, &me32 ) );
			}
			CloseHandle(hModuleSnap);
		}
	}
  } while( Process32Next( hProcessSnap, &pe32 ) );

  CloseHandle( hProcessSnap );
  return 0;
}

bool installAutoRun()
{
	HKEY hklm;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_ALL_ACCESS, &hklm)==ERROR_SUCCESS)
	{
		bool res = RegSetValueEx(hklm,REGISTRY_NAME,0,REG_SZ,(BYTE*)goodPath,lstrlen(goodPath)*2)==ERROR_SUCCESS;
		RegCloseKey(hklm);
		return res;
	}
	return false;
}

bool unInstallAutoRun()
{
	HKEY hklm;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_ALL_ACCESS, &hklm)==ERROR_SUCCESS)
	{
		bool res = RegDeleteValue(hklm,REGISTRY_NAME)==ERROR_SUCCESS;
		RegCloseKey(hklm);
		return res;
	}
	return false;
}

BOOL IsUserAdmin(VOID)
{
BOOL b;
SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
PSID AdministratorsGroup; 
b = AllocateAndInitializeSid(
    &NtAuthority,
    2,
    SECURITY_BUILTIN_DOMAIN_RID,
    DOMAIN_ALIAS_RID_ADMINS,
    0, 0, 0, 0, 0, 0,
    &AdministratorsGroup); 
if(b) 
{
    if (!CheckTokenMembership( NULL, AdministratorsGroup, &b)) 
    {
         b = FALSE;
    } 
    FreeSid(AdministratorsGroup); 
}

return(b);
}

int runTaskAsAdmin(WCHAR *thisexe, WCHAR *param)
{
	SHELLEXECUTEINFO shei;
	memset(&shei,0,sizeof(SHELLEXECUTEINFO));
	shei.cbSize=sizeof(SHELLEXECUTEINFO);
	shei.lpFile = thisexe;
	shei.lpParameters = param;
	shei.lpVerb = L"runas";
	shei.nShow = SW_HIDE;
	shei.fMask = SEE_MASK_NOCLOSEPROCESS;  //ensures hProcess gets the process handle

	if(ShellExecuteEx(&shei))
	{
		printf("Getting admin...OK\n");
		if (WaitForSingleObject(shei.hProcess,20000)==WAIT_TIMEOUT)
			return RETURN_ERROR;
		DWORD code;
		if (!GetExitCodeProcess(shei.hProcess,&code)){
			return RETURN_ERROR;
		}
		return code;
	} else
	{
		printf("Getting admin...FAULT.\n");
	}
	return RETURN_ERROR;
}

void usage()
{
	printf("Command line usage - fp.exe [/argumentsBelow] Text\n");
	printf("/noencode - not encoding character\n");
	printf("/t X - showing message time. Only in 0...59 seconds. 0 - permanently\n");
	printf("/a - all screen\n");
	printf("/c - after clock\n");
	printf("/s - second line - default\n");
	printf("/ap /sp /cp - the same as above, but not update another parts of screen, not same as \"/t 0\"\n");
	printf("this 6 arguments not combined with itselfs.\n");
	printf("\n");
	printf("Not combined:\n");
	printf("/install - install  autorun\n");
	printf("/uninstall - uninstall  autorun\n");
	printf("/force - force start in current dir, not installing or touch service\n");
	printf("/timeout x - set refresh timeout in ms. Allow in 100...20000\n");
	printf("/help show this\n");
	printf("\n");
	printf("Return code:\n");
	printf("0 - OK\n");
	printf("1 -Warning\n");
	printf("-1 -Error\n");
	printf("-2 -Arguments error\n");
}

void printCommandLineParsingError()
{
	printf("Invalid command line arguments\n");
	ExitProcess(RETURN_ARGVERROR);
}

bool doInstall(bool isUninstall, int argc, char *argv[], WCHAR *thisexe)
{
	if(IsUserAdmin())
	{
		bool res;
		if(isUninstall)
			res = unInstallAutoRun();
		else
			res = installAutoRun();
		if(res){
			printf(OKSTR);
			return true;
		} else {
			printf("Error while writing registry.\n");
			return false;
		}
	} else
	{
		WCHAR w[1024];
		w[0]=0;
		for(int i=1; i<argc; i++)
		{
			wsprintf(w,i==1?L"%s%S":L"%s %S",w,argv[i]);
		}
		if(runTaskAsAdmin(thisexe,w)==RETURN_OK)
		{
			printf(OKSTR);
			return true;
		} else
		{
			printf("Error while writing registry.\n");
			return false;
		}
	}
}

DWORD WINAPI mainLoop(LPVOID)
{
		GpuUsage mGpu;
		CpuUsage mCpu;
		DWORD lastTimeSync=0;
		DWORD lastTimeUpdate=0;
		IPC *stopper = (IPC*)VirtualAlloc(adress,sizeof(IPC)+4,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
		stopper->time = 0;
		stopper->type = UPDATE_BOTH;
		bool lastOk=true;
		syncTime();
		while(true)
		{
			DWORD time = GetTickCount();
			if(stopper->time==0){
				if(time>lastTimeSync+SYNCTIME)
				{
					syncTime();
					lastTimeSync = time;
				}
			}
			if(stopper->time == 0)
			{
				stopper->type = UPDATE_BOTH;
			}
			if(time>lastTimeUpdate+TIMEOUT)
			{
				bool res = updateInfo(&mCpu,&mGpu, stopper->type);
				if(res && res!=lastOk)
					syncTime();
				lastOk=res;
				lastTimeUpdate = time;
			}
			if(stopper->time!=0)
				if(time>stopper->time)
				{
					stopper->time = 0;
					lastTimeUpdate = 0;
					continue;
				}
			Sleep(REFRESHTIME);
		}
}

int mVolume;
HANDLE hThread = INVALID_HANDLE_VALUE;
HANDLE gMutex; 
DWORD WINAPI VolumeThreadProc(LPVOID )
{
	Sleep(100); // wait another event
	hThread = INVALID_HANDLE_VALUE;
	int vol = mVolume;
	if(WaitForSingleObject(gMutex,5000)==TIMEOUT)
		return 0;
	char text[17];
	bool st = false;
	text[0]='V';
	text[1]='O';
	text[2]='L';
	text[3]=' ';
	const int min =4;
	const int max = 12;
	if(vol>=0)
	{
		for(int i = min; i < max; i++)
		{
			text[i]= (i-min)*100/(max-min)<vol?(char)0xFF:'_';
		}
	} else
	{
		text[4]='*';
		text[5]='*';
		text[6]='M';
		text[7]='U';
		text[8]='T';
		text[9]='E';
		text[10]='*';
		text[11]='*';
		text[12]=' ';
	}
	int v = vol;
	if(v<0)
		v=-v;
	if(v >= 100)
	{
		text[12] = v/100+0x30;
		st = true;
	}
	else
		text[12] = ' ';
	v = v%100;
	if(v >= 10 || st)
		text[13] = v/10+0x30;
	else
		text[13] = ' ';
	v = v%10;
	text[14] = v+0x30;
	text[15] = '%';
	text[16] = 0;
	((IPC*)adress)->type = UPDATE_1STLINE;
	((IPC*)adress)->time = GetTickCount()+2000;
	setText(text,DEFAULT_TIMEOUT,0,1);
	ReleaseMutex(gMutex);
	return 0;
}

void setVol(int vol)
{
	mVolume = vol;
	if(hThread == INVALID_HANDLE_VALUE)
		hThread = CreateThread(0,0,VolumeThreadProc,0,0,0);
}

int main(int argc, char *argv[])
{
	bool isEncode = true;
	bool isFoundSetText = false;
	bool isFoundSetTextLine = false;
	bool isTimeOutSet = false;
	bool isForce = false;
	bool isPermanenty = false;
	IPC setTextIPC = {DEFAULT_TIME,UPDATE_1STLINE};
	char foundedSetText[33];
	foundedSetText[0] = 0;
	int foundSetTextLinePos = 0;
	WCHAR thisexe[MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL),thisexe,MAX_PATH);

	if(argc==2 && _strcmpi(argv[1],"/installandcopy")==0) // service argument while installing
	{
		if(IsUserAdmin())
		{
			int pId = GetThisProcess(goodPath);
			if(pId)
			{
				HANDLE hP = OpenProcess(PROCESS_ALL_ACCESS,false,pId);
				if(hP!=INVALID_HANDLE_VALUE)
				{
					TerminateProcess(hP,0);
					CloseHandle(hP);
				}
			}
			Sleep(300);
			DeleteFile(goodPath);
			if(CopyFile(thisexe,goodPath,false))
				printf("Copy...OK\n");
			else
			{
				printf("Error while copy file.\n");
				return RETURN_ERROR;
			}
			printf("Installing autorun...");
			if(installAutoRun()){
				printf(OKSTR);
			} else {
				printf("Error writing registry.\n");
				return RETURN_ERROR;
			}
			return RETURN_OK;
		} else
		    printf("Error. No admin.\n");
		return RETURN_ERROR;
	}

	if(argc>1) // parsing arguments
	{
		for(int i=1; i<argc; i++)
		{
			if(_strcmpi(argv[i], "/help")==0)
			{
				usage();
				return RETURN_OK;
			}
			else if(_strcmpi(argv[i], "/install")==0)
			{
				if(argc!=2 || isFoundSetText)
					printCommandLineParsingError();
				else
					return doInstall(false,argc,argv,thisexe)?RETURN_OK:RETURN_ERROR;
			}
			else if(_strcmpi(argv[i], "/uninstall")==0)
			{
				if(argc!=2 || isFoundSetText)
					printCommandLineParsingError();
				else
					return doInstall(true,argc,argv,thisexe)?RETURN_OK:RETURN_ERROR;
			}
			else if(_strcmpi(argv[i], "/force")==0)
			{
					isForce = true;
			}
			else if(_strcmpi(argv[i], "/noencode")==0)
			{
				if(isFoundSetText)
					printCommandLineParsingError();
				isEncode = false;
			}
			else if(_strcmpi(argv[i], "/timeout")==0)
			{
				if(i+2!=argc || isFoundSetText)
					printCommandLineParsingError();
				else
				{
					i++;
					int t = atoi(argv[i]);
					if(t==0)
						printCommandLineParsingError();
					if(t<100 || t>20000)
					{
						printf("Timerout must be in 100...20000 ms");
						return RETURN_ARGVERROR;
					}
					isTimeOutSet = true;
					TIMEOUT = t;
				}
			}
			else if(_strcmpi(argv[i], "/t")==0)
			{
				if(i+1>=argc  || isFoundSetText)
					printCommandLineParsingError();
				else
				{
					if(setTextIPC.time==0)
						printCommandLineParsingError();
					i++;
					int t = atoi(argv[i]);
					if(t==0 && argv[i][0]!=0x30)
						printCommandLineParsingError();
					if(t<0 || t>59)
					{
						printf("Time must be in 0...59 seconds. 0 - permanently");
						return RETURN_ARGVERROR;
					}
					setTextIPC.time = t;
				}
			}
			else if(_strcmpi(argv[i], "/a")==0)
			{
				if(isFoundSetTextLine || isFoundSetText)
					printCommandLineParsingError();
				setTextIPC.type = UPDATE_NONE;
				isFoundSetTextLine = true;
			}
			else if(_strcmpi(argv[i], "/c")==0)
			{
				if(isFoundSetTextLine || isFoundSetText)
					printCommandLineParsingError();
				setTextIPC.type = UPDATE_2NDLINE;
				isFoundSetTextLine = true;
			}
			else if(_strcmpi(argv[i], "/s")==0)
			{
				if(isFoundSetTextLine || isFoundSetText)
					printCommandLineParsingError();
				setTextIPC.type = UPDATE_1STLINE;
				isFoundSetTextLine = true;
			}
			else if(_strcmpi(argv[i], "/ap")==0)
			{
				if(isFoundSetTextLine || isFoundSetText)
					printCommandLineParsingError();
				setTextIPC.type = UPDATE_NONE;
				setTextIPC.time = 0;
				isPermanenty = true;
				isFoundSetTextLine = true;
			}
			else if(_strcmpi(argv[i], "/cp")==0)
			{
				if(isFoundSetTextLine || isFoundSetText)
					printCommandLineParsingError();
				setTextIPC.type = UPDATE_2NDLINE;
				setTextIPC.time = 0;
				isPermanenty = true;
				isFoundSetTextLine = true;
			}
			else if(_strcmpi(argv[i], "/sp")==0)
			{
				if(isFoundSetTextLine || isFoundSetText)
					printCommandLineParsingError();
				setTextIPC.type = UPDATE_1STLINE;
				setTextIPC.time = 0;
				isPermanenty = true;
				isFoundSetTextLine = true;
			} else
			{
				isFoundSetText = true;
				int j = 0;
				if(foundSetTextLinePos && foundSetTextLinePos<33)
				{
					foundedSetText[foundSetTextLinePos] = ' ';
					foundSetTextLinePos++;
				}

				while(foundSetTextLinePos<33 && argv[i][j]!=0)
				{
					foundedSetText[foundSetTextLinePos]=argv[i][j];
					foundSetTextLinePos++;
					j++;
				}
				foundedSetText[foundSetTextLinePos]=0;
			}
		}
	}



	if(!isForce && lstrcmpi(thisexe, goodPath)!=0) // check if we are have installed in system and install if neccary
	{
		if(runTaskAsAdmin(thisexe,L"/installandcopy")==RETURN_OK)
		{
			printf("Installing...OK\n");
		}else
		{
			printf("Installing...FAULT\n");
			return RETURN_OK;
		}
		printf("Running...");
		PROCESS_INFORMATION pi32;
		STARTUPINFO si32;
		memset(&si32,0,sizeof(STARTUPINFO));
		si32.cb = sizeof(STARTUPINFO);
		WCHAR w[1024];
		wsprintf(w,L"%s ",goodPath);
		for(int i=1; i<argc; i++)
		{
			wsprintf(w,L"%s %S",w,argv[i]);
		}
		if(CreateProcess((LPWSTR)goodPath, w,0,0,false,DETACHED_PROCESS,0,0,&si32,&pi32))
		{
			printf(OKSTR);
			CloseHandle(pi32.hProcess);
		} else
		{
			printf("Error while starting.\n");
			return RETURN_ERROR;
		}
		return RETURN_OK;
	}

	if(!isForce && GetConsoleWindow()) // hide window if we have own
	{
		CWindow myWindow;
		myWindow.Attach(GetConsoleWindow());
		if(myWindow.GetWindowProcessID()==GetCurrentProcessId())
			ShowWindow(GetConsoleWindow(),SW_HIDE);
	}
	
	int procId = GetThisProcess(thisexe);


	if(isTimeOutSet && procId) // wait until non admin proc has stoped
	{
		HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS,false,procId);
		if(hProc!=INVALID_HANDLE_VALUE)
		{
			WaitForSingleObject(hProc,100);
			CloseHandle(hProc);
			procId = GetThisProcess(thisexe);
		}
	}

	if(isTimeOutSet && procId)
	{
			if(IsUserAdmin())
			{
				HKEY hklm;
				if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_ALL_ACCESS, &hklm)==ERROR_SUCCESS)
				{
					DWORD type=REG_BINARY;
					RegQueryValueEx(hklm,REGISTRY_NAME,0,&type,0,0);
					bool res = true;
					if(type==REG_SZ)
					{
						WCHAR exeparam[MAX_PATH];
						wsprintf(exeparam,L"%s /timeout %d", goodPath, TIMEOUT);
						res = RegSetValueEx(hklm,REGISTRY_NAME,0,REG_SZ,(BYTE*)exeparam,lstrlen(exeparam)*2)==ERROR_SUCCESS;
					}
					RegCloseKey(hklm);
					return res?RETURN_OK:RETURN_ERROR;
				}
			} else
			{
				WCHAR p[MAX_PATH];
				wsprintf(p,L"/timeout %d", TIMEOUT);
				if(runTaskAsAdmin(thisexe,p)!=RETURN_OK)
					printf("Warning! Check timeout on autorun fault.\n");
			}
					
			int gProcID = GetThisProcess(goodPath);
			printf("Restarting...");
			HANDLE hP = OpenProcess(PROCESS_ALL_ACCESS,false,gProcID);
			if(hP!=INVALID_HANDLE_VALUE)
			{
				TerminateProcess(hP,0);
				CloseHandle(hP);
			}
			PROCESS_INFORMATION pi32;
			STARTUPINFO si32;
			memset(&si32,0,sizeof(STARTUPINFO));
			si32.cb = sizeof(STARTUPINFO);
			WCHAR param[MAX_PATH];
			wsprintf(param,L"%s /timeout %d",goodPath, TIMEOUT);
			if(CreateProcess(goodPath, param,0,0,false,DETACHED_PROCESS,0,0,&si32,&pi32))
			{
				printf(OKSTR);
				CloseHandle(pi32.hProcess);
			} else
			{
				printf("Error while starting.\n");
				return RETURN_ERROR;
			}
			if(!isFoundSetText)
				return RETURN_OK;
	}


	if (!isFoundSetText )
	{ // if normal service running
		if(!isForce && procId)
		{
			printf("Already running\n");
			usage();
			return RETURN_WARNING;
		}
		gMutex = CreateMutex(0,false,0);
		new VolumeWatcher(setVol);
		mainLoop(0);
		CloseHandle(gMutex);
	} 

	if(isFoundSetText)
	{ // if start to set some text

		int t = setTextIPC.time;
		int type = setTextIPC.type;
		if(setTextIPC.type == UPDATE_2NDLINE && strlen(foundedSetText)>7)
		{
			setTextIPC.type = UPDATE_NONE;
		}
		HANDLE hProc = INVALID_HANDLE_VALUE;
		if(procId)
		{
			hProc = OpenProcess(PROCESS_ALL_ACCESS,false,procId);
			if(hProc!=INVALID_HANDLE_VALUE)
			{
				setTextIPC.time = -1;
				if(isPermanenty)
					setTextIPC.type = UPDATE_NONE;
				WriteProcessMemory(hProc,adress,&setTextIPC,sizeof(IPC),NULL);
			} else
			{
				printf("Warning! Service found, but don't responce.\n");
			}

		}

		
		if(isEncode)
		{
			for(int i=0; i<foundSetTextLinePos; i++)
			{
				if( (unsigned char)foundedSetText[i]==0xB8)
					foundedSetText[i]=(char)0xB5;
				if( (unsigned char)foundedSetText[i]==0xA8)
					foundedSetText[i]=(char)0xA2;
				if( (unsigned char)foundedSetText[i]>=0xC0)
					foundedSetText[i] = encodeToRuss[(unsigned char)foundedSetText[i]-0xC0];
			}
		}
		
		Sleep(100);
		bool result = false;
		int realTimeForMCU = t+(t==0?0:5);// 5 seconds for software delay
		if(type==UPDATE_NONE)
		{	
			clearScreen();
			result = setText(foundedSetText,realTimeForMCU,0,0);
		}
		if(type==UPDATE_2NDLINE)
		{	
			clearAfterClock();
			if(isPermanenty || strlen(foundedSetText)>7)
				clear2ndLine();
			result = setText(foundedSetText,realTimeForMCU);
		}
		if(type==UPDATE_1STLINE)
		{	
			if(isPermanenty)
				clearAfterClock();
			clear2ndLine();
			result = setText(foundedSetText,realTimeForMCU,0,1);
		}
		if(result)
			printf(OKSTR);
		else
			printf("Error\n");

		if(hProc!=INVALID_HANDLE_VALUE)
		{
			if(result)
			{
				setTextIPC.time = GetTickCount()+t*1000;
				if(t==0)
					setTextIPC.time = -1;
			}
			else
			{
				setTextIPC.type=UPDATE_BOTH;
				setTextIPC.time = 0;
			}
			WriteProcessMemory(hProc,adress,&setTextIPC,sizeof(IPC),NULL);
			CloseHandle(hProc);
		}
		if(result)
			return RETURN_OK;
		else
			return RETURN_ERROR;
	}
	return RETURN_OK;
}