#include "vtrace.h"

vEngine *__engine = NULL;

void __cudaSafeCallNoSync(cudaError error, const char *file, const int line)
{
    if (error != cudaSuccess)
	{
        fprintf(stderr, "cudaSafeCallNoSync() Runtime API error in file <%s>, line %i : %s.\n", file, line, cudaGetErrorString(error));

        //exit(-1);
    }
}

void __cutilCheckError(CUTBoolean error, const char *file, const int line)
{
    if (error != CUTTrue)
	{
        fprintf(stderr, "CUTIL CUDA error in file <%s>, line %i.\n", file, line);

        //exit(-1);
    }
}

void __cutilCheckMsg(const char *errorMessage, const char *file, const int line)
{
    cudaError_t error = cudaGetLastError();

    if (error != cudaSuccess)
	{
        fprintf(stderr, "cutilCheckMsg() CUTIL CUDA error: %s in file <%s>, line %i : %s.\n", errorMessage, file, line, cudaGetErrorString(error));

        //exit(-1);
    }

    error = cudaThreadSynchronize();

    if (error != cudaSuccess)
	{
        fprintf(stderr, "cutilCheckMsg cudaThreadSynchronize error: %s in file <%s>, line %i : %s.\n", errorMessage, file, line, cudaGetErrorString(error));

        //exit(-1);
    }
}

vEngine::vEngine()
{
}

vEngine::~vEngine()
{
	Shutdown();
}

int vEngine::Init(int width, int height, bool vsync, bool windowed, bool tripleBuffering)
{
	if (__engine != NULL)
	{
		return -1;
	}
	else
	{
		lastTicks   = GetTickCount();
		elapsedTime = 0;
		frames      = 0;

		windowWidth  = width;
		windowHeight = height;

		RECT windowRect; SetRect(&windowRect, 0, 0, width, height);
		if (windowed) AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

		const int realWindowWidth  = windowRect.right - windowRect.left;
		const int realWindowHeight = windowRect.bottom - windowRect.top;

		const int windowX = windowed ? (GetSystemMetrics(SM_CXFULLSCREEN) - realWindowWidth) / 2 : 0;
		const int windowY = windowed ? (GetSystemMetrics(SM_CYFULLSCREEN) - realWindowHeight) / 2 : 0;

		WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(0), 0, 0, 0, 0, "VTRACE", 0};
		RegisterClassEx(&wc);

		hWindow = CreateWindow("VTRACE", "VTrace Test", windowed ? WS_OVERLAPPEDWINDOW : (WS_EX_TOPMOST | WS_POPUP), windowX, windowY, realWindowWidth, realWindowHeight, 0, 0, wc.hInstance, 0);

		ShowWindow(hWindow, SW_SHOWDEFAULT);
		UpdateWindow(hWindow);

		ShowCursor(windowed);

#ifndef __DEVICE_EMULATION__

		if (CompositeInit(vsync, windowed, tripleBuffering))
		{
			return -1;
		}

#endif

		if (CoreInit())
		{
			return -1;
		}

		__engine = this;

		if (CreateRenderTarget(&primaryRenderTarget, width, height, VTRACE_RENDER_TARGET_COLOR_BUFFER | VTRACE_RENDER_TARGET_Z_BUFFER))
		{
			return -1;
		}

		return 0;
	}
}

int vEngine::Shutdown()
{
	if (!__engine) return -1;

	if (FreeRenderTarget(&primaryRenderTarget))
	{
		return -1;
	}

	if (CoreShutdown())
	{
		return -1;
	}

#ifndef __DEVICE_EMULATION__

	if (CompositeShutdown())
	{
		return -1;
	}

#endif

	cudaThreadExit();
	cutilCheckMsg("cudaThreadExit failed");

	UnregisterClass("VTRACE", GetModuleHandle(NULL));

	__engine = NULL;

	return 0;
}

