#include "VolumeWatcher.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <iostream>

#define CLASS_NAME L"WindowClassForVolumeWatcher"

class CVolumeNotification : public IAudioEndpointVolumeCallback 
{ 
	VolumeCallBack mCB;
    LONG m_RefCount; 
    ~CVolumeNotification(void) {}; 
public: 
    CVolumeNotification(VolumeCallBack cb) : m_RefCount(1) 
    { 
		mCB = cb;
    } 
    STDMETHODIMP_(ULONG)AddRef() { return InterlockedIncrement(&m_RefCount); } 
    STDMETHODIMP_(ULONG)Release()  
    { 
        LONG ref = InterlockedDecrement(&m_RefCount);  
        if (ref == 0) 
            delete this; 
        return ref; 
    } 
    STDMETHODIMP QueryInterface(REFIID IID, void **ReturnValue) 
    { 
        if (IID == IID_IUnknown || IID== __uuidof(IAudioEndpointVolumeCallback))  
        { 
            *ReturnValue = static_cast<IUnknown*>(this); 
            AddRef(); 
            return S_OK; 
        } 
        *ReturnValue = NULL; 
        return E_NOINTERFACE; 
    } 

    STDMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA NotificationData) 
    { 
		mCB((NotificationData->bMuted?-1:1) * int(NotificationData->fMasterVolume*100.0f+0.5f));
        return S_OK; 
    } 
}; 


VolumeWatcher::VolumeWatcher(VolumeCallBack callBackFunc)
{
	CoInitialize(NULL);
    HRESULT hr;
    IMMDeviceEnumerator *deviceEnumerator = NULL;;

    // 
    //    Instantiate an endpoint volume object.
    //
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
    IMMDevice *defaultDevice = NULL;

    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    deviceEnumerator->Release();
    deviceEnumerator = NULL;

    endpointVolume = NULL;
	if(defaultDevice)
	{
		hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
		if(endpointVolume)
		{
			volumeNotification = new CVolumeNotification(callBackFunc); 
			hr = endpointVolume->RegisterControlChangeNotify(volumeNotification); 
			defaultDevice->Release(); 
			defaultDevice = NULL; 
		}
	}
}

VolumeWatcher::~VolumeWatcher()
{
	if(endpointVolume)
	{
		endpointVolume->UnregisterControlChangeNotify(volumeNotification); 
		endpointVolume->Release(); 
		volumeNotification->Release(); 
	}

    CoUninitialize();
}
