// todo: - remove dependence on model matrix, it can't be used anymore because the basis for animated / deformed surfaces is a lattice, not a matrix
//       - implement dynamic render targets creation / support
//       - implement optimized surface reconstruction shader using push -> pull, do low frequency indirect lighting of generated micro directional lights at low res reconstruction
//         do directional screen space occlusion for high frequency indirect lighting at half res
//       - provide API interface for attaching shadowmaps to lights
//       - implement shadow mapping with soft shadowing filters, in the lighting step add in the low frequency and high frequency indirect contributions
//       - implement hdr postprocessing (tone mapping, bloom, depth of field, motion blur)
//       - provide API interface for mixed (traditional / axel) rendering
//       - implement soft particles rendering for smoke / fog / fire
//       - take a look at the possibility of allowing core to render a layer of transparent surfaces (for water mostly) (meh priority)

#include "vtrace.h"

//#ifdef _DEBUG

#define CheckCgError(function) __CheckCgError(function, __FILE__, __LINE__)
#define AbortCgError(function) __AbortCgError(function, __FILE__, __LINE__)

#define SafeRelease(pointer) if(pointer) {(pointer)->Release(); (pointer) = 0;}
/*
#else

#define CheckCgError(function) false
#define AbortCgError(function) ;

#define SafeRelease(pointer) {(pointer)->Release(); (pointer) = 0;}

#endif*/

inline bool __CheckCgError(const std::string &function, const char *file, const int line)
{
	CGerror error; const char * errorMessage(cgGetLastErrorString(&error));

	if (error == CG_NO_ERROR)
	{
		return false;
	}

	std::cerr << "Error calling " << function << "() at " << file << ":" << line << " - " << errorMessage << std::endl;

	if (error == CG_COMPILER_ERROR)
	{
		std::cerr << cgGetLastListing(__engine->g_context) << std::endl;
	}

	return true;
}

inline void __AbortCgError(const std::string &function, const char *file, const int line)
{
	if (__CheckCgError(function, file, line))
	{
		//exit(-1);
	}
}

void vEngine::LightDirectional(const vVec3 *direction, const vVec3 *diffuse, const vVec3 *specular)
{
	vMat4x4 matModelView;
	vVec3 vsOrigin(0,0,0), vsDirection;

	vMatrixMultiply(&matModelView, &viewMatrix, &modelMatrix);
	vMatrixProjectVec3(&vsOrigin, &matModelView, &vsOrigin);
	vMatrixProjectVec3(&vsDirection, &matModelView, direction);

	AddLight(new DirectionalLight(vsDirection-vsOrigin, *diffuse, *specular));
}

void vEngine::LightPoint(const vVec3 *position, const vVec3 *diffuse, const vVec3 *specular)
{
	vMat4x4 matModelView;
	vVec3	vsPosition;

	vMatrixMultiply(&matModelView, &viewMatrix, &modelMatrix);
	vMatrixProjectVec3(&vsPosition, &matModelView, position);

	AddLight(new PointLight(vsPosition, *diffuse, *specular));
}