int vEngine::CreateRenderTarget(RenderTarget *target, unsigned int width, unsigned int height, unsigned int flags)
{
	target->flags = flags;

#ifndef __DEVICE_EMULATION__

	if (CompositeCreateRenderSurface(&target->assembled, width, height, flags))
	{
		return -1;
	}

#endif

	for (int i = 0; i < VTRACE_NUM_RECONSTRUCTION_LAYERS; i++)
	{
		int layerWidth = width   / (i + 1);
		int layerHeight = height / (i + 1);

		if (CoreCreateRenderSurface(&target->coreSurfaceLevels[i]))
		{
			return -1;
		}

#ifndef __DEVICE_EMULATION__

		if (CompositeCreateRenderSurface(&target->compositeSurfaceLevels[i], layerWidth, layerHeight, flags))
		{
			return -1;
		}

#endif
	}

	return 0;
}

int vEngine::FreeRenderTarget(vEngine::RenderTarget * target)
{
#ifndef __DEVICE_EMULATION__

	if (CompositeFreeRenderSurface(&target->assembled))
	{
		return -1;
	}

#endif

	for (int i = 0; i < VTRACE_NUM_RECONSTRUCTION_LAYERS; i++)
	{
		if (CoreFreeRenderSurface(&target->coreSurfaceLevels[i]))
		{
			return -1;
		}

#ifndef __DEVICE_EMULATION__

		if (CompositeFreeRenderSurface(&target->compositeSurfaceLevels[i]))
		{
			return -1;
		}

#endif
	}

	return 0;
}

int vEngine::BeginScene(RenderTarget * target)
{
	if (target == NULL) target = &primaryRenderTarget;

#ifndef __DEVICE_EMULATION__

	if (CompositeBeginScene(target))
	{
		return -1;
	}

#endif

	if (CoreBeginScene(target))
	{
		return -1;
	}

	return 0;
}

int vEngine::EndScene()
{
	if (CoreEndScene())
	{
		return -1;
	}

#ifndef __DEVICE_EMULATION__

	if (CompositeEndScene())
	{
		return -1;
	}

#endif

	++frames;	// todo: this should get ripped out, it can be a part of the user app

	DWORD ticks(GetTickCount());

	elapsedTime += (ticks - lastTicks);

	if (elapsedTime > 1000)
	{
		char buff[256] = {0};
		sprintf(buff, "VTrace Test - %i\0", frames);

		SetWindowText(hWindow, buff);

		elapsedTime -= 1000;
		frames = 0;
	}

	lastTicks = ticks;

	return 0;
}

bool vEngine::IsActive() const
{
	return __engine != NULL;
}

HWND vEngine::GetWindowHandle() const
{
	return hWindow;
}

LRESULT WINAPI vEngine::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)	// todo: since the setcurrentview functions and pass functions are public this should be part of the app now too
{
    switch(msg)
    {
    case WM_KEYDOWN:

		switch(wParam)
		{
		case VK_ESCAPE:

            PostQuitMessage(0);

			return 0;

		case '0': __engine->SetCurrentView(0); return 0;
		case '1': __engine->SetCurrentView(1); return 0;
		case '2': __engine->SetCurrentView(2); return 0;
		case '3': __engine->SetCurrentView(3); return 0;
		case '4': __engine->SetCurrentView(4); return 0;
		case '5': __engine->SetCurrentView(5); return 0;
		case '6': __engine->SetCurrentView(6); return 0;
		case '7': __engine->SetCurrentView(7); return 0;
		case '8': __engine->SetCurrentView(8); return 0;
		case '9': __engine->SetCurrentView(9); return 0;

		case 'Q': __engine->TogglePass(PASS_AMBIENT_OCCLUSION); return 0;
		case 'W': __engine->TogglePass(PASS_DIRECT_LIGHTING); return 0;
		case 'E': __engine->TogglePass(PASS_ALBEDO_TEXTURE); return 0;
		case 'R': __engine->TogglePass(PASS_AMBIENT_FILTERING); return 0;
		}

		break;

    case WM_DESTROY:

        PostQuitMessage(0);

        return 0;

    case WM_PAINT:

        ValidateRect(hWnd, NULL);

        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}