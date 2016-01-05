#pragma once

#include <Windows.h>
#include <d3dkmthk.h>

#define ADL_OK   0
typedef struct ADLPMActivity
{
/// Must be set to the size of the structure
	int iSize;
/// Current engine clock.
	int iEngineClock;
/// Current memory clock.
	int iMemoryClock;
/// Current core voltage.
	int iVddc;
/// GPU utilization.
	int iActivityPercent;
/// Performance level index.
	int iCurrentPerformanceLevel;
/// Current PCIE bus speed.
	int iCurrentBusSpeed;
/// Number of PCIE bus lanes.
	int iCurrentBusLanes;
/// Maximum number of PCIE bus lanes.
	int iMaximumBusLanes;
/// Reserved for future purposes.
	int iReserved;								
} ADLPMActivity;
/// Memory Allocation Call back 
typedef void* ( __stdcall *ADL_MAIN_MALLOC_CALLBACK )( int );
typedef int ( *ADL_PM_CURRENTACTIVITY_GET )(int iAdapterIndex, ADLPMActivity *lpActivity);
typedef int (*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
typedef int (*ADL_MAIN_CONTROL_DESTROY)();

struct AtiDriver { // disassembled
	int p1;//00721248  02 00 00 00 02 00 01 00  .....
	int p2;
	int p3;//00721250  00 00 00 00 00 00 00 00  ........
	int p4;
	int p5; //00721258  00 00 00 00 00 00 00 00  ........
	int p6;
	int p7; //00721260  00 00 00 00 00 00 00 00  ........
	int p8;
	int p9; //00721268  00 00 00 00 00 00 00 00  ........
	int p10;
	int p11; //00721270  00 00 00 00 00 00 00 00  ........
	int p12;
	int p13; //00721278  00 00 00 00 00 00 00 00  ........
	int p14;
	int p15; //00721280  00 00 00 00 00 00 00 00  ........
	int p16;
	int p17;//00721288  00 00 00 00 00 00 00 00  ........
	int p18;
	int p19;//00721290  0C 01 00 00 80 00 00 00  ...€...
	int p20;
	int p21; //00721298  00 00 01 00 00 00 00 03  ......
	int p22;
	int p23; //007212A0  02 00 00 00 00 00 00 00  .......
	int p24;//
	int p25;//007212A8  00 00 00 00 00 00 00 00  ........
	int p26;
	int p27;//007212B0  00 00 00 00 00 00 00 00  ........
	int p28;
	int p29;//007212B8  00 00 00 00 00 00 00 00  ........
	int p30;
	int p31;//007212C0  00 00 00 00 00 00 00 00  ........
	int p32;
	int p33; //007212C8  00 00 00 00 00 00 00 00  ........
	int p34;
	int p35; //007212D0  00 00 00 00 00 00 00 00  ........
	int p36;
	int p37; //007212D8  00 00 00 00 00 00 00 00  ........
	int p38;
	int p39; //007212E0  00 00 00 00 00 00 00 00  ........
	int p40;
	int p41; //007212E8  00 00 00 00 00 00 00 00  ........
	int p42;
	int p43; //007212F0  00 00 00 00 00 00 00 00  ........
	int p44;
	int p45; //007212F8  00 00 00 00 00 00 00 00  ........
	int p46;
	int p47; //00721300  00 00 00 00 00 00 00 00  ........
	int p48;
	int p49; //00721308  00 00 00 00 00 00 00 00  ........
	int p50;
	int p51; //00721310  00 00 00 00 10 00 00 00  ........
	int p52;
	int p53; //00721318  10 00 00 00 1A 00 C0 00  ........
	int p54;
	int p55; //00721320  00 00 00 00 00 00 00 00  ........
	int p56;//
	int p57; //00721328  28 00 00 00 28 00 00 00  (...(...
	int iSize;
	int iEngineClock; //00721330  00 00 00 00 00 00 00 00  ........
	int iMemoryClock;
	int iVddc; //	00721338  00 00 00 00 00 00 00 00  ........
	int iActivityPercent;
	int iCurrentPerformanceLevel; // 00721340  00 00 00 00 00 00 00 00  ........
	int iCurrentBusSpeed;
	int iCurrentBusLanes; // 00721348  00 00 00 00 00 00 00 00  ........
	int iMaximumBusLanes;
	int iReserved;		 // 00721350  00 00 00 00              .....
};

// magic numbers, do not change them
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  34
 
// function pointer types
typedef int *(*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int (*NvAPI_Initialize_t)();
typedef int (*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);
typedef int (*NvAPI_GPU_GetUsages_t)(int *handle, unsigned int *usages);

// Memory allocation function

class GpuUsage
{
public:
	GpuUsage(void);
	~GpuUsage(void);
	int getUsage();
private:
	int getAtiUsage();
	int getNVidiaUsage();
	NvAPI_GPU_GetUsages_t       NvAPI_GPU_GetUsages;
	int         *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS];
	/*ADL_PM_CURRENTACTIVITY_GET getActivity;
	ADL_MAIN_CONTROL_DESTROY destroy;*/
};

