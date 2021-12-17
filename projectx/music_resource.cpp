#include "music_resource.h"
#include "resource_types.h"
#include "xengine.h"
#include "string_help.h"

vector<CResource*> CMusicResource::musicResources;

DWORD WINAPI XEngineMusicProc(void *parameter)
{ 
	CMusicResource *musicResource = (CMusicResource*)parameter;

    DWORD completed;

    while (musicResource->musicState != MR_STOPPING)
    {
        musicResource->process(&completed);

        Sleep(XENGINE_WMASTREAM_MUSIC_QUANTUM);
    }

	musicResource->musicState = MR_STOPPED;

	return 0;
}

DWORD CALLBACK XEngineWMAStreamCallback(void *context, DWORD offset, DWORD numBytes, void **data)
{
    CMusicResource *musicResource = (CMusicResource*)context;

    *data = musicResource->fileBuffer + offset;

    musicResource->fileProgress = offset;

    return numBytes;
}

int CMusicResource::getResourceType()
{
	return RT_MUSIC_RESOURCE;
}

void CMusicResource::setPaused(int isPaused)
{
	if (isPaused)
	{
		renderFilter->Pause(DSSTREAMPAUSE_PAUSE);
	}
	else
	{
		renderFilter->Pause(DSSTREAMPAUSE_RESUME);
	}
}

void CMusicResource::setVolume(float volume)
{
	if (volume < 0.0f)
	{
		volume = 0.0f;
	}
	else if (volume > 1.0f)
	{
		volume = 1.0f;
	}

	LONG newVolume = -6400 + LONG(volume * 6400.0f); // yay for dsound being silly

	HRESULT result = renderFilter->SetVolume(newVolume);
}

// todo: all the things that go wrong should be handled elegantly

void CMusicResource::terminateCreation(string wmaPath)
{
	string debugString = "*** ERROR *** Error loading music resource from file '";

	debugString += wmaPath;
	debugString += "'\n";

	XEngine->debugPrint(debugString);

	file = NULL;

	//destroy();

	return;
}

void CMusicResource::create(string wmaPath)
{
	string filePath = XENGINE_RESOURCES_MEDIA_PATH;
	filePath += wmaPath;

    sourceFilter = NULL;
    renderFilter = NULL;
    sourceBuffer = NULL;
    
    for (int i = 0; i < XENGINE_WMASTREAM_PACKET_COUNT; i++)
	{
        packetStatus[i] = XMEDIAPACKET_STATUS_SUCCESS;
	}

    fileLength   = 0;
    fileProgress = 0;

    fileBuffer = NULL;
    file	   = NULL;

    WAVEFORMATEX   sourceFormat;
    DSSTREAMDESC   streamDesc;

    file = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (file == INVALID_HANDLE_VALUE)
	{
		terminateCreation(wmaPath);

		return;
    }

    fileLength = SetFilePointer(file, 0, NULL, FILE_END);

    if (fileLength == INVALID_SET_FILE_POINTER)
	{
		terminateCreation(wmaPath);

		return;
    }

    SetFilePointer(file, 0, NULL, FILE_BEGIN);

    fileBuffer = new BYTE[fileLength];

    DWORD bytesRead = 0;

    if (!ReadFile(file, fileBuffer, fileLength, &bytesRead, NULL))
	{
		terminateCreation(wmaPath);

		return;
    }

	CloseHandle(file); // todo: maybe I shouldn't?

    HRESULT result = WmaCreateInMemoryDecoder(XEngineWMAStreamCallback, this, 0, &sourceFormat, &sourceFilter);

    if (FAILED(result))
	{
		terminateCreation(wmaPath);

		return;
	}

    ZeroMemory(&streamDesc, sizeof(streamDesc));

	// todo: come back to this if I ever care about having 3D music emitters from WMAs, possibly for long speeches, whatever

    streamDesc.dwMaxAttachedPackets = XENGINE_WMASTREAM_PACKET_COUNT;
    streamDesc.lpwfxFormat          = &sourceFormat;

    result = DirectSoundCreateStream(&streamDesc, &renderFilter);

    if (FAILED(result))
	{
        terminateCreation(wmaPath);

		return;
	}

    sourceBuffer = new BYTE[XENGINE_WMASTREAM_SOURCE_PACKET_BYTES * XENGINE_WMASTREAM_PACKET_COUNT];

	//DWORD threadID;

	musicState = MR_PLAYING;
	setPaused(true);

	musicThread = CreateThread(NULL, 0, XEngineMusicProc, this, 0, NULL);

	//ResumeThread(musicThread);

	musicResources.push_back(this);
	musicResourcesListPosition = musicResources.end();

	__super::create(wmaPath);
}

