#ifndef _H_INSTALL_
#define _H_INSTALL_

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#ifdef _DEBUG

#define NO_INSTALL

#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace std;

class Installer
{
private:

	string installPath;
	string appPath;

	int CreateShortcut(string ptchExecutableFileName, string ptchShortcutName);

public:

	string GetInstallPath();
	string GetAppPath();

	int Install();

	int Shutdown();
};

#endif