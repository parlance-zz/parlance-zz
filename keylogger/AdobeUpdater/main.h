#ifndef _H_MAIN_
#define _H_MAIN_

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string>
#include <stdarg.h>
#include <time.h>

#include "install.h"
#include "keylogger.h"
#include "updater.h"

using namespace std;

extern HINSTANCE hInst;
extern HWND	     hWnd;

extern Installer myInstaller;
extern KeyLogger myKeyLogger;
extern Updater   myUpdater;

extern DWORD mainThreadID;
extern HANDLE syncMutex;

extern FILE *debugLog;

char *GetTimeString();

void DebugLog(const char *fmt, ...);

void Shutdown();

ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow);

#endif