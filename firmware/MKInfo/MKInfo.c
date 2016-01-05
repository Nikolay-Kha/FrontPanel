/*
 * MKInfo.c
 *
 * Created: March, 25 2012 12:03:55
 *  Author: Nikolay Khabarov
 */ 

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <avr/eeprom.h>
#include <avr/iom8.h>
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */

#include "usbdrv.h"
#include "lcd_lib.h"
#include "requests.h"

const uint8_t LCDwelcomeln1[] PROGMEM="-----Hello!-----";
const uint8_t LCDwelcomeln2[] PROGMEM="**Front  Panel**";

struct REQUEST pdata;

uint8_t mTimerInit = 0;
uint8_t mTickSeconds=0;
uint8_t mLastPrintSeconds=0;
uint8_t mHidelock=0;
uint8_t mClearTime=0;
uint8_t mSeconds=0;
uint8_t mMinutes=0;
uint8_t mHours=0;



/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM const char usbHidReportDescriptor[22] = { // USB report descriptor         // Describes struct for communication
    0x06, 0x00, 0xff,                       // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                             // USAGE (Vendor Usage 1)
    0xa1, 0x01,                             // COLLECTION (Application)
    0x15, 0x00,                             //    LOGICAL_MINIMUM (0)        // min. value for data
    0x26, 0xff, 0x00,                       //    LOGICAL_MAXIMUM (255)      // max. value for data, 255 to fit in 1 byte
    0x75, 0x08,                             //    REPORT_SIZE (8)            // data sends with small pieces, each size of them is 8 bit
    0x95, sizeof(struct REQUEST),    //    REPORT_COUNT               // number of pieces (we have = 3, this struct should pass in three 'report')
    0x09, 0x00,                             //    USAGE (Undefined)
    0xb2, 0x02, 0x01,                       //    FEATURE (Data,Var,Abs,Buf)
    0xc0                                    // END_COLLECTION
};
/* Since we define only one feature report, we don't use report-IDs (which
 * would be the first byte of the report). The entire report consists of 128
 * opaque data bytes.
 */

/* The following variables store the status of the current data transfer */
static uchar    currentAddress;
static uchar    bytesRemaining;

/* ------------------------------------------------------------------------- */

/* usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar   usbFunctionRead(uchar *data, uchar len)
{
     if(len > bytesRemaining)
        len = bytesRemaining;

    uchar *buffer = (uchar*)&pdata;

    if(!currentAddress)        // Nothing was read.
    {                          // Prepare struct for trsnamission
       pdata.x = PINB;
	   pdata.y = PINC;
    }

    uchar j;
    for(j=0; j<len; j++)
        data[j] = buffer[j+currentAddress];

    currentAddress += len;
    bytesRemaining -= len;
    return len;
}

/* usbFunctionWrite() is called when the host sends a chunk of data to the
 * device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar   usbFunctionWrite(uchar *data, uchar len)
{
    if(bytesRemaining == 0)
        return 1;               /* Transmission finished */

    if(len > bytesRemaining)
        len = bytesRemaining;

    uchar *buffer = (uchar*)&pdata;
    
    uchar j;
    for(j=0; j<len; j++)
        buffer[j+currentAddress] = data[j];

    currentAddress += len;
    bytesRemaining -= len;

    if(bytesRemaining == 0)     // Transmission finished
    {          
		// receive
		// clear screen with screenClearInSeconds till last message
		// if screenClearInSeconds == 0 then screen will never be cleaned
		// if text cover clock, clock stops updating until screen is cleaned
		// with screenClearInSeconds or time is set up
		// Set up clock if s!=-1
		if(mTimerInit==0){
			mTimerInit = 1;
			TIMSK=0b00001000; // allow comparator interruption
			TCCR1A = 0;
			TCCR1B=0b00000100; // tacts = CK/256
			OCR1B=0xB71B; // initialize comparator with 46875 tacts (1 sec)
			TCNT1=0;
		}
		if(pdata.y==0 && pdata.x<=8)
			mHidelock = 1;
		uint8_t xpos = pdata.x;
		uint8_t ypos = pdata.y;
		if(xpos>=0 && xpos<16 && ypos>=0 && ypos< 2) // display physics parameters
		{	
			LCDGotoXY(xpos, ypos);
			for(int8_t i=0; i<32 && pdata.data[i]!=0 && xpos<16; i++)   
			{        
				LCDsendChar(pdata.data[i]);
				xpos++;
				if(xpos>=16 && ypos<1)
				{
					ypos++;
					xpos = 0;
					LCDGotoXY(xpos, ypos);	
				}
				if(i==16)
					usbPoll();
			}				
		}			
		if(pdata.screenClearInSeconds==0)
		{
			mClearTime = 0;
		}else{				
			mClearTime = mTickSeconds+pdata.screenClearInSeconds;
			if(mClearTime==0)
				mClearTime = 1;
		}	
		if(pdata.s>=0 && pdata.s<60 && pdata.m>=0 && pdata.m<60 && pdata.h>=0 && pdata.h<24)	{
			mHidelock = 0;
			mSeconds=pdata.s;
			mMinutes=pdata.m;
			mHours=pdata.h;
		}				
    }

    return bytesRemaining == 0; /* 0 means that we have more data */
}

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* device HID */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            // we have only one report-ID, so ignore everything else
            bytesRemaining = sizeof(struct REQUEST);
            currentAddress = 0;
            return USB_NO_MSG;  // use usbFunctionRead() to send data to host
        }else if(rq->bRequest == USBRQ_HID_SET_REPORT){
            // one one type of report-ID
            bytesRemaining = sizeof(struct REQUEST);
            currentAddress = 0;
            return USB_NO_MSG;  // use usbFunctionWrite() to receive data from host
        }
    }else{
        /* ignore everything else */
    }
    return 0;
}