int vEngine::CompositeInit(bool vsync, bool windowed, bool tripleBuffering)
{
	view = 1;

	bDeviceLost = false;
	bWindowed   = windowed;

	for(int i = 0; i < NUM_PASSES; i++)
	{
		passEnabled[i] = true;
	}

	pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	if (pD3D == NULL)
	{
		return -1;
	}
   
	unsigned int adapterNum;

	for (adapterNum = 0; adapterNum < pD3D->GetAdapterCount(); adapterNum++)
	{
		D3DADAPTER_IDENTIFIER9 adapterId;
		pD3D->GetAdapterIdentifier(adapterNum, 0, &adapterId);

		int deviceNum;
		cudaD3D9GetDevice(&deviceNum, adapterId.DeviceName);

		if (cudaGetLastError() == cudaSuccess) 
		{
			break;
		}
	}

	if (adapterNum == pD3D->GetAdapterCount())
	{
		printf("Unable to find CUDA->D3D9 capable device\n");

		return -1;
	}

	RECT              rc; GetClientRect(hWindow,&rc);
	D3DDISPLAYMODE d3ddm; pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	D3DPRESENT_PARAMETERS d3dpp; ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.Windowed               = bWindowed;
	d3dpp.BackBufferCount        = 1 + tripleBuffering;
	d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow          = hWindow;
	d3dpp.BackBufferWidth	     = rc.right  - rc.left;
	d3dpp.BackBufferHeight       = rc.bottom - rc.top;
	d3dpp.BackBufferFormat       = d3ddm.Format;

	if (!vsync)
	{
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	}

	if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWindow, D3DCREATE_PUREDEVICE | D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &pD3DDevice)))
	{
		return -1;
	}

	pD3DDevice->GetRenderTarget(0, &frameBuffer);

	cudaD3D9SetDirect3DDevice(pD3DDevice);
	cutilCheckMsg("cudaD3D9SetDirect3DDevice failed");

	g_context = context = cgCreateContext();
	AbortCgError("cgCreateContext");

	cgD3D9SetDevice(pD3DDevice);
	AbortCgError("cgD3D9SetDevice");

	cgD3D9RegisterStates(context);
	AbortCgError("cgD3D9RegisterStates");

	cgD3D9SetManageTextureParameters(context, CG_TRUE);
	AbortCgError("cgD3D9SetManageTextureParameters");

	if (FAILED(InitDeviceObjects()))
	{
		return -1;
	}

	return 0;
}

int vEngine::CompositeShutdown()
{
	for (EffectMap::iterator it(effects.begin()); it!=effects.end(); ++it)
	{
		CGeffect effect(it->second);

		if (effect)
		{
			cgDestroyEffect(effect);
		}
	}

	cgD3D9SetDevice(0);
	cgDestroyContext(context);

	DestroyDeviceObjects();

	SafeRelease(pD3DDevice);
	SafeRelease(pD3D);

	return 0;
}

int vEngine::CompositeCreateRenderSurface(CompositeRenderSurface * renderSurface, unsigned int width, unsigned int height, unsigned int flags)
{
	// TODO: Create the composite render surface

	return -1;
}

int vEngine::CompositeFreeRenderSurface(CompositeRenderSurface * renderSurface)
{
	// TODO: Free the composite render surface

	return -1;
}

