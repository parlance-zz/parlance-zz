#include <cmath>

#include "vtrace.h"

#define VTEST_GFX_WIDTH		1280
#define VTEST_GFX_HEIGHT	900

#define VTEST_VSYNC			false
#define VTEST_WINDOWED		true
#define VTEST_TRIPLEBUFFER	false

#define VTEST_TIME_SMOOTHING 0.9

double smoothTime = 0.0;
double frameTime  = 1000.0 / 60.0;

__int64 lastTicks;
__int64 tickFrequency;

vEngine::Surface	testSurface;
vEngine::Skin		testSkin;

vEngine engine;

void OurInit()
{
	//engine.LoadSurface(&testSurface, "..\\Data Sets\\Kayla's Models\\rider_body_high.vts");
	//engine.LoadSkin(&testSkin, "..\\Data Sets\\Kayla's Models\\rider_body_high.skn");

	//engine.LoadSurface(&testSurface, "..\\Data Sets\\Kayla's Models\\beast_high.vts");
	//engine.LoadSkin(&testSkin, "..\\Data Sets\\Kayla's Models\\beast_high.skn");

	engine.LoadSurface(&testSurface, "..\\Data Sets\\Misc\\tree.vts");
	engine.LoadSkin(&testSkin, "..\\Data Sets\\Misc\\tree.skn");

	//engine.LoadSurface(&testSurface, "..\\Data Sets\\Kayla's Models\\rider_head_high.vts");
	//engine.LoadSkin(&testSkin, "..\\Data Sets\\Kayla's Models\\rider_head_high.skn");

	//engine.LoadSurface(&testSurface, "..\\Data Sets\\Alien Shark\\alienshark_high.vts");
	//engine.LoadSkin(&testSkin, "..\\Data Sets\\Alien Shark\\alienshark_high.skn");

	vMat4x4 projectionMatrix;
	vMatrixPerspectiveFovLH(&projectionMatrix, 3.14159f / 3.0f, float(VTEST_GFX_WIDTH) / VTEST_GFX_HEIGHT, 1.0f, 100.0f);

	engine.SetProjectionMatrix(&projectionMatrix);

	QueryPerformanceFrequency((LARGE_INTEGER*)&tickFrequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&lastTicks);
}

void OurRender()
{
	__int64 newTicks; QueryPerformanceCounter((LARGE_INTEGER*)&newTicks);

	frameTime   = frameTime * VTEST_TIME_SMOOTHING + ((newTicks - lastTicks) / double(tickFrequency)) * 1000.0 * (1.0 - VTEST_TIME_SMOOTHING);
	smoothTime += frameTime;

	lastTicks = newTicks;

	vVec3 cameraPos(sinf(smoothTime * 0.0005f) * 6.0f, sinf(smoothTime * 0.0002f) * 4.0f, cosf(smoothTime * 0.0002f) * 6.0f);
	//vVec3 cameraPos(sinf(smoothTime * 0.005f) * 8.0f, sinf(smoothTime * 0.002f) * 4.0f, cosf(smoothTime * 0.005f) * 8.0f);

	vVec3 target(0.0f, 0.0f, 0.0f);

	vMat4x4 modelMatrix, viewMatrix;
	vMatrixIdentity(&modelMatrix);
	vMatrixLookAt(&viewMatrix, &cameraPos, &target);

	engine.SetModelMatrix(&modelMatrix);
	engine.SetViewMatrix(&viewMatrix);

	vVec3 up(0.4,-1,0.4), down(-0.4,1,-0.4);
	vVec3 white(1.5,1.25,1.0), dark(0.5f,0.5f, 0.75f);
	vVec3 spec(0.25f, 0.2f, 0.1f);
	vVec3 zero(0,0,0);

	vVec3 p0(20,5,15),p1(-20,10,30);
	vVec3 red(1,0.2f,0.2f),blue(0.2f,0.2f,1);

	engine.LightDirectional(&down, &white, &spec);
	engine.LightDirectional(&up, &dark, &zero);
	//engine.LightPoint(&p0, &dark, &zero);
	//engine.LightPoint(&p1, &dark, &zero);
	
	engine.SetSkin(&testSkin);

	const int testSize = 2;

	srand(0);

	for (int x = -testSize; x <= testSize; x++)
	{
		//for (int y = -testSize; y <= testSize; y++)
		for (int y = 0; y <= 0; y++)
		{
			for (int z = -testSize; z <= testSize; z++)
			{
				//if (!(x==0 && y==0 && z==0))
				{
					//vVec3 translation = vVec3(x, y, z);
					vVec3 translation = vVec3(x + (rand() % 100 - 50) * 0.01f, y, z + (rand() % 100 - 50) * 0.01f);
					vMatrixTranslate(&modelMatrix, &translation);

					vMat4x4 rotationMatrix;
					//vVec3 rotation = vVec3(3.141592635f / 2.0f, 0.0f, 0.0f);
					//vVec3 rotation = vVec3(x + ticks * 0.00025f, y + ticks * 0.00025f, z + ticks * 0.00025f);
					float ang = (rand() % 600) * 0.01f;
					vVec3 rotation = vVec3(0, 0, 0);
					vMatrixPitchYawRoll(&rotationMatrix, &rotation);

					vMat4x4 scaleMatrix;

					vVec3 scale = vVec3(0.8f, 0.8f, 0.8f);

					vMatrixScale(&scaleMatrix, &scale);

					vMatrixMultiply(&modelMatrix, &modelMatrix, &rotationMatrix);
					vMatrixMultiply(&modelMatrix, &modelMatrix, &scaleMatrix);

					engine.SetModelMatrix(&modelMatrix);

					engine.RenderSurface(&testSurface);
				}
			}
		}
	}
}

void OurShutdown()
{
	engine.FreeSkin(&testSkin);
	engine.FreeSurface(&testSurface);
}

int main(int argc, char * argv[])
{
	if (engine.Init(VTEST_GFX_WIDTH, VTEST_GFX_HEIGHT, VTEST_VSYNC, VTEST_WINDOWED, VTEST_TRIPLEBUFFER))
	{
		return -1;
	}

	const HWND hWindow = engine.GetWindowHandle();

	OurInit();

    MSG msg; ZeroMemory(&msg, sizeof(msg));

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else if (!engine.BeginScene())
		{
            OurRender();

			engine.EndScene();
		}
    }

	OurShutdown();

	engine.Shutdown();

	return EXIT_SUCCESS;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR szCmdLine, INT)
{
	return main(1, &szCmdLine);
}