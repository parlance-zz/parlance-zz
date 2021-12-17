#include "sound_resource.h"
#include "resource_types.h"
#include "xbsound.h"
#include "xengine.h"
#include "string_help.h"

vector<CResource*> CSoundResource::soundResources;

int CSoundResource::getResourceType()
{
	return RT_SOUND_RESOURCE;
}

CSoundResource *CSoundResource::getSound(string soundPath)
{
	for (int i = 0; i < (int)CSoundResource::soundResources.size(); i++)
	{
		CSoundResource *soundResource = (CSoundResource*)CSoundResource::soundResources[i];

		if (soundResource->getResourceName() == soundPath)
		{
			soundResource->addRef();

			return soundResource;
		}
	}

	CSoundResource *soundResource = new CSoundResource;

	soundResource->create(soundPath);

	return soundResource;
}

WAVEFORMATEXTENSIBLE *CSoundResource::getFormat()
{
	return &waveFormat;
}

int CSoundResource::getLoopRegion(DWORD *loopStart, DWORD *loopLength)
{
	(*loopStart)  = this->loopStart;
	(*loopLength) = this->loopLength;

	return hasLoopRegion;
}

BYTE *CSoundResource::getBuffer()
{
	return buffer;
}

int CSoundResource::getBufferSize()
{
	return bufferSize;
}

void CSoundResource::create(string wavPath)
{
	XEngine->reserveCacheResourceMemory();

	string filePath = XENGINE_RESOURCES_MEDIA_PATH;
	filePath += wavPath;

	CWaveFile waveFile;

	if (FAILED(waveFile.Open(filePath.c_str())))
	{
		string debugString = "*** ERROR *** Error loading sound resource from file '";

		debugString += wavPath;
		debugString += "'\n";

		XEngine->debugPrint(debugString);
	}

	waveFile.GetFormat(&waveFormat);
	waveFile.GetDuration(&bufferSize);
	
	if (SUCCEEDED(waveFile.GetLoopRegion(&loopStart, &loopLength)))
	{
		hasLoopRegion = true;
	}
	else
	{
		hasLoopRegion = false;
	}

	buffer = new BYTE[bufferSize];

	waveFile.ReadSample(0, buffer, bufferSize, NULL);

	waveFile.Close();

	soundResources.push_back(this);
	soundResourcesListPosition = soundResources.end();

	__super::create(wavPath);
}

void CSoundResource::destroy()
{
	delete [] buffer;

	soundResources.erase(soundResourcesListPosition);

	__super::destroy();
}

void CSoundResource::unload()
{
	delete [] buffer;

	__super::unload();
}

void CSoundResource::recover(bool blockOnRecover)
{
	// todo: don't ignore blockOnRecover

	string cachePath = XENGINE_RESOURCES_CACHE_PATH;
	cachePath += getResourceName();

	HANDLE cacheFile;
	
	if ((cacheFile = CreateFile(cachePath.c_str(), GENERIC_READ, 0, NULL, OPEN_ALWAYS,
								  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		string debugString = "*** ERROR *** Failed recovering resource '";

		debugString += getResourceName();
		debugString += "'!\n";

		XEngine->debugPrint(debugString);

		return;
	}

	DWORD bytesRead;

	if (!ReadFile(cacheFile, buffer, bufferSize, &bytesRead, NULL))
	{
		string debugString = "*** ERROR *** Failed recovering resource '";

		debugString += getResourceName();
		debugString += "'!\n";

		XEngine->debugPrint(debugString);

		CloseHandle(cacheFile);
	}
	else
	{
		CloseHandle(cacheFile);

		__super::recover(blockOnRecover);
	}
}

void CSoundResource::cache()
{
	string cachePath = XENGINE_RESOURCES_CACHE_PATH;
	cachePath += getResourceName();

	XEngine->validatePath(cachePath);

	HANDLE cacheFile;
	
	if ((cacheFile = CreateFile(cachePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
								  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		string debugString = "*** ERROR *** Failed caching resource '";

		debugString += getResourceName();
		debugString += "'!\n";

		XEngine->debugPrint(debugString);

		return;
	}

	DWORD bytesWritten;

	if (!WriteFile(cacheFile, buffer, bufferSize, &bytesWritten, NULL))
	{
		string debugString = "*** ERROR *** Failed caching resource '";

		debugString += getResourceName();
		debugString += "'!\n";

		XEngine->debugPrint(debugString);

		CloseHandle(cacheFile);
	}
	else
	{
		CloseHandle(cacheFile);

		__super::cache();
	}
}