HRESULT vEngine::InitDeviceObjects()
{
	if(FAILED(D3DXCreateTextureFromFile(pD3DDevice, "Data/Textures/noise.png", &texNoiseNormals))) return E_FAIL;

	// TODO: Allocate buffers
	/*if(FAILED(pD3DDevice->CreateTexture(windowWidth, windowHeight, 1, 0, 
										D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texAlbedo, 0))) return E_FAIL;
	if(FAILED(pD3DDevice->CreateTexture(windowWidth, windowHeight, 1, 0, 
										D3DFMT_R32F, D3DPOOL_DEFAULT, &texDepth, 0))) return E_FAIL;*/

	if(FAILED(pD3DDevice->CreateTexture(windowWidth/2, windowHeight/2, 1, D3DUSAGE_RENDERTARGET, 
										D3DFMT_R16F, D3DPOOL_DEFAULT, &texSpareIntensity, 0))) return E_FAIL;
	if(FAILED(pD3DDevice->CreateTexture(windowWidth, windowHeight, 1, D3DUSAGE_RENDERTARGET, 
										D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &texNormalDepth, 0))) return E_FAIL;
	if(FAILED(pD3DDevice->CreateTexture(windowWidth/2, windowHeight/2, 1, D3DUSAGE_RENDERTARGET, 
										D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &texNormalDepth4, 0))) return E_FAIL;
	if(FAILED(pD3DDevice->CreateTexture(windowWidth/2, windowHeight/2, 1, D3DUSAGE_RENDERTARGET, 
										D3DFMT_R16F, D3DPOOL_DEFAULT, &texAmbientOcclusion, 0))) return E_FAIL;
	if(FAILED(pD3DDevice->CreateTexture(windowWidth, windowHeight, 1, D3DUSAGE_RENDERTARGET, 
										D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &texLight, 0))) return E_FAIL;

	views.push_back(texLight);
	//views.push_back(texAlbedo);
	views.push_back(texNormalDepth);
	views.push_back(texNormalDepth4);
	views.push_back(texAmbientOcclusion);

	for (unsigned int i = 0; i < views.size(); i++)
	{
		views[i]->AddRef();
	}

	if (!bDeviceLost) 
	{
		// TODO: Register buffers with CUDA
		/*cudaD3D9RegisterResource(texAlbedo, cudaD3D9RegisterFlagsNone);
		cutilCheckMsg("cudaD3D9RegisterResource (g_texture_2d) failed");

		cudaD3D9ResourceSetMapFlags(texAlbedo, cudaD3D9MapFlagsWriteDiscard);
		cutilCheckMsg("cudaD3D9ResourceSetMapFlags (g_texture_2d) failed");

		cudaD3D9RegisterResource(texDepth, cudaD3D9RegisterFlagsNone);
		cutilCheckMsg("cudaD3D9RegisterResource (g_texture_2d) failed");

		cudaD3D9ResourceSetMapFlags(texDepth, cudaD3D9MapFlagsWriteDiscard);
		cutilCheckMsg("cudaD3D9ResourceSetMapFlags (g_texture_2d) failed");*/
	}

	return S_OK;
}

HRESULT vEngine::DestroyDeviceObjects()
{
	// TODO: Unregister buffers with CUDA
	/*cudaD3D9UnregisterResource(texAlbedo);
	cutilCheckMsg("cudaD3D9UnregisterResource (g_texture_2d) failed");

	cudaD3D9UnregisterResource(texDepth);
	cutilCheckMsg("cudaD3D9UnregisterResource (g_texture_2d) failed");*/

	for (unsigned int i = 0; i < views.size(); i++)
	{
		SafeRelease(views[i]);
	}

	views.clear();

	SafeRelease(texNoiseNormals);
	// TODO: Release buffers
	//SafeRelease(texAlbedo);
	//SafeRelease(texDepth);
	SafeRelease(texSpareIntensity);
	SafeRelease(texNormalDepth);
	SafeRelease(texNormalDepth4);
	SafeRelease(texAmbientOcclusion);
	SafeRelease(texLight);

	return S_OK;
}

HRESULT vEngine::DeviceLostHandler()
{
	HRESULT hr;

	fprintf(stderr, "-> Starting DeviceLostHandler() \n");

	if (FAILED(hr = pD3DDevice->TestCooperativeLevel()))
	{
		fprintf(stderr, "TestCooperativeLevel = %08x failed, will attempt to reset\n", hr);

		if (hr == D3DERR_DEVICELOST) 
		{
			fprintf(stderr, "TestCooperativeLevel = %08x DeviceLost, will retry next call\n", hr);

			return S_OK;
		}

		if (hr == D3DERR_DEVICENOTRESET)
		{
			fprintf(stderr, "TestCooperativeLevel = %08x will try to RESET the device\n", hr);

			if (bWindowed)
			{
				pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

				d3dpp.BackBufferFormat = d3ddm.Format;
			}

			if (FAILED(hr = pD3DDevice->Reset(&d3dpp))) 
			{
				fprintf(stderr, "TestCooperativeLevel = %08x RESET device FAILED!\n", hr);

				return hr;
			} 
			else 
			{
				fprintf(stderr, "TestCooperativeLevel = %08x RESET device SUCCESS!\n", hr);

				cudaD3D9SetDirect3DDevice(pD3DDevice);
				cutilCheckMsg("cudaD3D9SetDirect3DDevice failed");

				InitDeviceObjects();

				bDeviceLost = false;
			}
		}
	}

	return hr;
}

