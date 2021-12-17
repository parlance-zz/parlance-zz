#ifndef _H_SOUND_RESOURCE
#define _H_SOUND_RESOURCE

#include <xtl.h>
#include <string>
#include <stdio.h>
#include <vector>

//#include <dsound.h>

#include "cache_resource.h"

using namespace std;

class CSoundResource : public CCacheResource
{
public:

	int getResourceType();

	void create(string wavPath);

	void destroy(); // additionally, this releases the sound buffers / memory

	static vector<CResource*> soundResources;	// main sound resource list

	WAVEFORMATEXTENSIBLE *getFormat();
	int getLoopRegion(DWORD *loopStart, DWORD *loopLength);

	BYTE *getBuffer();
	int   getBufferSize();

	static CSoundResource *getSound(string soundPath);

	// caching mechanisms

	void unload();
	void cache();

protected:

	void recover(bool blockOnRecover);

private:

	vector<CResource*>::iterator soundResourcesListPosition; // keeps track of where in the sound resource list this resource resides

	// defines the loop points, if any

	bool hasLoopRegion;

	DWORD loopStart;
	DWORD loopLength;

	// the format of our wave data

	WAVEFORMATEXTENSIBLE waveFormat;

	// and the raw wave data

    DWORD bufferSize;
	BYTE *buffer;
};

#endif