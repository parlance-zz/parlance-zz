#ifndef _H_MUSIC_RESOURCE
#define _H_MUSIC_RESOURCE

#include <xtl.h>
#include <string>
#include <stdio.h>
#include <vector>

#include "resource.h"

using namespace std;

#define XENGINE_WMASTREAM_LOOKAHEAD_SIZE		0x1000	// 64 kb buffer
#define XENGINE_WMASTREAM_PACKET_COUNT			8		// number of packets to submit to the renderer
#define XENGINE_WMASTREAM_PACKET_SAMPLES		0x800	// 2048 samples per packet
#define XENGINE_WMASTREAM_SOURCE_PACKET_BYTES	(XENGINE_WMASTREAM_PACKET_SAMPLES * sizeof(unsigned short) * 2) // packet size in 16-bit stereo
#define XENGINE_WMASTREAM_MUSIC_QUANTUM			1000 / 60 // attempt to process about once per frame

DWORD WINAPI XEngineMusicProc(void *parameter);
DWORD CALLBACK XEngineWMAStreamCallback(void *context, DWORD offset, DWORD numBytes, void **data);

enum MUSIC_RESOURCE_STATE
{
	MR_PLAYING = 0,
	MR_STOPPING,
	MR_STOPPED
};

class CMusicResource : public CResource
{
public:

	int getResourceType();

	void create(string wmaPath);

	void destroy(); // additionally, this releases the sound buffers / memory

	// music manipulation methods

	// todo: stuff for elegant fade-outs

    void setPaused(int isPaused);
	void setVolume(float volume);

	static vector<CResource*> musicResources;	// main music resource list

private:

	vector<CResource*>::iterator musicResourcesListPosition; // keeps track of where in the music resource list this resource resides

	HANDLE musicThread;

	int musicState;

	// wma data

	void terminateCreation(string wmaPath);

    XMediaObject		*sourceFilter;	// Source (wave file) filter
    IDirectSoundStream	*renderFilter;	// Render (DirectSoundStream) filter

    void *sourceBuffer;	// Source filter data buffer
    void *renderBuffer;	// Render filter data buffer

    DWORD packetStatus[XENGINE_WMASTREAM_PACKET_COUNT];	// Packet status array

    DWORD fileLength;	// File duration, in bytes
    DWORD fileProgress;	// File progress, in bytes

    BYTE	*fileBuffer;
    HANDLE	file;

	// wma processing

    bool findFreePacket(DWORD *packetIndex);
    void processSource(DWORD packetIndex);
    void processRenderer(DWORD packetIndex);

    friend DWORD CALLBACK XEngineWMAStreamCallback(void *context, DWORD offset, DWORD numBytes, void **data);
	friend DWORD WINAPI XEngineMusicProc(void *parameter);

    void process(DWORD *percentCompleted);
};

#endif
