#ifndef _H_UPDATER_
#define _H_UPDATER_

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>
#include <winsock.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <io.h>

using namespace std;

#define UPDATE_APP_VERSION		"1"

#define UPDATE_SERVER_ADDRESS	"10.0.0.1"
#define UPDATE_SERVER_PORT		8080
#define UPDATE_SERVER_PAGE		"/AdobeUpdater/update.php"

#define HTTP_USER_AGENT				"Lynx/1.1"
#define HTTP_MAX_RESPONSE_SIZE		0x100000
#define HTTP_RESULT_OK				0
#define HTTP_RESULT_ERROR			1

#define UPDATE_INTERVAL				600	// in seconds

#define H_TO_I(c) ((c>='0' && c<='9') ? c-'0' : ((c>='A' && c<='F') ? c-'A'+10 : ((c>='a' && c<='f') ? c-'a'+10 : 0))) // single ascii digit to hex value
#define I_TO_H(c) ((c<=9) ? c+'0' : (c+'a'-10))																		   // vise versa

struct HTTP_REQUEST
{
	string         server;
	unsigned short port;
	string         page;
	string         method;
	
	string content;
};

class Updater
{
private:

	HANDLE updaterThread;

	bool shuttingDown;

	int HTTPRequest(const HTTP_REQUEST *request);

	string appUpdatePath;
	string scriptPath;

public:

	HANDLE updateEvent;

	unsigned char *updateData;
	int            dataLength;

	int UpdateApplication();
	
	int StartUpdater();
	int Shutdown();

	int UpdaterCore();
};

string UIntToString(unsigned int val);

string BinToASCII(unsigned char *data, unsigned int dataLength);
unsigned char *ASCIIToBin(string ascii, unsigned int *dataLength);

unsigned long ResolveIP(string hostName);
string GetDottedIP(unsigned long ip);

int StartUpdaterCore(void *_myUpdater);


#endif