CGparameter vEngine::GetUserParam(const std::string &typeName)
{
	ParameterList &params(userParams[typeName]);
	size_t &used(userParamsUsed[typeName]);

	if (used == params.size())
	{
		CGtype type(cgGetNamedUserType(effect, typeName.c_str()));

		params.push_back(cgCreateParameter(context, type));
	}

	return params[used++];
}

void vEngine::ResetUserParams()
{
	for(std::map<std::string,size_t>::iterator it(userParamsUsed.begin()), end(userParamsUsed.end()); it!=end; ++it)
	{
		it->second = 0;
	}
}

void vEngine::SetEffect(const std::string &name)
{
	static const std::string shaderPath("Data/Effects/");

	effectName = name;

	EffectMap::const_iterator it(effects.find(name));

	if (it == effects.end())
	{
		CGeffect effect(cgCreateEffectFromFile(context, (shaderPath + name + ".cgfx").c_str(), 0));

		CheckCgError("cgCreateEffectFromFile(\"" + name + "\")");

		effects.insert(std::make_pair(name, effect));
		this->effect = effect;
	}
	else
	{
		this->effect = it->second;
	}
}

void vEngine::RemoveEffect()
{
	std::cerr << "Warning: Removing effect \"" << effectName << "\" due to Cg error." << std::endl;

	effects[effectName] = 0;
}

CGparameter vEngine::GetParam(const std::string &name)
{
	if (!effect) return 0;

	CGparameter param(cgGetNamedEffectParameter(effect, name.c_str()));

	if (CheckCgError("cgGetNamedEffectParameter(" + effectName + ", " + name + ")"))
	{
		RemoveEffect();

		return 0;
	}
	else if(!param)
	{
		std::cerr << "No parameter \"" + name + "\" in effect \"" + effectName+"\"" << std::endl;

		RemoveEffect();

		return 0;
	}

	return param;
}

void vEngine::SetParam(const std::string &name, float x)
{
	CGparameter param(GetParam(name)); 
	if (!param) return;

	cgSetParameter1f(param, x);

	if (CheckCgError("cgSetParameter1f")) RemoveEffect();
}

void vEngine::SetParam(const std::string &name, float x, float y)
{
	CGparameter param(GetParam(name)); 
	if (!param) return;

	cgSetParameter2f(param, x, y);

	if (CheckCgError("cgSetParameter2f")) RemoveEffect();
}

void vEngine::SetParam(const std::string &name, float x, float y, float z)
{
	CGparameter param(GetParam(name)); 
	if (!param) return;

	cgSetParameter3f(param, x, y, z);

	if (CheckCgError("cgSetParameter3f")) RemoveEffect();
}

void vEngine::SetParam(const std::string &name, float x, float y, float z, float w)
{
	CGparameter param(GetParam(name)); 
	if (!param) return;

	cgSetParameter4f(param, x, y, z, w);

	if (CheckCgError("cgSetParameter4f")) RemoveEffect();
}

void vEngine::SetParam(const std::string &name, bool v)
{
	SetParam(name, v ? 1.0f : 0.0f);
}

void vEngine::SetParam(const std::string &name, IDirect3DTexture9 *tex)
{
	CGparameter param(GetParam(name)); 
	if (!param) return;

	cgD3D9SetTextureParameter(param, tex);

	if (CheckCgError("cgD3D9SetTextureParameter")) RemoveEffect();
}

