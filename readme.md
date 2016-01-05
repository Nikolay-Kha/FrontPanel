# Front Panel Project
This project displays information on LCD1602 display from you desktop PC. LCD
display can be mounted in 5.25" or 3.5" drive bay of your PC. Device based on
ATmega8 MCU and connects to PC with USB interface.
![](photo.jpg?raw=true)

Features:
- display current time (synced with PC) even when PC is turned off(using +5V
 stand by in USB port)
- display CPU, GPU (NVIDIA and ATI driver support) load and RAM memory usage
- display volume on changing(very useful with hardware keyboard's buttons)
- display any text you want with console util
- no usb drivers require
- works with Windows XP, Vista, 7, 8, 10

# Dependencies
To build from sources firmware part of project can be opened with Atmel Studio
(tested with version 6.2) in located in 'firmware' directory. Software part can
oppened with Microsoft Visual Studio (tested with MS VS Express 2015) and also
require Windows Driver Kit Version 7.1.0 which can be installed - 
https://www.microsoft.com/en-us/download/details.aspx?id=11800 . Located
in 'software' dir.  
Project also contains some 3rd party files with GNU GPL license: V-USB, lcd_lib,
oddebug. Copy of these files are included in sources.

# Circuit diagram
![](circuit.png?raw=true)

# Usage
After connection to USB bus Front Panel shows welcome screen. And nothing will
chnage until special util starts. Compile and run software part - it installs it
automatically in C:\Windows directory and autorun  
HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run  
Util will be available in command line as "fp". While computer is booting util runs
and update LCD with current status and sync clock on it. When computer shutdown,
Front Panel sense that no updates is coming and left only clock on display until
power is present in USB bus. Command line util allow to print any text you want.
So, 'fp' util can be used by any other 3rd party software to show notifications,
event or any other stuff.

# Command line util usage
fp.exe [/argumentsBelow] Text  
/noencode - not encoding character  
/t X - showing message time. Only in 0...59 seconds. 0 - permanently  
/a - all screen  
/c - after clock  
/s - second line - default  
/ap /sp /cp - the same as above, but not update another parts of screen, not
same as "/t 0" this 6 arguments not combined with itself.  
  
Not combined:  
/install - install  autorun   
/uninstall - uninstall  autorun  
/force - force start in current dir, not installing or touch service  
/timeout x - set refresh timeout in ms. Allow in 100...20000  
/help show this  
  
Return code:  
0 - OK  
1 - Warning  
-1 - Error  
-2 - Arguments error  
