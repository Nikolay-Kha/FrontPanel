#pragma once


#ifndef VOLUMEWATCHER
#define VOLUMEWATCHER

#include <Windows.h>


typedef void (* VolumeCallBack)(int);
struct IAudioEndpointVolume;
class CVolumeNotification;

class VolumeWatcher
{
public:
	VolumeWatcher(VolumeCallBack callBackFunc);
	~VolumeWatcher();
private:
	IAudioEndpointVolume *endpointVolume;
	CVolumeNotification *volumeNotification;
};

#endif //VOLUMEWATCHER