void vEngine::RenderPass(const std::string &passName, IDirect3DTexture9 *target0, IDirect3DTexture9 *target1)
{
	struct VertexStruct
	{
		float position[3];
		float texture[2];
	};

	int width  = windowWidth;
	int height = windowHeight;

	if (target0)
	{
		D3DSURFACE_DESC dest; target0->GetLevelDesc(0, &dest);

		width  = dest.Width;
		height = dest.Height;
	}

	const float biasS = 0.5f / width;
	const float biasT = 0.5f / height;
	const float s0 = biasS, s1 = biasS + 1.0f;
	const float t0 = biasT, t1 = biasT + 1.0f;

	const VertexStruct vertices[4] = 
	{
		{  {-1,-1,0}, {s0,t0} },
		{  {-1, 1,0}, {s0,t1} },
		{  { 1, 1,0}, {s1,t1} },
		{  { 1,-1,0}, {s1,t0} }
	};

	const VertexStruct flipVertices[4] = 
	{
		{  {-1,-1,0}, {s0,t1} },
		{  {-1, 1,0}, {s0,t0} },
		{  { 1, 1,0}, {s1,t0} },
		{  { 1,-1,0}, {s1,t1} }
	};

	CGtechnique technique(cgGetFirstTechnique(effect));

	while (technique && cgValidateTechnique(technique) == CG_FALSE) 
	{
		technique = cgGetNextTechnique(technique);
	}

	if (technique)
	{
		CGpass pass = cgGetNamedPass(technique, passName.c_str());

		if (pass)
		{
			IDirect3DSurface9 *surface0 = frameBuffer;
			if (target0) target0->GetSurfaceLevel(0, &surface0);

			pD3DDevice->SetRenderTarget(0, surface0);

			if (target1)
			{
				IDirect3DSurface9 *surface1; target1->GetSurfaceLevel(0, &surface1);

				pD3DDevice->SetRenderTarget(1, surface1);
			}

			pD3DDevice->BeginScene();

			cgSetPassState(pass);

			pD3DDevice->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
			pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, target0 ? flipVertices : vertices, sizeof(VertexStruct));

			cgResetPassState(pass);

			pD3DDevice->EndScene();

			if (target1)
			{
				pD3DDevice->SetRenderTarget(1, NULL);
			}
		}
	}		
}

void vEngine::DownSample2x2(IDirect3DTexture9 *texSource, const std::string &pass, IDirect3DTexture9 *texDest)
{
	D3DSURFACE_DESC dest; texDest->GetLevelDesc(0, &dest);

	float texelWidth	= 1.0f / dest.Width;	// How many source texels per dest texel
	float texelHeight	= 1.0f / dest.Height;	// How many source texels per dest texel
	float sampleWidth	= texelWidth / 2.0f;	// How many source texels per dest sample
	float sampleHeight	= texelHeight / 2.0f;	// How many source texels per dest sample

	float offsets[4][2];

	for (int t = 0; t < 2; t++)
	{
		for (int s = 0; s < 2; s++)
		{
			offsets[t * 2 + s][0] = (s * 1.0f - 0.5f) * sampleWidth;
			offsets[t * 2 + s][1] = (t * 1.0f - 0.5f) * sampleHeight;
		}
	}

	SetEffect("filter");
	if (!effect) return;

	SetParam("TexSource", texSource);

	CGparameter param = cgGetNamedEffectParameter(effect, "SampleOffsets2x2");
	cgSetParameterValuefr(param, 8, offsets[0]);

	if(CheckCgError("cgSetParameterValuefr"))
	{
		RemoveEffect();

		return;
	}

	RenderPass(pass, texDest);
}

