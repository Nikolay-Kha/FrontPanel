#include "GpuUsage.h"
#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) // ntsubauth

/*void* __stdcall ADL_Main_Memory_Alloc(int iSize) {
	void* lpBuffer = malloc(iSize);
	return lpBuffer;
}*/

void EnableTakeOwnershipName()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	OpenProcessToken(GetCurrentProcess(),
	TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	LookupPrivilegeValue(NULL, SE_TAKE_OWNERSHIP_NAME,
	   &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
	   (PTOKEN_PRIVILEGES)NULL, 0);
	CloseHandle(hToken);
}



GpuUsage::GpuUsage(void)
{
/*	getActivity = NULL;
	HMODULE hDLL = LoadLibrary(L"atiadlxy.dll");
	if(hDLL!=NULL)
	{
		ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE) GetProcAddress(hDLL,"ADL_Main_Control_Create");
		destroy = (ADL_MAIN_CONTROL_DESTROY) GetProcAddress(hDLL,"ADL_Main_Control_Destroy");
		if (destroy &&  ADL_OK == ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1))
		{
			getActivity = (ADL_PM_CURRENTACTIVITY_GET) GetProcAddress(hDLL,"ADL_Overdrive5_CurrentActivity_Get");
		}
	}*/

	memset(gpuHandles, 0, sizeof (gpuHandles));
	NvAPI_GPU_GetUsages      = NULL;
	HMODULE hNVidiaMod = LoadLibraryA("nvapi.dll");

	if ( hNVidiaMod!=NULL ) 
	{
		// nvapi.dll internal function pointers
		NvAPI_QueryInterface_t      NvAPI_QueryInterface     = NULL;
		NvAPI_Initialize_t          NvAPI_Initialize         = NULL;
		NvAPI_EnumPhysicalGPUs_t    NvAPI_EnumPhysicalGPUs   = NULL;
 
		// nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
		NvAPI_QueryInterface = (NvAPI_QueryInterface_t) GetProcAddress(hNVidiaMod, "nvapi_QueryInterface");
 
		// some useful internal functions that aren't exported by nvapi.dll
		NvAPI_Initialize = (NvAPI_Initialize_t) (*NvAPI_QueryInterface)(0x0150E828);
		NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t) (*NvAPI_QueryInterface)(0xE5AC921F);
		NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t) (*NvAPI_QueryInterface)(0x189A1FDF);
 
		if ( !(NvAPI_Initialize == NULL || NvAPI_EnumPhysicalGPUs == NULL ||
			NvAPI_EnumPhysicalGPUs == NULL || NvAPI_GPU_GetUsages == NULL) )
		{
			// initialize NvAPI library, call it once before calling any other NvAPI functions
			(*NvAPI_Initialize)();
 
			int          gpuCount = 0;
			unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };
 
			(*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);
		}
	}
}



/*int GpuUsage::getUsage()
{
	if(getActivity!=NULL){
		ADLPMActivity act;
		if(getActivity(0,&act)==ADL_OK)
			return act.iActivityPercent;
	}
	return -1;
}*/


GpuUsage::~GpuUsage(void)
{
	//destroy();
}

int GpuUsage::getAtiUsage()
{
	/*HDC hdc = CreateDCA(0,"\\\\.\\DISPLAY1",0,0);
	if(hdc==NULL)
		return -1;*/
	NTSTATUS res;
	//D3DKMT_OPENADAPTERFROMHDC oafh;
	D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME oadn;
	//oafh.hDc = hdc;
	lstrcpyW(oadn.DeviceName, L"\\\\.\\DISPLAY1");
	res = D3DKMTOpenAdapterFromGdiDisplayName(&oadn);
	//res = D3DKMTOpenAdapterFromHdc(&oafh);
	if(res!=STATUS_SUCCESS)
	{
		//DeleteDC(hdc);
		return -1;
	}

	D3DKMT_ESCAPE esc;
	esc.hAdapter = oadn.hAdapter;
	esc.hDevice = 0;
	esc.Type = D3DKMT_ESCAPE_DRIVERPRIVATE; 
	D3DDDI_ESCAPEFLAGS flag;
	flag.HardwareAccess=2;
	flag.Reserved=0;
	flag.Value=0x10002;
	esc.Flags = flag;

	AtiDriver ad;
	memset(&ad,0,sizeof(ad));
	ad.p1=2;
	ad.p2=0x010002;
	ad.p19 = 0x10c;
	ad.p20 = 0x80;
	ad.p21 = 0x10000;
	ad.p22 = 0x3000000;
	ad.p23 = 2;
	ad.p52=0x10;
	ad.p53=0x10;
	ad.p54=0xc0001a;
	ad.p57=0x28;
	ad.iSize=0x28;


	esc.pPrivateDriverData = &ad;
	esc.PrivateDriverDataSize = sizeof(AtiDriver);//0x10c;
	esc.hContext = 0;

	res = D3DKMTEscape(&esc);
	D3DKMT_CLOSEADAPTER ca;
	ca.hAdapter = oadn.hAdapter;
	D3DKMTCloseAdapter(&ca);
	if(res==STATUS_SUCCESS)
		return ad.iActivityPercent;
	return -1;
}

int GpuUsage::getNVidiaUsage()
{

    unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };
 
    // gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
    gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;
    (*NvAPI_GPU_GetUsages)(gpuHandles[0], gpuUsages);
    return gpuUsages[3];
}

int GpuUsage::getUsage()
{
	int usage = -1;
	if(NvAPI_GPU_GetUsages!=NULL)
	{
		usage = getNVidiaUsage();
	}
	if(usage == -1)
	{
		usage = getAtiUsage();
	}
	return usage;
}