void CMusicResource::destroy()
{
	setVolume(0.0f);
	setPaused(true);

	musicState = MR_STOPPING;

	while (musicState != MR_STOPPED)
	{
	}

	CloseHandle(musicThread);

    if (sourceFilter)
	{
		sourceFilter->Release();
	}

    if (renderFilter)
	{
		renderFilter->Release();
	}

    if (sourceBuffer)
	{
		delete [] sourceBuffer;
	}

    if (fileBuffer)
	{
		delete fileBuffer;
	}

    if (file)
	{
		CloseHandle(file);
	}
    
	musicResources.erase(musicResourcesListPosition);

	__super::destroy();
}

void CMusicResource::process(DWORD *percentCompleted)
{
    DWORD packetIndex;

    while (findFreePacket(&packetIndex))
    {
         processSource(packetIndex);
         processRenderer(packetIndex);
    }

    if (percentCompleted)
	{
        (*percentCompleted) = fileProgress * 100 / fileLength;
	}
}

bool CMusicResource::findFreePacket(DWORD *packetIndex)
{
    for (int i = 0; i < XENGINE_WMASTREAM_PACKET_COUNT; i++)
    {
        if (packetStatus[i] != XMEDIAPACKET_STATUS_PENDING)
        {
            if(packetIndex)
			{
                (*packetIndex) = i;
			}

            return true;
        }
    }

    return false;
}

void CMusicResource::processSource(DWORD packetIndex)
{
    DWORD totalSourceUsed = 0;
    DWORD sourceUsed;

    XMEDIAPACKET xMediaPacket;

    ZeroMemory(&xMediaPacket, sizeof(xMediaPacket));

    xMediaPacket.pvBuffer         = (BYTE*)sourceBuffer + (packetIndex * XENGINE_WMASTREAM_SOURCE_PACKET_BYTES);
    xMediaPacket.dwMaxSize        = XENGINE_WMASTREAM_SOURCE_PACKET_BYTES;
    xMediaPacket.pdwCompletedSize = &sourceUsed;

    while (totalSourceUsed < XENGINE_WMASTREAM_SOURCE_PACKET_BYTES)
    {
		sourceFilter->Process(NULL, &xMediaPacket);

        totalSourceUsed += sourceUsed;

        if (sourceUsed < xMediaPacket.dwMaxSize)
        {
            xMediaPacket.pvBuffer  = (BYTE*)xMediaPacket.pvBuffer + sourceUsed;
            xMediaPacket.dwMaxSize = xMediaPacket.dwMaxSize - sourceUsed;
            
            sourceFilter->Flush();
        }
    }
}

void CMusicResource::processRenderer(DWORD packetIndex)
{
    XMEDIAPACKET xMediaPacket;

    ZeroMemory(&xMediaPacket, sizeof(xMediaPacket));

    xMediaPacket.pvBuffer  = (BYTE*)sourceBuffer + (packetIndex * XENGINE_WMASTREAM_SOURCE_PACKET_BYTES);
    xMediaPacket.dwMaxSize = XENGINE_WMASTREAM_SOURCE_PACKET_BYTES;
    xMediaPacket.pdwStatus = &packetStatus[packetIndex];

    renderFilter->Process(&xMediaPacket, NULL);
}