void vEngine::DownSample4x4(IDirect3DTexture9 *texSource, const std::string &pass, IDirect3DTexture9 *texDest)
{
	D3DSURFACE_DESC dest; texDest->GetLevelDesc(0, &dest);

	float texelWidth	= 1.0f / dest.Width;	// How many source texels per dest texel
	float texelHeight	= 1.0f / dest.Height;	// How many source texels per dest texel
	float sampleWidth	= texelWidth / 4.0f;	// How many source texels per dest sample
	float sampleHeight	= texelHeight / 4.0f;	// How many source texels per dest sample

	float offsets[16][2];

	for (int t = 0; t < 4; t++)
	{
		for (int s = 0; s < 4; s++)
		{
			offsets[t * 4 + s][0] = (s * 1.0f - 1.5f) * sampleWidth;
			offsets[t * 4 + s][1] = (t * 1.0f - 1.5f) * sampleHeight;
		}
	}

	SetEffect("filter");
	if (!effect) return;

	SetParam("TexSource", texSource);

	CGparameter param = cgGetNamedEffectParameter(effect, "SampleOffsets4x4");
	cgSetParameterValuefr(param, 32, offsets[0]);

	if (CheckCgError("cgSetParameterValuefr"))
	{
		RemoveEffect();

		return;
	}

	RenderPass(pass, texDest);
}

void vEngine::RenderSSN()
{
	SetEffect("ssn");
	if (!effect) return;

	// TODO: Fix screenspace normals pass
	//SetParam("TexDepth",        texDepth);
	//SetParam("TexAlbedo",       texAlbedo);
	SetParam("TexelSize",       texelSize[0], texelSize[1]);
	SetParam("FrustumExtents",  frustumExtents[0], frustumExtents[1]);
	SetParam("InverseProjectA", 1.0f / projectionMatrix.i22);
	SetParam("ProjectB",        projectionMatrix.i32);
	
	RenderPass("NormalDepth", texNormalDepth, NULL);
}

void vEngine::RenderSSAO()
{
	if (!passEnabled[PASS_AMBIENT_OCCLUSION]) return;

	DownSample2x2(texNormalDepth, "TapNormal2x2", texNormalDepth4);

	const float depthScaling = 0.1f;

	SetEffect("ssao");
	if (!effect) return;

	SetParam("TexNormalDepth4",	texNormalDepth4);
	SetParam("TexNoiseNormals", texNoiseNormals);
	SetParam("FrustumExtents",	frustumExtents[0], frustumExtents[1]);
	SetParam("NoiseScale",		windowWidth / 64.0f, windowHeight / 64.0f);
	SetParam("DepthScaling",	depthScaling);

	RenderPass("SSAO", texAmbientOcclusion);

	SetEffect("filter");
	if (!effect) return;

	SetParam("TexNormalDepth",	texNormalDepth);
	SetParam("CbDepthScaling",	depthScaling);
	SetParam("TexSource",		texAmbientOcclusion);
	SetParam("CbSampleOffset",	1.0f / windowWidth, 0.0f);

	RenderPass("CrossBilateral", texSpareIntensity);

	SetParam("TexSource",		texSpareIntensity);
	SetParam("CbSampleOffset",	0.0f, 1.0f / windowHeight);

	RenderPass("CrossBilateral", texAmbientOcclusion);
}

