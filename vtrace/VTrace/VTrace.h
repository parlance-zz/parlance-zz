#ifndef _VTRACE_H_
#define _VTRACE_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <iostream>

#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_d3d9_interop.h>

//#include <cudpp\cudpp.h>

#include <cg\cgd3d9.h>

#include <d3dx9.h>

#define VTRACE_NUM_RECONSTRUCTION_LAYERS	4

#include "vtrace_math.h"
#include "vtrace_kernels.h"

// *****************************************

//#define __DEVICE_EMULATION__
//#ifdef _DEBUG

#define cutilSafeCallNoSync(err)     __cudaSafeCallNoSync(err, __FILE__, __LINE__)
#define cutilCheckError(err)         __cutilCheckError   (err, __FILE__, __LINE__)
#define cutilCheckMsg(msg)           __cutilCheckMsg     (msg, __FILE__, __LINE__)
/*
#else

#define cutilSafeCallNoSync(err)	;
#define cutilCheckError(err)		;
#define cutilCheckMsg(msg)			;

#endif*/

enum CUTBoolean 
{
    CUTFalse = 0,
    CUTTrue = 1
};

void __cudaSafeCallNoSync(cudaError error, const char *file, const int line);
void __cutilCheckError(CUTBoolean error, const char *file, const int line);
void __cutilCheckMsg(const char *errorMessage, const char *file, const int line);

// *****************************************

extern class vEngine *__engine;

class vEngine
{
public:

#include "vtrace_core_public.h"
#include "vtrace_composite_public.h"

	enum RenderTargetFlags
	{
		VTRACE_RENDER_TARGET_COLOR_BUFFER = 0x01,
		VTRACE_RENDER_TARGET_Z_BUFFER     = 0x02
	};

	struct RenderTarget
	{
		CoreRenderSurface      coreSurfaceLevels[VTRACE_NUM_RECONSTRUCTION_LAYERS];
		CompositeRenderSurface compositeSurfaceLevels[VTRACE_NUM_RECONSTRUCTION_LAYERS];	// these are the intermediate textures used in reconstruction which are mapped to respective c_xBuffers

		CompositeRenderSurface assembled;												// the reconstruction shader will use all the textures from compositesurfacelevels to generate this, the final reconstructed image buffers

		unsigned int flags;
	};

	vEngine();
	~vEngine();

	int Init(int width, int height, bool vsync, bool windowed, bool tripleBuffer);
	int Shutdown();

	int CreateRenderTarget(RenderTarget *target, unsigned int width, unsigned int height, unsigned int flags);
	int FreeRenderTarget(RenderTarget *target);

	int BeginScene(RenderTarget *target = NULL);
	int EndScene();
	
	bool IsActive() const;
	HWND GetWindowHandle() const;

private:

	RenderTarget primaryRenderTarget;

	HWND		    hWindow;
	unsigned int	windowWidth;
	unsigned int	windowHeight;

	DWORD			lastTicks;
	DWORD			elapsedTime;
	int				frames;

	static LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "vtrace_core_private.h"
#include "vtrace_composite_private.h"
};

#endif