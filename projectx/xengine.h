#ifndef _H_XENGINE
#define _H_XENGINE

#include <xtl.h>
#include <xfont.h>

#include <stdio.h>
#include <string>
#include <vector>
#include <list>
#include <math.h>
#include <fstream>

using namespace std;

#include "engine_events.h"
#include "input_structs.h"

// timing defines / globals

#define XENGINE_TIMING_MAX_TWEEN			4.0f			// todo: values may need tweaking
#define XENGINE_TIMING_STUTTER_THRESHOLD	0.4f
#define XENGINE_TIMING_STUTTER_SMOOTHING	0.5f

// graphics defines / globals

#define XENGINE_GRAPHICS_WIDTH  640
#define XENGINE_GRAPHICS_HEIGHT 480

#define XENGINE_GRAPHICS_PUSHBUFFER_SIZE	(1024 * 1024) // (1024 * 512) // 1 MB pushbuffer
#define XENGINE_GRAPHICS_KICKOFF_SIZE		(1024 * 256)  // (1024 * 32)  // 256 kb kickoff size

#define XENGINE_GRAPHICS_MAX_OPAQUE_SURFACE_INSTANCES  0x1000 // 4096 max opaque surface instances, after that they will
															  // will simply be discarded =\

#define XENGINE_GRAPHICS_MAX_BLENDED_SURFACE_INSTANCES 0x800 // 2048 max blended surface instances, after that they will
															 // will simply be discarded =\

// resources defines / globals

#define XENGINE_RESOURCES_MEDIA_PATH   "d:\\"
#define XENGINE_RESOURCES_CACHE_PATH   "z:\\"

#define XENGINE_RESOURCES_SHADER_PATH  "scripts\\";

#define XENGINE_RESOURCE_MINIMUM_FREE_MEMORY (1024 * 1024 * 4) // 4 MB threshold, may need adjusting

// audio defines / globals

#define XENGINE_AUDIO_DOPPLER_FACTOR	2.0f // may need tweaking
#define XENGINE_AUDIO_DISTANCE_FACTOR	2.0f // number of meters in a vector unit
#define XENGINE_AUDIO_ROLLOFF_FACTOR	1.0f

// graphics structs

enum XENGINE_LIGHTING_TYPE
{
	XLT_NOLIGHTING = 0,
	XLT_LIGHTMAP,		// this doesn't refer to a bsp lightmap, in this case it refers to the technique used
	XLT_VERTEXLIGHTING	//                                                    to dynamicly light this surface
};

struct XSURFACE
{
	class CVertexBufferResource *vertexBuffer;	// main surface vertex buffer, mapped to stream 0
	class CIndexBufferResource  *indexBuffer;	// main surfaces indices

	CVertexBufferResource *persistantAttributes; // secondary vertex buffer used for storing immutable vertex attributes
												 // in a dynamic animated surface. usually NULL and unused.

	// any of these can be null if they are not used
	// the shader will be sought after optimally, then the texture/lightmap pair

	class CShaderResource  *shader;
	class CTextureResource *texture;
	CTextureResource	   *lightmap;

	D3DXVECTOR3 pos;		// average of the bounding box defined implicitly in this surfaces vertices, used for sorting

	int lightingType;		// some member of the XENGINE_LIGHTING_TYPE enum describing how the light the surface
							// given the surface instance lights

	int lastFrameRendered;
};

struct XSURFACE_INSTANCE
{
	XSURFACE *surface;
	
	int lastFrameRendered;

	// ought to contain transform info, entity color info, lighting info, animation info

	bool hasTransform;
	D3DXMATRIX world;

	bool hasColor;
	D3DCOLOR entityColor;

	// used for vertex animated surfaces

	CVertexBufferResource *target; // corresponds to the target vertex buffer of a lerp animation, can be NULL
								   // if there is no lerp animation. must be NULL if the surface vertex buffer type
								   // is static. if this is not NULL, the surface persistantAttributes buffer cannot
								   // be NULL.

	float time;		// some value between 0 and 1 corresponding to the lrp weighting between the source and destination
					// vertex buffers.

	int numLights; // ranges from 0 to 4
	//CLightEntity *lights[4];
};

// main XEngine class

class CXEngine
{
public:

	void init();		// responsible for initializing all entity lists, resources lists, audio lists, graphics mode
						// setting up directsound/directmusic, creating the relevant audio paths, initializing the shader
						// system, etc. init process should be nicely logged.

	int update();		// updates the main game loop, processes queued game messages first, gets input, updates entities,
						// then updates collision/culling infos, then updates the cameras and renders, then updates any
						// queued audio events. returns 1 until desired program termination

	void begin();		// performs any final init and enters the main game loop, checks to see if the program needs
						// to quit.

	void shutdown();	// responsible for de-initializing directsound/directmusic/direct3D, releasing memory, whatever.
						// it's not really important but if I decide to port later, it would become important.

	// engine event interface

	void queueEngineEvent(ENGINE_EVENT *engineEvent); //enqueues the game event for handling at the beginning of the next frame

	// entity system interface

	// resource system interface

	void beginLoading();

	void reserveCacheResourceMemory();

	void validatePath(string path); // creates any nessecary directories to make sure the file specified in the path
									// will not fail on creation.