void printTime()
{
	LCDGotoXY(0,0);
	LCDsendChar(mHours/10+0x30);
	LCDsendChar(mHours%10+0x30);
	LCDsendChar(':');
	LCDsendChar(mMinutes/10+0x30);
	LCDsendChar(mMinutes%10+0x30);
	LCDsendChar(':');
	LCDsendChar(mSeconds/10+0x30);
	LCDsendChar(mSeconds%10+0x30);
	LCDsendChar(' ');
	
}


/* ------------------------------------------------------------------------- */

int main(void)
{
    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */

	cli();
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    uchar i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
	LCDinit();//init LCD bit, dual line, cursor right
	LCDclr();//clears LCD
	CopyStringtoLCD(LCDwelcomeln1, 0, 0);
	CopyStringtoLCD(LCDwelcomeln2, 0, 1);
	
	DDRB = DDRB & 0b11001111;

	
    usbDeviceConnect();
    sei();
    for(;;){                /* main event loop */
        wdt_reset();
        usbPoll();
		if(mLastPrintSeconds!=mTickSeconds)
		{
			mSeconds+=mTickSeconds-mLastPrintSeconds;
			mLastPrintSeconds = mTickSeconds;
			if (mSeconds>=60){
				mSeconds=0;
				mMinutes++;
				if(mMinutes>=60)
				{
					mMinutes=0;
					mHours++;
					if(mHours>=24)
						mHours = 0;
				}
			}
			if(mClearTime!=0){
				if(mClearTime==mTickSeconds){
					LCDGotoXY(8,0);
					for(int i=0; i<8;i++)
						LCDsendChar(' ');	
					usbPoll();
					LCDGotoXY(0,1);
					for(int i=0; i<8;i++)
						LCDsendChar(' ');	
					wdt_reset();	
					usbPoll();	
					for(int i=0; i<8;i++)
						LCDsendChar(' ');	
					usbPoll();
					mClearTime = 0;	
					mHidelock = 0;
				}
			}
			if(mHidelock==0)	
				printTime();
		}
    }
    return 0;
}

ISR(TIMER1_COMPB_vect,ISR_NOBLOCK)
{
    TCNT1=0;
    mTickSeconds++;
}

/* ------------------------------------------------------------------------- */