void vEngine::RenderLight()
{
	SetEffect("light");
	if (!effect) return;

	// TODO: Fix lighting pass
	SetParam("TexNormalDepth",		   texNormalDepth);
	//SetParam("TexAlbedo",			   texAlbedo);
	SetParam("TexelSize",              texelSize[0], texelSize[1]);
	SetParam("FrustumExtents",		   frustumExtents[0], frustumExtents[1]);
	SetParam("EnableAmbientOcclusion", passEnabled[PASS_AMBIENT_OCCLUSION]);

	if (passEnabled[PASS_AMBIENT_OCCLUSION])
	{
		D3DSURFACE_DESC desc; texAmbientOcclusion->GetLevelDesc(0, &desc);

		SetParam("TexelSize4",           1.0f / desc.Width, 1.0f / desc.Height);
		SetParam("TexNormalDepth4",      texNormalDepth4);
		SetParam("TexAmbientOcclusion4", texAmbientOcclusion);
	}

	SetParam("EnableDirectLighting",	passEnabled[PASS_DIRECT_LIGHTING]);
	SetParam("EnableAlbedoTexture",		passEnabled[PASS_ALBEDO_TEXTURE]);
	SetParam("EnableAmbientFiltering",	passEnabled[PASS_AMBIENT_FILTERING]);

	CGparameter param(cgGetNamedEffectParameter(effect, "Lights"));

	static bool first(true); // TODO: Fix to allow dynamic numbers of light sources

	if (first) cgSetArraySize(param, lights.size());

	if (cgGetError() == CG_NO_ERROR)
	{
		for (unsigned int i = 0; i < lights.size(); i++)
		{
			CGparameter light(GetUserParam(lights[i]->GetTypename()));
			lights[i]->SetParameters(light);

			if (first) cgConnectParameter(light, cgGetArrayParameter(param, i));
		}
	}

	first = false; // TODO: Fix to allow dynamic numbers of light sources

	RenderPass("Light", texLight);
	CheckCgError("RenderPass(texLight)");

	/*
	for (int i = 0; i < lights.size(); i++)
	{
		cgDisconnectParameter(cgGetArrayParameter(param, i));
	}
	*/

	ResetUserParams();
}

int vEngine::CompositeBeginScene(vEngine::RenderTarget * target)
{
	// TODO: Use target

	for (unsigned int i = 0; i < lights.size(); i++)
	{
		delete lights[i];
	}

	lights.clear();

	if (!bDeviceLost) 
	{
		// TODO: Fix mapping phase
		/*IDirect3DResource9 * ppResources[2] = {texAlbedo, texDepth};
		cudaD3D9MapResources(2, ppResources);

		cutilCheckMsg("cudaD3D9MapResources(2) failed");

		cutilSafeCallNoSync(cudaD3D9ResourceGetMappedPointer((void**)&sceneConstants.compositeFBuffer[0], texAlbedo, 0, 0));
		cutilSafeCallNoSync(cudaD3D9ResourceGetMappedPointer((void**)&sceneConstants.compositeZBuffer[0], texDepth, 0, 0));*/

		return 0;
	}
	else
	{
		return -1;
	}
}

int vEngine::CompositeEndScene()
{
	// TODO: Fix unmapping phase
	/*IDirect3DResource9 * ppResources[2] = {texAlbedo, texDepth};
	cudaD3D9UnmapResources(2, ppResources);

	cutilCheckMsg("cudaD3D9UnmapResources(2) failed");*/

	if (bDeviceLost) 
	{
		if (FAILED(DeviceLostHandler()))
		{
			fprintf(stderr, "DeviceLostHandler FAILED!\n");

			return -1;
		}
	}

	if (!bDeviceLost) 
	{
		texelSize[0] = 1.0f / windowWidth;
		texelSize[1] = 1.0f / windowHeight;

		frustumExtents[0] = 1.0f / projectionMatrix.i00;
		frustumExtents[1] = 1.0f / projectionMatrix.i11;
		
		if (view == 0)
		{
			SetEffect("filter");

			if (effect)
			{
				// TODO: Fix pass-through renderer
				// SetParam("TexSource", texAlbedo);
				RenderPass("Tap");

				CheckCgError("RenderPass(\"Tap\")");
			}
		}
		else
		{
			RenderSSN();
			RenderSSAO();
			RenderLight();

			SetEffect("filter");

			if (effect)
			{
				SetParam("TexSource", views[view - 1]);
				RenderPass("Tap");

				CheckCgError("RenderPass(\"Tap\")");
			}
		}

		if (pD3DDevice->Present(0, 0, 0, 0) == D3DERR_DEVICELOST) 
		{
			bDeviceLost = true;

			fprintf(stderr, "DrawScene Present = Detected D3D DeviceLost\n");
			
			DestroyDeviceObjects();

			return -1;
		}
	}

	return 0;
}