	vector<struct SHADER_INFO*> shaderInfos; // built at runtime, stores the location of every shader availible to the engine
											 // which includes which file it is in, and within that file, where it is.
										     // also holds the shader name.

	// graphics system interface

	LPDIRECT3DDEVICE8 getD3DDevice(); // returns the currently used direct 3D device. I'm not really sure if making
									  // the applications graphics device publically accessible to client code could
									  // be classified as 'sloppy' coding, but at this point, I don't much give a fuck.

	IDirect3DTexture8 *getWhiteTexture(); // this texture is used to fill texture stages in which a texture could not
										  // be found. oddly enough, null texture stages are not valid for pixel shaders
										  // that expect them, in fact, it causes a really fucking nasty crash. thus,
										  // the awesome preloaded 4x4 pixel white texture of fun.

	void queueSurface(XSURFACE_INSTANCE *surface);
	
	class CCameraEntity *getCurrentCamera();

	// debug stuff

	void debugPrint(string debugText);
	void debugRenderText(string debugText, int x, int y);

	// input stuff

	// for any of these input functions XENGINE_INPUT_ALL_PORTS can be passed to port num
	// which will blend the input from all connected controllers for the requested value, otherwise, 0, 1, 2, 3
	// axis is a member of the AXIS_TYPE enumeration, and button is a member of the BUTTON_TYPE enumeration

	void getAxisState(int portNum, int axis, D3DXVECTOR2 *axisState);	// gets a 2D fp vector representing the state of one of the joysticks

	int getButtonDown(int portNum, int button);		// gets whether a certain button is currently 'down' past the deadzone
	int getButtonPressed(int portNum, int button);  // gets whether a certain button is 'pressed', that is, it is down now, but was not last frame
	int getButtonState(int portNum, int button);    // gets the exact analog state (int between 0 and 255) of a button. for the non-analog buttons, just returns 0 or 1
	int getAnyButtonPressed(int portNum);			// if any button is pressed, return true, otherwise, false, doesn't include the triggers

	int isPortConnected(int portNum); // returns whether a given port is connected, if passed XENGINE_INPUT_ALL_PORTS,
									  // this function returns whether any controllers are connected.

	// timing stuff

	void setGameSpeed(float speed);

	int   getFrame();
	int   getRenderFrame();

	float getTween();
	float getTicks();
	float getGameSpeed();
	float getFPS();

	DWORD getMillisecs();

	// audio stuff

	void set3DListener(class CPhysicalEntity *listener);
	CPhysicalEntity *get3DListener();

	IDirectSound8 *getDirectSound();

private:
	
	// main engine stuff

	int terminated;

	// graphics stuff

	void initGraphics();
	void updateGraphics();
	void shutdownGraphics();

	IDirect3DTexture8  *whiteTexture;

	LPDIRECT3D8			direct3D;
	LPDIRECT3DDEVICE8	direct3DDevice;

	IDirect3DSurface8 *backBuffer; // used for debug font rendering

	CCameraEntity *currentCamera;

	XFONT *debugFont;

	// these hold how many surface instances we have to render this frame

	int numOpaqueSurfaces; 
	int numBlendedSurfaces;

	// these hold the references to those surface instances

	XSURFACE_INSTANCE *opaqueSurfaceInstances[XENGINE_GRAPHICS_MAX_OPAQUE_SURFACE_INSTANCES]; // sorted by shader
	XSURFACE_INSTANCE *blendedSurfaceInstances[XENGINE_GRAPHICS_MAX_BLENDED_SURFACE_INSTANCES]; // sorted by distance

	XSURFACE_INSTANCE *skyBoxSurface; // the skybox surface if there is any (otherwise is NULL) is rendered after
									  // all opaque surfaces (for speed) but before all blended surfaces

	CShaderResource *lastShader; // the last shader set, is NULL if the FFP pixel pipe was last used

	// todo: surface instance sorting callbacks

	void renderSurface(XSURFACE_INSTANCE *surface);
	void renderSkyBox(XSURFACE_INSTANCE *surface);

	// timing and tweening stuff

	void initTiming();
	void updateTiming();
	void shutdownTiming();

	int   frame;
	int   renderFrame;

	float ticks;
	float gameSpeed;
	float tween;
	float fps;

	LARGE_INTEGER millisecs;
	LARGE_INTEGER lastMillisecs;

	// input stuff

	void initInput();
	void updateInput();
	void shutdownInput();

	XINPUT_STATE	inputStates[4];
	XBGAMEPAD		gamePad[4];

	// debugging data and methods

	void initDebugging();
	void updateDebugging();
	void shutdownDebugging();

	FILE *debugLog;

	// engine event handling stuff

	void initEngineEvents();
	void updateEngineEvents();
	void shutdownEngineEvents();

	list<ENGINE_EVENT*> engineEventQueue;
	
	// entity stuff

	void initEntities();
	void updateEntities();
	void shutdownEntities();

	// resource stuff

	void initResources();
	void updateResources();
	void shutdownResources();

	// audio stuff

	CPhysicalEntity *listener;

	IDirectSound8 *directSound;

	void initAudio();
	void updateAudio();
	void shutdownAudio();
};

// global XEngine instance accessible to any code

extern CXEngine *XEngine;

#endif