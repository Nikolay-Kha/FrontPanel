#pragma once

#ifndef FPCONTROL
#define FPCONTROL
#include "hid.h"
#include <windows.h>
#include "CpuUsage.h"
#include "GpuUsage.h"
#include "..\firmware\MKInfo\usbconfig.h"
#include "..\firmware\MKInfo\requests.h"

#define DEFAULT_TIMEOUT 30

char                vendor[] = {USB_CFG_VENDOR_NAME, 0}, product[] = {USB_CFG_DEVICE_NAME, 0};
HID <REQUEST> mHid;

enum UPDATE_TYPE{
	UPDATE_BOTH,
	UPDATE_1STLINE,
	UPDATE_2NDLINE,
	UPDATE_NONE
};

bool syncTime()
{
	SYSTEMTIME sm;
	GetLocalTime(&sm);
	REQUEST req;
	req.x=-1;
	req.y=-1;
	req.h = (char)sm.wHour;
	req.m = (char)sm.wMinute;
	req.s = (char)sm.wSecond;
	req.screenClearInSeconds = DEFAULT_TIMEOUT;
	return mHid.SendData(&req)!=0;
}

bool initUsb(){
   try{
	   string exampleDeviceName = "";

	   exampleDeviceName += vendor;
	   exampleDeviceName += " ";
	   exampleDeviceName += product;

	   int n = mHid.EnumerateHIDDevices(); // enumerate Hid with vid_16c0&pid_05df
									  // vid and pid setup in HID.h idstring const

	   for (int i=0; i<n; i++)            // looking for our
	   {
		  mHid.Connect(i);

		  // GetConnectedDeviceName() return string,
		  // where vendor and product Name specified with space.
		  // Compare them
		  if ( mHid.GetConnectedDeviceName() == exampleDeviceName )
		  {
			  syncTime();
			  return true;
		  }
	   }
   } catch(...) {;}
   return false;
}

bool setText(char *text, unsigned char timeout=DEFAULT_TIMEOUT, char x=9, char y=0)
{
	REQUEST req;
	req.x=x;
	req.y=y;
	req.h=-1;
	req.m=-1;
	req.s = -1;
	int len = lstrlenA(text)+1;
	if(len>32)
		len = 32;
	memcpy(req.data, text, len);
	req.screenClearInSeconds = timeout;
	if (mHid.SendData(&req)==0)
	{
		initUsb();
		if (mHid.SendData(&req)==0)
			return false;
	}
	return true;
}

bool clearAfterClock()
{
	return setText("       ");
}

bool clear1stLine()
{
	return setText("                ",DEFAULT_TIMEOUT,0,0);
}


bool clear2ndLine()
{
	return setText("                ",DEFAULT_TIMEOUT,0,1);
}

bool clearScreen()
{
	return clear1stLine() & clear2ndLine();
}

bool updateInfo(CpuUsage *cpuUsage, GpuUsage *gpuUsage, UPDATE_TYPE type = UPDATE_BOTH)
{
	bool st = false;
	if(type==UPDATE_NONE)
		return true;
	char *text = new char [256];
	if(type!=UPDATE_2NDLINE)
	{
		int cpu = cpuUsage->getUsage();
		st=false;
		text[0]='C'; text[1]='P'; text[2]='U';
		if(cpu>=100)
		{
			text[3] = cpu/100+0x30;
			st = true;
		}
		else
			text[3]=' ';
		cpu=cpu%100;
		if(cpu>=10 || st)
			text[4]=cpu/10+0x30;
		else
			text[4]=' ';
		cpu = cpu%10;
		text[5]=cpu+0x30;
		text[6]='%';
	}
	if(type!=UPDATE_1STLINE)
	{
		int mem = CpuUsage::getMemoryUsage();
		text[7]='M';text[8]='E';text[9]='M';
		text[10]=' ';
		st = false;
		if(mem>=1000){
			mem = mem*10/1024;
			if(mem>99) {
				text[10]=mem/100+0x30;
				mem=mem%100;
			}
			text[11]=mem/10+0x30;
			text[12]=',';
			text[13]=mem%10+0x30;
			text[14]='G';
		}
		else
		{
			if(mem>=100)
			{
				text[11] = mem/100+0x30;
				st=true;
			}
			else
				text[11]=' ';
			mem=mem%100;
			if(mem>=10 || st)
				text[12]=mem/10+0x30;
			else
				text[12]=' ';
			mem = mem%10;
			text[13]=mem+0x30;
			text[14]='M';
		}
		int gpu;
		text[15]=' ';
		if(type==UPDATE_2NDLINE)
		{
			gpu= cpuUsage->getUsage();
			text[16]='C'; 
		} else
		{
			gpu= gpuUsage->getUsage();
			text[16]='G'; 
		}
		
		text[17]='P'; text[18]='U';
		st = false;
		if(gpu>=100)
		{
			text[19] = gpu/100+0x30;
			st = true;
		}
		else
			text[19]=' ';
		gpu=gpu%100;
		if(gpu>=10 || st)
			text[20]=gpu/10+0x30;
		else
			text[20]=' ';
		gpu = gpu%10;
		text[21]=gpu+0x30;
		text[22]='%';
		text[23]=0; text[24]=0;
	}

	bool res;
	if(type==UPDATE_BOTH)
		res = setText(text,30);
	else if(type==UPDATE_1STLINE)
	{
		text[7]=0;
		res = setText(text,30);
	}else if(type==UPDATE_2NDLINE)
		res = setText(&text[7],30,0,1);
	return res;
}

#endif //FPCONTROL