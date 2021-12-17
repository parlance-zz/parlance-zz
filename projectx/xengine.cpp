#include <xtl.h>
#include <dsound.h>
#include <dsstdfx.h>
#include <xgraphics.h>

#include "xengine.h"

#include "entity.h"
#include "physical_entity.h"
#include "camera_entity.h"
#include "sound_entity.h"
#include "sound3d_entity.h"
#include "mesh_entity.h"
#include "entity_types.h"
#include "resource.h"
#include "shader_resource.h"
#include "texture_resource.h"
#include "vertexbuffer_resource.h"
#include "indexbuffer_resource.h"
#include "bsp_resource.h"
#include "music_resource.h"
#include "sound_resource.h"
#include "cache_resource.h"
#include "md3_resource.h"
#include "resource_types.h"
#include "local_player.h"
#include "player_resource.h"
#include "weapon.h"
#include "weapon_resource.h"

#include "math_help.h"
#include "string_help.h"

// instantiate global XEngine instance

CXEngine *XEngine = new CXEngine;

// ********* main engine init, update, shutdown ********* 

void CXEngine::init()
{
	terminated = false;

	// todo: may cause precision problems

	_controlfp(_PC_24, _MCW_PC);

	initDebugging();
	initGraphics();
	initInput();
	initAudio();
	initTiming();
	initEngineEvents();
	initResources();
	initEntities();
}

void CXEngine::begin()
{
	//perform any final initialization here
	//like, load initial bsp or something

	beginLoading();

	CBSPResource *bsp = new CBSPResource;

	bsp->create("maps\\test.bsp");

	CLocalPlayer *localPlayer1 = new CLocalPlayer;
	localPlayer1->create(CPlayerResource::getPlayer("models\\players\\crash"), 0);

	D3DXVECTOR3 playerPos = D3DXVECTOR3(-200.0f, 0.0f, 0.0f);
	localPlayer1->setPosition(&playerPos);

	CLocalPlayer *localPlayer2 = new CLocalPlayer;
	localPlayer2->create(CPlayerResource::getPlayer("models\\players\\doom"), 1);

	playerPos = D3DXVECTOR3(-300.0f, 0.0f, 0.0f);
	localPlayer2->setPosition(&playerPos);

	CWeaponResource *machineGunResource = CWeaponResource::getWeapon("models\\weapons2\\gauntlet\\gauntlet");
	CWeaponResource *plasmaCannonResource = CWeaponResource::getWeapon("models\\weapons2\\gauntlet\\gauntlet");

	CWeapon *plasmaCannon1 = new CWeapon;
	plasmaCannon1->create(plasmaCannonResource);
	localPlayer1->giveWeapon(plasmaCannon1);

	CWeapon *machineGun1 = new CWeapon;
	machineGun1->create(machineGunResource);
	localPlayer2->giveWeapon(machineGun1);

	set3DListener(localPlayer1);

	CMusicResource *musicResource = new CMusicResource;

	musicResource->create("music\\test.wma");
	musicResource->setPaused(false);

	while (update())
	{
	}
}

int CXEngine::update()
{
	updateDebugging();
	updateTiming();
	updateInput();
	updateResources();
	updateAudio();
	updateEngineEvents();

	if (terminated)
	{
		return 0;
	}
	else
	{
		updateEntities();
		updateGraphics();
	}

	return 1;
}

void CXEngine::shutdown()
{
	debugPrint("XEngine - Beginning shutdown process...\n");

	shutdownEntities();
	shutdownResources();
	shutdownEngineEvents();
	shutdownTiming();
	shutdownAudio();
	shutdownInput();
	shutdownGraphics();
	shutdownDebugging();
}

// ********* graphics ********* 

IDirect3DTexture8 *CXEngine::getWhiteTexture()
{
	return whiteTexture;
}

void CXEngine::initGraphics()
{
	direct3D = Direct3DCreate8(D3D_SDK_VERSION);

	direct3D->SetPushBufferSize(XENGINE_GRAPHICS_PUSHBUFFER_SIZE, XENGINE_GRAPHICS_KICKOFF_SIZE);

	D3DPRESENT_PARAMETERS d3dpp; 
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.BackBufferWidth = 640;
	d3dpp.BackBufferHeight = 480;
	d3dpp.BackBufferFormat = d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.BackBufferCount = 1;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

	d3dpp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES_MULTISAMPLE_LINEAR; // uncomment to activate multisampling
    //                        D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_QUINCUNX;
							//D3DMULTISAMPLE_4_SAMPLES_SUPERSAMPLE_GAUSSIAN;

	//if I uncommented this, vsync would be disabled and I'd get some high refresh rates and smooth animation, but it makes
	//tweening more diffcult because of sub-frame precision issues when the frame rate can go anywhere from ~200 fps to ~20.
	//I think I'll just leave vsync on.

	//d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE_OR_IMMEDIATE;
	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	direct3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &direct3DDevice);

	direct3DDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);

	//todo: some logging of caps or whatever might be appropriate here

	// init default render states that represent the base states that shaders must record commands to get 'into'
	// their preferred state, and then back to this state. this might need some adjusting, in addition, perhaps
	// this 'state' needs to be recorded in a decent way.

	direct3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	direct3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	direct3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	//direct3DDevice->SetRenderState(D3DRS_LOCALVIEWER, false); // todo: will this fix the env mapping?

	direct3DDevice->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTEXF_LINEAR);
	direct3DDevice->SetTextureStageState(0,D3DTSS_MINFILTER,D3DTEXF_LINEAR);
	direct3DDevice->SetTextureStageState(0,D3DTSS_MIPFILTER,D3DTEXF_LINEAR);
	direct3DDevice->SetTextureStageState(0,D3DTSS_ADDRESSU,D3DTADDRESS_WRAP);
	direct3DDevice->SetTextureStageState(0,D3DTSS_ADDRESSV,D3DTADDRESS_WRAP);
	direct3DDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
	direct3DDevice->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);

	direct3DDevice->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
	direct3DDevice->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);

	direct3DDevice->SetTextureStageState(1,D3DTSS_MAGFILTER,D3DTEXF_LINEAR);
	direct3DDevice->SetTextureStageState(1,D3DTSS_MINFILTER,D3DTEXF_LINEAR);
	direct3DDevice->SetTextureStageState(1,D3DTSS_MIPFILTER,D3DTEXF_POINT);
	direct3DDevice->SetTextureStageState(1,D3DTSS_ADDRESSU,D3DTADDRESS_WRAP);
	direct3DDevice->SetTextureStageState(1,D3DTSS_ADDRESSV,D3DTADDRESS_WRAP);
	direct3DDevice->SetTextureStageState(1,D3DTSS_COLOROP, D3DTOP_MODULATE);
	direct3DDevice->SetTextureStageState(1,D3DTSS_COLORARG1, D3DTA_TEXTURE);

	direct3DDevice->SetTextureStageState(2,D3DTSS_MAGFILTER,D3DTEXF_LINEAR);
	direct3DDevice->SetTextureStageState(2,D3DTSS_MINFILTER,D3DTEXF_LINEAR);
	direct3DDevice->SetTextureStageState(2,D3DTSS_MIPFILTER,D3DTEXF_POINT);
	direct3DDevice->SetTextureStageState(2,D3DTSS_ADDRESSU,D3DTADDRESS_WRAP);
	direct3DDevice->SetTextureStageState(2,D3DTSS_ADDRESSV,D3DTADDRESS_WRAP);
	direct3DDevice->SetTextureStageState(2,D3DTSS_COLOROP, D3DTOP_DISABLE);
	direct3DDevice->SetTextureStageState(2,D3DTSS_COLORARG1, D3DTA_TEXTURE);

	direct3DDevice->SetTextureStageState(3,D3DTSS_MAGFILTER,D3DTEXF_LINEAR);
	direct3DDevice->SetTextureStageState(3,D3DTSS_MINFILTER,D3DTEXF_LINEAR);
	direct3DDevice->SetTextureStageState(3,D3DTSS_MIPFILTER,D3DTEXF_POINT);
	direct3DDevice->SetTextureStageState(3,D3DTSS_ADDRESSU,D3DTADDRESS_WRAP);
	direct3DDevice->SetTextureStageState(3,D3DTSS_ADDRESSV,D3DTADDRESS_WRAP);
	direct3DDevice->SetTextureStageState(3,D3DTSS_COLOROP, D3DTOP_DISABLE);
	direct3DDevice->SetTextureStageState(3,D3DTSS_COLORARG1, D3DTA_TEXTURE);

	direct3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	direct3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	direct3DDevice->SetRenderState(D3DRS_ALPHAREF, 0x00000000);
	direct3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	direct3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	lastShader = NULL;

	// load up the debug font

	XFONT_OpenBitmapFont(L"D:\\debugfont.bmf", 5 * 1024, &debugFont);

	// create the white texture

	direct3DDevice->CreateTexture(4, 4, 1, NULL, D3DFMT_X8R8G8B8, NULL, &whiteTexture);

	DWORD *buff = new DWORD[4 * 4];
	memset(buff, 0xFFFFFFFF, sizeof(DWORD) * 4 * 4);

	D3DLOCKED_RECT lockedRect = {0};

	whiteTexture->LockRect(0, &lockedRect, NULL, NULL);

	XGSwizzleRect(buff, 0, NULL, lockedRect.pBits, 4, 4, NULL, sizeof(DWORD));

	whiteTexture->UnlockRect(0);

	delete [] buff;
}

LPDIRECT3DDEVICE8 CXEngine::getD3DDevice()
{
	return direct3DDevice;
}

CCameraEntity *CXEngine::getCurrentCamera()
{
	return currentCamera;
}

void CXEngine::queueSurface(XSURFACE_INSTANCE *surface)
{
	if (surface->surface->shader)
	{
		switch (surface->surface->shader->getShaderType())
		{
		case ST_OPAQUE:

			if (numOpaqueSurfaces < XENGINE_GRAPHICS_MAX_OPAQUE_SURFACE_INSTANCES)
			{
				opaqueSurfaceInstances[numOpaqueSurfaces] = surface;
				numOpaqueSurfaces++;
			}

			break;

		case ST_SKYBOX:

			if (!skyBoxSurface)
			{
				skyBoxSurface = surface;
			}

			break;

		case ST_BLENDED:

			if (numBlendedSurfaces < XENGINE_GRAPHICS_MAX_BLENDED_SURFACE_INSTANCES)
			{
				blendedSurfaceInstances[numBlendedSurfaces] = surface;
				numBlendedSurfaces++;
			}

			break;
		}
	}
	else
	{
		if (numOpaqueSurfaces < XENGINE_GRAPHICS_MAX_OPAQUE_SURFACE_INSTANCES)
		{
			opaqueSurfaceInstances[numOpaqueSurfaces] = surface;
			numOpaqueSurfaces++;
		}
	}
}

void CXEngine::renderSurface(XSURFACE_INSTANCE *surface)
{
	// acquire some surface resources

	IDirect3DTexture8 *oldTexture;
	IDirect3DTexture8 *oldLightmap;

	IDirect3DTexture8 *newTexture;
	IDirect3DTexture8 *newLightmap;
	
	if (surface->surface->texture)
	{
		surface->surface->texture->acquire(true);

		newTexture = surface->surface->texture->getTexture2D();
	}
	else
	{
		newTexture = NULL;
	}

	if (surface->surface->lightmap)
	{
		surface->surface->lightmap->acquire(true);

		newLightmap = surface->surface->lightmap->getTexture2D();
	}
	else
	{
		newLightmap = NULL;
	}

	// when we've encountered a new shader (or lack of shader), set some surface states

	if (lastShader != surface->surface->shader)
	{
		if (lastShader != NULL)
		{
			lastShader->deactivate();
		}

		lastShader = surface->surface->shader;

		if (lastShader)
		{
			direct3DDevice->SetRenderState(D3DRS_COLORVERTEX, true);

			// todo: what about entity color?

			if (surface->surface->lightingType == XLT_LIGHTMAP)
			{
				lastShader->activate(surface->surface->lightmap);	
			}
			else
			{
				lastShader->activate();
			}
		}
	}

	// regardless of the previous shader, init surface states

	if (!lastShader)
	{
		switch (surface->surface->lightingType)
		{
		case XLT_LIGHTMAP:

			direct3DDevice->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);

			direct3DDevice->GetTexture(1, (IDirect3DBaseTexture8**)&oldLightmap);
			if (oldLightmap) oldLightmap->Release();

			if (oldLightmap != newLightmap)
			{
				direct3DDevice->SetTexture(1, newLightmap);
			}

			break;

		case XLT_VERTEXLIGHTING:

			direct3DDevice->SetRenderState(D3DRS_COLORVERTEX, true);
			direct3DDevice->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_DIFFUSE);

			break;
		}

		direct3DDevice->GetTexture(0, (IDirect3DBaseTexture8**)&oldTexture);
		if (oldTexture) oldTexture->Release();

		if (oldTexture != newTexture)
		{
			direct3DDevice->SetTexture(0, newTexture);
		}
	}
	else
	{
		lastShader->setLightmap(surface->surface->lightmap);
	}

	// render the geometry
	// todo: setup lighting states / entity colors
	// also surface instance transform should not over-ride shader transform, instead they should be multiplied together

	if (surface->hasTransform)
	{
		direct3DDevice->SetTransform(D3DTS_WORLD, &surface->world);
	}

	surface->surface->vertexBuffer->acquire(true);
	surface->surface->indexBuffer->acquire(true);

	DWORD oldFVF; direct3DDevice->GetVertexShader(&oldFVF);

	if (surface->surface->vertexBuffer->isDynamic())
	{
		if (surface->target)
		{
			if (surface->surface->persistantAttributes)
			{
				DWORD newFVF = XENGINE_MD3_VERTEX_FVF;

				if (oldFVF != newFVF)
				{
					direct3DDevice->SetVertexShader(newFVF);
				}

				surface->surface->vertexBuffer->acquire(true);
				surface->surface->indexBuffer->acquire(true);

				surface->surface->persistantAttributes->acquire(true);

				surface->target->acquire(true);
				
				MD3_DYNAMIC_VERTEX *sourceVertices = (MD3_DYNAMIC_VERTEX*)surface->surface->vertexBuffer->getVertices();
				MD3_DYNAMIC_VERTEX *destVertices   = (MD3_DYNAMIC_VERTEX*)surface->target->getVertices();

				TMD3_TEXCOORD *texCoords = (TMD3_TEXCOORD*)surface->surface->persistantAttributes->getVertices();

				int numVertices = surface->surface->vertexBuffer->getNumVertices();
				MD3_VERTEX *tmpVertices = new MD3_VERTEX[numVertices];

				float destWeight = surface->time;
				float sourceWeight = 1.0f - destWeight;

				for (int v = 0; v < numVertices; v++)
				{
					tmpVertices[v].pos    = sourceWeight * sourceVertices[v].pos    + destWeight * destVertices[v].pos;
					tmpVertices[v].normal = sourceWeight * sourceVertices[v].normal + destWeight * destVertices[v].normal;

					tmpVertices[v].texCoord = texCoords[v].texCoord;
				}

				direct3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 0, surface->surface->indexBuffer->getNumTriangles(),
													   surface->surface->indexBuffer->getIndices(), D3DFMT_INDEX16, tmpVertices, sizeof(MD3_VERTEX));

				delete [] tmpVertices;

				surface->surface->vertexBuffer->drop();
				surface->surface->indexBuffer->drop();

				surface->surface->persistantAttributes->drop();

				surface->target->drop();
			}
			else
			{
				// error
			}

		}

		if (surface->surface->shader)
		{
			if (surface->surface->shader->isDynamic())
			{
				// todo: let the shader apply vertex deformations to the surface
			}
		}

		// todo: could still have a dynamic surface without a target, need to draw it
	}
	else
	{
		DWORD newFVF = surface->surface->vertexBuffer->getFVF();

		if (oldFVF != newFVF)
		{
			direct3DDevice->SetVertexShader(newFVF);
		}

		DWORD oldVertexSize;
		DWORD newVertexSize = surface->surface->vertexBuffer->getVertexSize();

		IDirect3DVertexBuffer8 *oldVertexBuffer;
		IDirect3DVertexBuffer8 *newVertexBuffer = surface->surface->vertexBuffer->getVertexBuffer();

		direct3DDevice->GetStreamSource(0, &oldVertexBuffer, (UINT*)&oldVertexSize);

		if (oldVertexBuffer) oldVertexBuffer->Release();

		IDirect3DIndexBuffer8 *oldIndexBuffer;
		IDirect3DIndexBuffer8 *newIndexBuffer = surface->surface->indexBuffer->getIndexBuffer();

		DWORD oldVertexBufferOffset;
		DWORD newVertexBufferOffset = surface->surface->indexBuffer->getVertexBufferOffset();

		direct3DDevice->GetIndices(&oldIndexBuffer, (UINT*)&oldVertexBufferOffset);

		if (oldIndexBuffer) oldIndexBuffer->Release();

		if (oldVertexBuffer != newVertexBuffer || oldVertexSize != newVertexSize)
		{
			direct3DDevice->SetStreamSource(0, newVertexBuffer, newVertexSize);
		}

		if (oldIndexBuffer != newIndexBuffer || oldVertexBufferOffset != newVertexBufferOffset)
		{
			direct3DDevice->SetIndices(newIndexBuffer, newVertexBufferOffset);
		}

		direct3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 0, surface->surface->indexBuffer->getNumTriangles());
	}

	surface->surface->vertexBuffer->drop();
	surface->surface->indexBuffer->drop();

	if (surface->hasTransform)
	{
		D3DXMATRIX identityMatrix;
		D3DXMatrixIdentity(&identityMatrix);

		direct3DDevice->SetTransform(D3DTS_WORLD, &identityMatrix);
	}

	// and let go of our resources

	if (surface->surface->texture)
	{
		surface->surface->texture->drop();
	}

	if (surface->surface->lightmap)
	{
		surface->surface->lightmap->drop();
	}
}

// todo: the top face of the sky box needs to be curved in order to reduce sky box corner artifacts

void CXEngine::renderSkyBox(XSURFACE_INSTANCE *surface)
{
	if (lastShader)
	{
		lastShader->deactivate();
	}

	SHADER_SKYBOX_INFO skyBoxInfo;

	surface->surface->shader->getSkyBoxInfo(&skyBoxInfo);

	direct3DDevice->SetTexture(1, NULL);

	direct3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);
	//direct3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

	D3DXVECTOR3 cameraPosition;
	currentCamera->getPosition(&cameraPosition);

	D3DXMATRIX transMatrix;
	D3DXMatrixTranslation(&transMatrix, cameraPosition.x, cameraPosition.y, cameraPosition.z);
	
	D3DXMATRIX scalingMatrix;
	D3DXMatrixScaling(&scalingMatrix, 4000.0f, 4000.0f, 4000.0f);

	D3DXMatrixMultiply(&transMatrix, &scalingMatrix, &transMatrix);

	direct3DDevice->SetTransform(D3DTS_WORLD, &transMatrix);

	BSP_MESH_VERTEX skyBoxVerts[4 * 6];

	// front (cw winding 0123)

	skyBoxVerts[0].pos = D3DXVECTOR3(-1.0f, -1.0f, 1.0f); skyBoxVerts[0].texCoord = D3DXVECTOR2(0.0f, 0.0f); skyBoxVerts[0].color =  0xFFFFFFFF;
	skyBoxVerts[1].pos = D3DXVECTOR3(-1.0f, +1.0f, 1.0f); skyBoxVerts[1].texCoord = D3DXVECTOR2(0.0f, 1.0f); skyBoxVerts[1].color =  0xFFFFFFFF;
	skyBoxVerts[2].pos = D3DXVECTOR3(+1.0f, +1.0f, 1.0f); skyBoxVerts[2].texCoord = D3DXVECTOR2(1.0f, 1.0f); skyBoxVerts[2].color =  0xFFFFFFFF;
	skyBoxVerts[3].pos = D3DXVECTOR3(+1.0f, -1.0f, 1.0f); skyBoxVerts[3].texCoord = D3DXVECTOR2(1.0f, 0.0f); skyBoxVerts[3].color =  0xFFFFFFFF;

	// back (cw winding reversed)

	skyBoxVerts[4].pos = D3DXVECTOR3(-1.0f, -1.0f, -1.0f); skyBoxVerts[4].texCoord = D3DXVECTOR2(0.0f, 1.0f); skyBoxVerts[4].color =  0xFFFFFFFF;
	skyBoxVerts[7].pos = D3DXVECTOR3(-1.0f, +1.0f, -1.0f); skyBoxVerts[7].texCoord = D3DXVECTOR2(0.0f, 0.0f); skyBoxVerts[7].color =  0xFFFFFFFF;
	skyBoxVerts[6].pos = D3DXVECTOR3(+1.0f, +1.0f, -1.0f); skyBoxVerts[6].texCoord = D3DXVECTOR2(1.0f, 0.0f); skyBoxVerts[6].color =  0xFFFFFFFF;
	skyBoxVerts[5].pos = D3DXVECTOR3(+1.0f, -1.0f, -1.0f); skyBoxVerts[5].texCoord = D3DXVECTOR2(1.0f, 1.0f); skyBoxVerts[5].color =  0xFFFFFFFF;

	// left (cw winding 0123)

	skyBoxVerts[8].pos = D3DXVECTOR3(-1.0f, -1.0f, -1.0f); skyBoxVerts[8].texCoord = D3DXVECTOR2(1.0f, 1.0f); skyBoxVerts[8].color =  0xFFFFFFFF;
	skyBoxVerts[9].pos = D3DXVECTOR3(-1.0f, +1.0f, -1.0f); skyBoxVerts[9].texCoord = D3DXVECTOR2(1.0f, 0.0f); skyBoxVerts[9].color =  0xFFFFFFFF;
	skyBoxVerts[10].pos = D3DXVECTOR3(-1.0f, +1.0f, +1.0f); skyBoxVerts[10].texCoord = D3DXVECTOR2(0.0f, 0.0f); skyBoxVerts[10].color =  0xFFFFFFFF;
	skyBoxVerts[11].pos = D3DXVECTOR3(-1.0f, -1.0f, +1.0f); skyBoxVerts[11].texCoord = D3DXVECTOR2(0.0f, 1.0f); skyBoxVerts[11].color =  0xFFFFFFFF;

	// right (cw winding reversed)

	skyBoxVerts[12].pos = D3DXVECTOR3(+1.0f, -1.0f, -1.0f); skyBoxVerts[12].texCoord = D3DXVECTOR2(1.0f, 1.0f); skyBoxVerts[12].color =  0xFFFFFFFF;
	skyBoxVerts[15].pos = D3DXVECTOR3(+1.0f, +1.0f, -1.0f); skyBoxVerts[15].texCoord = D3DXVECTOR2(1.0f, 0.0f); skyBoxVerts[15].color =  0xFFFFFFFF;
	skyBoxVerts[14].pos = D3DXVECTOR3(+1.0f, +1.0f, +1.0f); skyBoxVerts[14].texCoord = D3DXVECTOR2(0.0f, 0.0f); skyBoxVerts[14].color =  0xFFFFFFFF;
	skyBoxVerts[13].pos = D3DXVECTOR3(+1.0f, -1.0f, +1.0f); skyBoxVerts[13].texCoord = D3DXVECTOR2(0.0f, 1.0f); skyBoxVerts[13].color =  0xFFFFFFFF;

	// top (cw winding 0123)

	skyBoxVerts[16].pos = D3DXVECTOR3(-1.0f, +1.0f, -1.0f); skyBoxVerts[16].texCoord = D3DXVECTOR2(0.0f, 1.0f); skyBoxVerts[16].color =  0xFFFFFFFF;
	skyBoxVerts[19].pos = D3DXVECTOR3(-1.0f, +1.0f, +1.0f); skyBoxVerts[19].texCoord = D3DXVECTOR2(0.0f, 0.0f); skyBoxVerts[19].color =  0xFFFFFFFF;
	skyBoxVerts[18].pos = D3DXVECTOR3(+1.0f, +1.0f, +1.0f); skyBoxVerts[18].texCoord = D3DXVECTOR2(1.0f, 0.0f); skyBoxVerts[18].color =  0xFFFFFFFF;
	skyBoxVerts[17].pos = D3DXVECTOR3(+1.0f, +1.0f, -1.0f); skyBoxVerts[17].texCoord = D3DXVECTOR2(1.0f, 1.0f); skyBoxVerts[17].color =  0xFFFFFFFF;

	// bottom (cw winding reversed)

	skyBoxVerts[20].pos = D3DXVECTOR3(-1.0f, -1.0f, -1.0f); skyBoxVerts[20].texCoord = D3DXVECTOR2(0.0f, 0.0f); skyBoxVerts[20].color =  0xFFFFFFFF;
	skyBoxVerts[21].pos = D3DXVECTOR3(-1.0f, -1.0f, +1.0f); skyBoxVerts[21].texCoord = D3DXVECTOR2(0.0f, 1.0f); skyBoxVerts[21].color =  0xFFFFFFFF;
	skyBoxVerts[22].pos = D3DXVECTOR3(+1.0f, -1.0f, +1.0f); skyBoxVerts[22].texCoord = D3DXVECTOR2(1.0f, 1.0f); skyBoxVerts[22].color =  0xFFFFFFFF;
	skyBoxVerts[23].pos = D3DXVECTOR3(+1.0f, -1.0f, -1.0f); skyBoxVerts[23].texCoord = D3DXVECTOR2(1.0f, 0.0f); skyBoxVerts[23].color =  0xFFFFFFFF;

	// todo: mechanism if a certain face texture is unavailible

	direct3DDevice->SetTexture(0, NULL);

	// front

	if (skyBoxInfo.farBox->front)
	{
		skyBoxInfo.farBox->front->acquire(true);
		direct3DDevice->SetTexture(0, skyBoxInfo.farBox->front->getTexture2D());
	}

	direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[0], sizeof(BSP_MESH_VERTEX));

	if (skyBoxInfo.farBox->front)
	{
		skyBoxInfo.farBox->front->drop();
	}

	// back

	if (skyBoxInfo.farBox->back)
	{
		skyBoxInfo.farBox->back->acquire(true);
		direct3DDevice->SetTexture(0, skyBoxInfo.farBox->back->getTexture2D());
	}

	direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[4], sizeof(BSP_MESH_VERTEX));

	if (skyBoxInfo.farBox->back)
	{
		skyBoxInfo.farBox->back->drop();
	}

	// left

	if (skyBoxInfo.farBox->left)
	{
		skyBoxInfo.farBox->left->acquire(true);
		direct3DDevice->SetTexture(0, skyBoxInfo.farBox->left->getTexture2D());
	}

	direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[8], sizeof(BSP_MESH_VERTEX));

	if (skyBoxInfo.farBox->left)
	{
		skyBoxInfo.farBox->left->drop();
	}

	// right

	if (skyBoxInfo.farBox->right)
	{
		skyBoxInfo.farBox->right->acquire(true);
		direct3DDevice->SetTexture(0, skyBoxInfo.farBox->right->getTexture2D());
	}

	direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[12], sizeof(BSP_MESH_VERTEX));

	if (skyBoxInfo.farBox->right)
	{
		skyBoxInfo.farBox->right->drop();
	}

	// up

	if (skyBoxInfo.farBox->up)
	{
		skyBoxInfo.farBox->up->acquire(true);
		direct3DDevice->SetTexture(0, skyBoxInfo.farBox->up->getTexture2D());
	}

	direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[16], sizeof(BSP_MESH_VERTEX));

	if (skyBoxInfo.farBox->up)
	{
		skyBoxInfo.farBox->up->drop();
	}

	// down

	if (skyBoxInfo.farBox->down)
	{
		skyBoxInfo.farBox->down->acquire(true);
		direct3DDevice->SetTexture(0, skyBoxInfo.farBox->down->getTexture2D());
	}

	direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[20], sizeof(BSP_MESH_VERTEX));

	if (skyBoxInfo.farBox->down)
	{
		skyBoxInfo.farBox->down->drop();
	}

	if (surface->surface->shader->getNumStages() > 0)
	{
		surface->surface->shader->activate();

		direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[0], sizeof(BSP_MESH_VERTEX));
		direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[4], sizeof(BSP_MESH_VERTEX));
		direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[8], sizeof(BSP_MESH_VERTEX));
		direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[12], sizeof(BSP_MESH_VERTEX));
		direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[16], sizeof(BSP_MESH_VERTEX));
		direct3DDevice->DrawVerticesUP(D3DPT_QUADLIST, 4, &skyBoxVerts[20], sizeof(BSP_MESH_VERTEX));

		surface->surface->shader->deactivate();
	}

	lastShader = NULL;

	D3DXMatrixIdentity(&transMatrix);
	direct3DDevice->SetTransform(D3DTS_WORLD, &transMatrix);

	direct3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
	//direct3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
}

int _cdecl opaqueSurfaceInstanceSort(const VOID* arg1, const VOID* arg2)
{
    XSURFACE_INSTANCE* s1 = *((XSURFACE_INSTANCE**)arg1);
    XSURFACE_INSTANCE* s2 = *((XSURFACE_INSTANCE**)arg2);

	// todo: work out which attributes which be best to sort by. right now we're going vertex shader is most important,
	// and shader is second most important, and the rest is unimportant.

	if (s1->surface->shader == s2->surface->shader)
	{
		return 0;
	}
	else
	{
		return +1; // todo: would this be faster if this was -1?
	}
}

int _cdecl blendedSurfaceInstanceSort(const VOID* arg1, const VOID* arg2)
{
    XSURFACE_INSTANCE* s1 = *((XSURFACE_INSTANCE**)arg1);
    XSURFACE_INSTANCE* s2 = *((XSURFACE_INSTANCE**)arg2);

	// todo: getPosition only once and cache it?

	D3DXVECTOR3 camPos; XEngine->getCurrentCamera()->getPosition(&camPos);

	D3DXVECTOR3 *surface1Position;
	D3DXVECTOR3 *surface2Position;

	if (s1->hasTransform)
	{
		D3DXVECTOR4 tmp; D3DXVec3Transform(&tmp, &s1->surface->pos, &s1->world);
		surface1Position = (D3DXVECTOR3*)&tmp;
	}
	else
	{
		surface1Position = &s1->surface->pos;
	}

	if (s2->hasTransform)
	{
		D3DXVECTOR4 tmp; D3DXVec3Transform(&tmp, &s2->surface->pos, &s2->world);
		surface2Position = (D3DXVECTOR3*)&tmp;
	}
	else
	{
		surface2Position = &s2->surface->pos;
	}

	D3DXVECTOR3 delta1 = camPos - (*surface1Position);
	D3DXVECTOR3 delta2 = camPos - (*surface2Position);

	if (D3DXVec3Dot(&delta1, &delta1) < D3DXVec3Dot(&delta2, &delta2))
	{
		return +1;
	}
	else
	{
		return -1;
	}
}

void CXEngine::updateGraphics()
{
	if (CBSPResource::world)
	{
		// don't bother clearing the color buffer if we're in a bsp world, it would just be rendered over anyway

		//direct3DDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
		direct3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	}
	else
	{
		direct3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	}

	// these are actually completely and totally functionless in d3d8x, looks good for posterity though

	direct3DDevice->BeginScene();

	// camera by camera, render the game world. the world->render() function will give us our big list of
	// surfaces instances that need to be rendered in this frame for this camera. at that point, we'll want
	// to first sort out the opaque surfaces by shader for speed, and then interate through them and render them.
	// next, if a skybox needs to be rendered, render it. then sort all the blended surface instances by distance
	// from the camera, and render those as well. at this point, we're done!

	for(list<CEntity*>::iterator iCamera = CCameraEntity::cameraEntities.begin(); iCamera != CCameraEntity::cameraEntities.end(); iCamera++)
	{	
		CCameraEntity *camera = (CCameraEntity*)(*iCamera);
		
		camera->activate();
		currentCamera = camera;

		renderFrame++;

		numOpaqueSurfaces  = 0;
		numBlendedSurfaces = 0;

		skyBoxSurface = NULL;

		if (CBSPResource::world)
		{
			CBSPResource::world->render();
		}
		else
		{
			for(list<CEntity*>::iterator iPhysicalEntity = CPhysicalEntity::physicalEntities.begin(); iPhysicalEntity != CPhysicalEntity::physicalEntities.end(); iPhysicalEntity++)
			{	
				CPhysicalEntity* physicalEntity = (CPhysicalEntity*)(*iPhysicalEntity);

				if (physicalEntity->getRenderEnable())
				{
					if (physicalEntity->getLastFrameRendered() < renderFrame && !physicalEntity->isHidden())
					{
						D3DXVECTOR3 boundingSpherePos;
						physicalEntity->getPosition(&boundingSpherePos);

						float boundingSphereRadius = physicalEntity->getBoundingSphereRadius();

						if (currentCamera->sphereInFrustum(boundingSpherePos.x,
													       boundingSpherePos.y,
														   boundingSpherePos.z,
														   boundingSphereRadius))
						{
							physicalEntity->render();
						}
					}
				}
			}
		}

		// todo: render renderable non-physical entities (gui shit)

		qsort(opaqueSurfaceInstances, numOpaqueSurfaces, sizeof(XSURFACE_INSTANCE*), opaqueSurfaceInstanceSort);

		for (int s = 0; s < numOpaqueSurfaces; s++)
		{
			renderSurface(opaqueSurfaceInstances[s]);
		}

		if (skyBoxSurface)
		{
			renderSkyBox(skyBoxSurface);
		}

		qsort(blendedSurfaceInstances, numBlendedSurfaces, sizeof(XSURFACE_INSTANCE*), blendedSurfaceInstanceSort);

		for (int s = 0; s < numBlendedSurfaces; s++)
		{
			renderSurface(blendedSurfaceInstances[s]);
		}
	}

	currentCamera = NULL;

	D3DVIEWPORT8 viewport;

	viewport.X = 0;
	viewport.Y = 0;

	viewport.Width  = 640;
	viewport.Height = 480;

	direct3DDevice->SetViewport(&viewport);

	direct3DDevice->EndScene();

	direct3DDevice->Present(NULL, NULL, NULL, NULL);
}

void CXEngine::shutdownGraphics()
{
	// todo: release device, other graphics shutdown stuff, etc.
}

// ********* timing ********* 

void CXEngine::initTiming()
{
	// timing and tweening init

	ticks	  = 0.0f;
	gameSpeed = 1.0;
	tween	  = 1.0f;

	frame		= 0;
	renderFrame = 0;

	QueryPerformanceCounter(&millisecs);
}

void CXEngine::setGameSpeed(float speed)
{
	if (speed < 0.0f)
	{
		debugPrint("*** ERROR *** Negative game speed change requested\n");
	}
	else
	{
		gameSpeed = speed;
	}
}

DWORD CXEngine::getMillisecs()
{
	return timeGetTime();
}

float CXEngine::getTween()
{
	return tween;
}

float CXEngine::getTicks()
{
	return ticks;
}

float CXEngine::getGameSpeed()
{
	return gameSpeed;
}

float CXEngine::getFPS()
{
	return fps;
}

int CXEngine::getFrame()
{
	return frame;
}

int CXEngine::getRenderFrame()
{
	return renderFrame;
}

void CXEngine::updateTiming()
{
	lastMillisecs.QuadPart = millisecs.QuadPart;
	QueryPerformanceCounter(&millisecs);

	float newTween = (millisecs.QuadPart - lastMillisecs.QuadPart) / (733333333.0f / 60.0f) * gameSpeed;
	
	/*
	if (fAbs(newTween - tween) > XENGINE_TIMING_STUTTER_THRESHOLD)
	{
		newTween = tween;
	}

	tween = tween * (1.0f - XENGINE_TIMING_STUTTER_SMOOTHING) + newTween * XENGINE_TIMING_STUTTER_SMOOTHING;
	*/

	tween = newTween;

	if (tween > XENGINE_TIMING_MAX_TWEEN)
	{
		tween = XENGINE_TIMING_MAX_TWEEN;

		// todo: any other corrective action here? cuz now the game speed is slowing down
	}

	// fixes weird stutters when the frame rate doesn't nessecarily change, or changes very slightly

	ticks += tween;

	frame++;

	fps = (60.0f / tween);
}

void CXEngine::shutdownTiming()
{
	// todo: any nessecary timing subsystem shutdown
}

// ********* input ********* 

void CXEngine::initInput()
{
	XInitDevices(0, 0);

    DWORD dwDeviceMask = XGetDevices(XDEVICE_TYPE_GAMEPAD);  // we're only concerned with xbox controllers

    for(DWORD i = 0; i < XGetPortCount(); i++)				 // haha, XGetPortCount() always returns 4, really, it's a define
    {														 // it's not even a function! silly microsoft
        ZeroMemory(&inputStates[i], sizeof(XINPUT_STATE));
        ZeroMemory(&gamePad[i], sizeof(XBGAMEPAD));

        if(dwDeviceMask & (1 <<i )) 
        {
            gamePad[i].device = XInputOpen(XDEVICE_TYPE_GAMEPAD, i, XDEVICE_NO_SLOT, NULL);

            XInputGetCapabilities(gamePad[i].device, &gamePad[i].caps);
        }
    }
}

void CXEngine::getAxisState(int portNum, int axis, D3DXVECTOR2 *axisState)
{
	if (portNum == XENGINE_INPUT_ALL_PORTS)
	{
		ZeroMemory(axisState, sizeof(D3DXVECTOR2));

		switch(axis)
		{
		case XENGINE_INPUT_AXIS_LEFT_JOYSTICK:
	
			{
				for (int i = 0; i < 4; i++)
				{
					if (gamePad[i].device != NULL)
					{
						axisState->x += gamePad[i].x1;
						axisState->y += gamePad[i].y1;
					}
				}

				if (axisState->x < -1.0f) axisState->x = -1.0f;
				if (axisState->x > +1.0f) axisState->x = +1.0f;

				if (axisState->y < -1.0f) axisState->y = -1.0f;
				if (axisState->y > +1.0f) axisState->y = +1.0f;
			}

			return;

		case XENGINE_INPUT_AXIS_RIGHT_JOYSTICK:

			{
				for (int i = 0; i < 4; i++)
				{
					if (gamePad[i].device != NULL)
					{
						axisState->x += gamePad[i].x2;
						axisState->y += gamePad[i].y2;
					}
				}

				if (axisState->x < -1.0f) axisState->x = -1.0f;
				if (axisState->x > +1.0f) axisState->x = +1.0f;

				if (axisState->y < -1.0f) axisState->y = -1.0f;
				if (axisState->y > +1.0f) axisState->y = +1.0f;
			}

			return;

		default:

			{
				string debugString = "*** ERROR *** Invalid input axis requested of type '";

				debugString += intString(axis);
				debugString += "'\n";

				debugPrint(debugString);
			}

			return;
		}
	}
	else if (portNum >= 0 && portNum < 4)
	{
		if (gamePad[portNum].device != NULL)
		{
			switch(axis)
			{
			case XENGINE_INPUT_AXIS_LEFT_JOYSTICK:

				axisState->x = gamePad[portNum].x1;
				axisState->y = gamePad[portNum].y1;

				return;
			
			case XENGINE_INPUT_AXIS_RIGHT_JOYSTICK:

				axisState->x = gamePad[portNum].x2;
				axisState->y = gamePad[portNum].y2;

				return;

			default:

				{
					string debugString = "*** ERROR *** Invalid input axis requested of type '";

					debugString += intString(axis);
					debugString += "'\n";

					debugPrint(debugString);

					ZeroMemory(axisState, sizeof(D3DXVECTOR2));
				}

				return;
			}
		}
		else
		{
			string debugString = "*** WARNING *** Disconnected input port number requested '";

			debugString += intString(portNum);
			debugString += "'\n";

			debugPrint(debugString);

			ZeroMemory(axisState, sizeof(D3DXVECTOR2));

			return;
		}
	}
	else
	{
		string debugString = "*** ERROR *** Invalid input port number requested '";

		debugString += intString(portNum);
		debugString += "'\n";

		debugPrint(debugString);

		ZeroMemory(axisState, sizeof(D3DXVECTOR2));

		return;
	}
}

int CXEngine::getButtonDown(int portNum, int button)
{
	if (portNum == XENGINE_INPUT_ALL_PORTS)
	{
		int result = 0;

		switch(button)
		{
		case XENGINE_INPUT_BUTTON_A:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].analogButtons[XINPUT_GAMEPAD_A] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_B:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].analogButtons[XINPUT_GAMEPAD_B] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_X:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].analogButtons[XINPUT_GAMEPAD_X] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_Y:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].analogButtons[XINPUT_GAMEPAD_Y] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_BLACK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].analogButtons[XINPUT_GAMEPAD_BLACK] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_WHITE:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].analogButtons[XINPUT_GAMEPAD_WHITE] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_LTRIGGER:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].analogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_RTRIGGER:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].analogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_START:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_START);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_BACK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_BACK);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_UP:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_DPAD_UP);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_DOWN:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_DPAD_DOWN);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_LEFT:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_DPAD_LEFT);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_RIGHT:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_DPAD_RIGHT);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_LEFT_JOYSTICK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_LEFT_THUMB);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_RIGHT_JOYSTICK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_RIGHT_THUMB);
				}
			}

			return result;

		default:

			{
				string debugString = "*** ERROR *** Invalid input button requested of type '";

				debugString += intString(button);
				debugString += "'\n";

				debugPrint(debugString);
			}

			return 0;
		}
	}
	else if (portNum >= 0 && portNum < 4)
	{
		if (gamePad[portNum].device != NULL)
		{
			switch(button)
			{
			case XENGINE_INPUT_BUTTON_A:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_A] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);

			case XENGINE_INPUT_BUTTON_B:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_B] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);

			case XENGINE_INPUT_BUTTON_X:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_X] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);

			case XENGINE_INPUT_BUTTON_Y:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_Y] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);

			case XENGINE_INPUT_BUTTON_BLACK:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_BLACK] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);

			case XENGINE_INPUT_BUTTON_WHITE:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_WHITE] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);

			case XENGINE_INPUT_BUTTON_LTRIGGER:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);

			case XENGINE_INPUT_BUTTON_RTRIGGER:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);

			case XENGINE_INPUT_BUTTON_START:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_START);

			case XENGINE_INPUT_BUTTON_BACK:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_BACK);

			case XENGINE_INPUT_BUTTON_DPAD_UP:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_DPAD_UP);

			case XENGINE_INPUT_BUTTON_DPAD_DOWN:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_DPAD_DOWN);

			case XENGINE_INPUT_BUTTON_DPAD_LEFT:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_DPAD_LEFT);

			case XENGINE_INPUT_BUTTON_DPAD_RIGHT:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_DPAD_RIGHT);

			case XENGINE_INPUT_BUTTON_LEFT_JOYSTICK:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_LEFT_THUMB);

			case XENGINE_INPUT_BUTTON_RIGHT_JOYSTICK:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_RIGHT_THUMB);

			default:

				{
					string debugString = "*** ERROR *** Invalid input button requested of type '";

					debugString += intString(button);
					debugString += "'\n";

					debugPrint(debugString);
				}

				return 0;

			}
		}
		else
		{
			string debugString = "*** WARNING *** Disconnected input port number requested '";

			debugString += intString(portNum);
			debugString += "'\n";

			debugPrint(debugString);

			return 0;
		}
	}
	else
	{
		string debugString = "*** ERROR *** Invalid input port number requested '";

		debugString += intString(portNum);
		debugString += "'\n";

		debugPrint(debugString);

		return 0;
	}
}


int CXEngine::getButtonPressed(int portNum, int button)
{
	if (portNum == XENGINE_INPUT_ALL_PORTS)
	{
		int result = 0;

		switch(button)
		{
		case XENGINE_INPUT_BUTTON_A:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedAnalogButtons[XINPUT_GAMEPAD_A]);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_B:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedAnalogButtons[XINPUT_GAMEPAD_B]);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_X:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedAnalogButtons[XINPUT_GAMEPAD_X]);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_Y:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedAnalogButtons[XINPUT_GAMEPAD_Y]);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_BLACK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedAnalogButtons[XINPUT_GAMEPAD_BLACK]);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_WHITE:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedAnalogButtons[XINPUT_GAMEPAD_WHITE]);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_LTRIGGER:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER]);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_RTRIGGER:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER]);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_START:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedButtons & XINPUT_GAMEPAD_START);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_BACK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedButtons & XINPUT_GAMEPAD_BACK);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_UP:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedButtons & XINPUT_GAMEPAD_DPAD_UP);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_DOWN:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedButtons & XINPUT_GAMEPAD_DPAD_DOWN);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_LEFT:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedButtons & XINPUT_GAMEPAD_DPAD_LEFT);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_RIGHT:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_LEFT_JOYSTICK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedButtons & XINPUT_GAMEPAD_LEFT_THUMB);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_RIGHT_JOYSTICK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].pressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
				}
			}

			return result;

		default:

			{
				string debugString = "*** ERROR *** Invalid input button requested of type '";

				debugString += intString(button);
				debugString += "'\n";

				debugPrint(debugString);
			}

			return 0;
		}
	}
	else if (portNum >= 0 && portNum < 4)
	{
		if (gamePad[portNum].device != NULL)
		{
			switch(button)
			{
			case XENGINE_INPUT_BUTTON_A:

				return (gamePad[portNum].pressedAnalogButtons[XINPUT_GAMEPAD_A]);

			case XENGINE_INPUT_BUTTON_B:

				return (gamePad[portNum].pressedAnalogButtons[XINPUT_GAMEPAD_B]);

			case XENGINE_INPUT_BUTTON_X:

				return (gamePad[portNum].pressedAnalogButtons[XINPUT_GAMEPAD_X]);

			case XENGINE_INPUT_BUTTON_Y:

				return (gamePad[portNum].pressedAnalogButtons[XINPUT_GAMEPAD_Y]);

			case XENGINE_INPUT_BUTTON_BLACK:

				return (gamePad[portNum].pressedAnalogButtons[XINPUT_GAMEPAD_BLACK]);

			case XENGINE_INPUT_BUTTON_WHITE:

				return (gamePad[portNum].pressedAnalogButtons[XINPUT_GAMEPAD_WHITE]);

			case XENGINE_INPUT_BUTTON_LTRIGGER:

				return (gamePad[portNum].pressedAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER]);

			case XENGINE_INPUT_BUTTON_RTRIGGER:

				return (gamePad[portNum].pressedAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER]);

			case XENGINE_INPUT_BUTTON_START:

				return (gamePad[portNum].pressedButtons & XINPUT_GAMEPAD_START);

			case XENGINE_INPUT_BUTTON_BACK:

				return (gamePad[portNum].pressedButtons & XINPUT_GAMEPAD_BACK);

			case XENGINE_INPUT_BUTTON_DPAD_UP:

				return (gamePad[portNum].pressedButtons & XINPUT_GAMEPAD_DPAD_UP);

			case XENGINE_INPUT_BUTTON_DPAD_DOWN:

				return (gamePad[portNum].pressedButtons & XINPUT_GAMEPAD_DPAD_DOWN);

			case XENGINE_INPUT_BUTTON_DPAD_LEFT:

				return (gamePad[portNum].pressedButtons & XINPUT_GAMEPAD_DPAD_LEFT);

			case XENGINE_INPUT_BUTTON_DPAD_RIGHT:

				return (gamePad[portNum].pressedButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

			case XENGINE_INPUT_BUTTON_LEFT_JOYSTICK:

				return (gamePad[portNum].pressedButtons & XINPUT_GAMEPAD_LEFT_THUMB);

			case XENGINE_INPUT_BUTTON_RIGHT_JOYSTICK:

				return (gamePad[portNum].pressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB);

			default:

				{
					string debugString = "*** ERROR *** Invalid input button requested of type '";

					debugString += intString(button);
					debugString += "'\n";

					debugPrint(debugString);
				}

				return 0;

			}
		}
		else
		{
			string debugString = "*** WARNING *** Disconnected input port number requested '";

			debugString += intString(portNum);
			debugString += "'\n";

			debugPrint(debugString);

			return 0;
		}
	}
	else
	{
		string debugString = "*** ERROR *** Invalid input port number requested '";

		debugString += intString(portNum);
		debugString += "'\n";

		debugPrint(debugString);

		return 0;
	}
}

int CXEngine::getButtonState(int portNum, int button)
{
	if (portNum == XENGINE_INPUT_ALL_PORTS)
	{
		int result = 0;

		switch(button)
		{
		case XENGINE_INPUT_BUTTON_A:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result += gamePad[i].analogButtons[XINPUT_GAMEPAD_A];
				}
			}

			if (result > 255) result = 255;

			return result;

		case XENGINE_INPUT_BUTTON_B:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result += gamePad[i].analogButtons[XINPUT_GAMEPAD_B];
				}
			}

			if (result > 255) result = 255;

			return result;

		case XENGINE_INPUT_BUTTON_X:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result += gamePad[i].analogButtons[XINPUT_GAMEPAD_X];
				}
			}

			if (result > 255) result = 255;

			return result;

		case XENGINE_INPUT_BUTTON_Y:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result += gamePad[i].analogButtons[XINPUT_GAMEPAD_Y];
				}
			}

			if (result > 255) result = 255;

			return result;

		case XENGINE_INPUT_BUTTON_BLACK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result += gamePad[i].analogButtons[XINPUT_GAMEPAD_BLACK];
				}
			}

			if (result > 255) result = 255;

			return result;

		case XENGINE_INPUT_BUTTON_WHITE:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result += gamePad[i].analogButtons[XINPUT_GAMEPAD_WHITE];
				}
			}

			if (result > 255) result = 255;

			return result;

		case XENGINE_INPUT_BUTTON_LTRIGGER:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result += gamePad[i].analogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER];
				}
			}

			if (result > 255) result = 255;

			return result;

		case XENGINE_INPUT_BUTTON_RTRIGGER:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result += gamePad[i].analogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER];
				}
			}

			if (result > 255) result = 255;

			return result;

		case XENGINE_INPUT_BUTTON_START:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_START);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_BACK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_BACK);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_UP:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_DPAD_UP);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_DOWN:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_DPAD_DOWN);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_LEFT:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_DPAD_LEFT);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_DPAD_RIGHT:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_DPAD_RIGHT);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_LEFT_JOYSTICK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_LEFT_THUMB);
				}
			}

			return result;

		case XENGINE_INPUT_BUTTON_RIGHT_JOYSTICK:

			for (int i = 0; i < 4; i++)
			{
				if (gamePad[i].device != NULL)
				{
					result |= (gamePad[i].buttons & XINPUT_GAMEPAD_RIGHT_THUMB);
				}
			}

			return result;

		default:

			{
				string debugString = "*** ERROR *** Invalid input button requested of type '";

				debugString += intString(button);
				debugString += "'\n";

				debugPrint(debugString);
			}

			return 0;
		}
	}
	else if (portNum >= 0 && portNum < 4)
	{
		if (gamePad[portNum].device != NULL)
		{
			switch(button)
			{
			case XENGINE_INPUT_BUTTON_A:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_A]);

			case XENGINE_INPUT_BUTTON_B:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_B]);

			case XENGINE_INPUT_BUTTON_X:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_X]);

			case XENGINE_INPUT_BUTTON_Y:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_Y]);

			case XENGINE_INPUT_BUTTON_BLACK:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_BLACK]);

			case XENGINE_INPUT_BUTTON_WHITE:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_WHITE]);

			case XENGINE_INPUT_BUTTON_LTRIGGER:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER]);

			case XENGINE_INPUT_BUTTON_RTRIGGER:

				return (gamePad[portNum].analogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER]);

			case XENGINE_INPUT_BUTTON_START:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_START);

			case XENGINE_INPUT_BUTTON_BACK:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_BACK);

			case XENGINE_INPUT_BUTTON_DPAD_UP:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_DPAD_UP);

			case XENGINE_INPUT_BUTTON_DPAD_DOWN:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_DPAD_DOWN);

			case XENGINE_INPUT_BUTTON_DPAD_LEFT:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_DPAD_LEFT);

			case XENGINE_INPUT_BUTTON_DPAD_RIGHT:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_DPAD_RIGHT);

			case XENGINE_INPUT_BUTTON_LEFT_JOYSTICK:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_LEFT_THUMB);

			case XENGINE_INPUT_BUTTON_RIGHT_JOYSTICK:

				return (gamePad[portNum].buttons & XINPUT_GAMEPAD_RIGHT_THUMB);

			default:

				{
					string debugString = "*** ERROR *** Invalid input button requested of type '";

					debugString += intString(button);
					debugString += "'\n";

					debugPrint(debugString);
				}

				return 0;

			}
		}
		else
		{
			string debugString = "*** WARNING *** Disconnected input port number requested '";

			debugString += intString(portNum);
			debugString += "'\n";

			debugPrint(debugString);

			return 0;
		}
	}
	else
	{
		string debugString = "*** ERROR *** Invalid input port number requested '";

		debugString += intString(portNum);
		debugString += "'\n";

		debugPrint(debugString);

		return 0;
	}
}

int CXEngine::getAnyButtonPressed(int portNum)
{
	// todo: fill this in

	return 0;
}

int CXEngine::isPortConnected(int portNum)
{
	if (portNum == XENGINE_INPUT_ALL_PORTS)
	{
		for (int i = 0; i < 4; i++)
		{
			if (gamePad[i].device != NULL)
			{
				return true;
			}
		}

		return false;
	}
	else if (portNum >= 0 && portNum < 4)
	{
		return (gamePad[portNum].device != NULL);
	}
	else
	{
		string debugString = "*** ERROR *** Invalid input port number requested '";

		debugString += intString(portNum);
		debugString += "'\n";

		debugPrint(debugString);

		return 0;
	}
}

void CXEngine::updateInput()
{
	//todo: update and handle controller input and generate engine events for connects/disconnects
	//      if there were ever going to be network support, this is where it would be updated.

    DWORD insertions, removals;
    XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals);

    for (DWORD i = 0; i < XGetPortCount(); i++)
    {
        // handle removed devices

        gamePad[i].removed = (removals & (1 << i)) ? TRUE : FALSE;

        if (gamePad[i].removed)
        {
            // if the controller was removed after XGetDeviceChanges but before
            // XInputOpen, the device handle will be NULL

            if (gamePad[i].device)
			{
                XInputClose(gamePad[i].device);
			}

            gamePad[i].device = NULL;

			// shutdown rumble when controller becomes disconnected, this is important ;)

            gamePad[i].feedback.Rumble.wLeftMotorSpeed  = 0;
            gamePad[i].feedback.Rumble.wRightMotorSpeed = 0;

			// and lastly, let the engine know we're operating less one controller here
			// if we're in the middle of a game, we might want to pause and let the player know
			// that he needs to plug it in.

			ENGINE_EVENT *engineEvent = new ENGINE_EVENT;
			EE_CONTROLLER_DISCONNECTION_STRUCT *controllerDisconnectInfo = new EE_CONTROLLER_DISCONNECTION_STRUCT;

			controllerDisconnectInfo->portNum = i;

			engineEvent->eventType = EE_CONTROLLER_DISCONNECTION;
			engineEvent->eventData = (void*)controllerDisconnectInfo;

			queueEngineEvent(engineEvent);
        }
		
        // handle inserted devices

        gamePad[i].inserted = (insertions & (1 << i)) ? TRUE : FALSE;

        if (gamePad[i].inserted) 
        {
            // TCR 1-14 device types

            gamePad[i].device = XInputOpen(XDEVICE_TYPE_GAMEPAD, i, XDEVICE_NO_SLOT, NULL);

            // if the controller is removed after XGetDeviceChanges but before
            // XInputOpen, the device handle will be NULL

            if(gamePad[i].device)
			{
                XInputGetCapabilities(gamePad[i].device, &gamePad[i].caps);
			}

			// yay, another controller. notify the engine via an engine event

			ENGINE_EVENT *engineEvent = new ENGINE_EVENT;
			EE_CONTROLLER_CONNECTION_STRUCT *controllerConnectInfo = new EE_CONTROLLER_CONNECTION_STRUCT;

			controllerConnectInfo->portNum = i;

			engineEvent->eventType = EE_CONTROLLER_CONNECTION;
			engineEvent->eventData = (void*)controllerConnectInfo;

			queueEngineEvent(engineEvent);
        }

        // if we have a valid device, poll it's state and track button changes

        if (gamePad[i].device)
        {
            // read the input state

            XInputGetState(gamePad[i].device, &inputStates[i]);
			
            // copy gamepad to local structure

            memcpy(&gamePad[i], &inputStates[i].Gamepad, sizeof(XINPUT_GAMEPAD));

            // put Xbox device input for the gamepad into our custom format

            FLOAT x1 = (gamePad[i].thumbLX + 0.5f) / 32767.5f;
            gamePad[i].x1 = (x1 >= 0.0f ? 1.0f : -1.0f) * max(0.0f, (fabsf(x1) - XENGINE_INPUT_DEADZONE) / (1.0f - XENGINE_INPUT_DEADZONE));

            FLOAT y1 = (gamePad[i].thumbLY + 0.5f) / 32767.5f;
            gamePad[i].y1 = (y1 >= 0.0f ? 1.0f : -1.0f) * max(0.0f, (fabsf(y1) - XENGINE_INPUT_DEADZONE) / (1.0f - XENGINE_INPUT_DEADZONE));

            FLOAT x2 = (gamePad[i].thumbRX + 0.5f) / 32767.5f;
            gamePad[i].x2 = (x2 >= 0.0f ? 1.0f : -1.0f) * max(0.0f, (fabsf(x2) - XENGINE_INPUT_DEADZONE) / (1.0f - XENGINE_INPUT_DEADZONE));

            FLOAT y2 = (gamePad[i].thumbRY + 0.5f) / 32767.5f;
            gamePad[i].y2 = (y2 >= 0.0f ? 1.0f : -1.0f) * max(0.0f, (fabsf(y2) - XENGINE_INPUT_DEADZONE) / (1.0f - XENGINE_INPUT_DEADZONE));

            // get the boolean buttons that have been pressed since the last
            // call. each button is represented by one bit

            gamePad[i].pressedButtons = (gamePad[i].lastButtons ^ gamePad[i].buttons) & gamePad[i].buttons;
            gamePad[i].lastButtons    =  gamePad[i].buttons;

            // get the analog buttons that have been pressed or released since
            // the last call

            for (DWORD b = 0; b < 8; b++)
            {
                // turn the 8-bit polled value into a boolean value

                BOOL pressed = (gamePad[i].analogButtons[b] > XENGINE_INPUT_GAMEPAD_MAX_CROSSTALK);

                if(pressed)
				{
                    gamePad[i].pressedAnalogButtons[b] = !gamePad[i].lastAnalogButtons[b];
				}
                else
				{
                    gamePad[i].pressedAnalogButtons[b] = FALSE;
				}
                
                // store the current state for the next time

                gamePad[i].lastAnalogButtons[b] = pressed;
            }
        }
    }
}

void CXEngine::shutdownInput()
{
	// todo: shutdown and release our devices cleanly, and stuff
}

// ********* debugging ********* 

void CXEngine::initDebugging()
{
	debugLog = fopen("d:\\log.txt", "wb");

	debugPrint("XEngine - Beginning initialization process...\n");
}

void CXEngine::debugPrint(string debugText)
{
#ifdef _DEBUG

	OutputDebugString(debugText.c_str());

#endif

	if (debugLog)
	{
		fprintf(debugLog, "%s", debugText.c_str());

		fflush(debugLog);
	}
}

void CXEngine::debugRenderText(string debugText, int x, int y)
{
	WCHAR wbuff[128];;
	swprintf(wbuff, L"%hs", debugText.c_str());

	debugFont->SetTextColor(D3DCOLOR_XRGB(0, 0, 0));

	debugFont->TextOut(backBuffer, wbuff, debugText.length(), x - 1, y);
	debugFont->TextOut(backBuffer, wbuff, debugText.length(), x + 1, y);
	debugFont->TextOut(backBuffer, wbuff, debugText.length(), x, y - 1);
	debugFont->TextOut(backBuffer, wbuff, debugText.length(), x, y + 1);

	debugFont->SetTextColor(D3DCOLOR_XRGB(255, 255, 255));
	debugFont->TextOut(backBuffer, wbuff, debugText.length(), x, y);
}

void CXEngine::updateDebugging()
{
	// todo: consider a more sophisticated debugging mechanism
}

void CXEngine::shutdownDebugging()
{
	debugPrint("XEngine shutdown successfully. Closing debug log...\n");

	fclose(debugLog);
}

// ********* engine events ********* 

void CXEngine::initEngineEvents()
{
	// todo: any engine event init code should go here
}

void CXEngine::queueEngineEvent(ENGINE_EVENT *engineEvent)
{
	engineEventQueue.push_front(engineEvent);
}

void CXEngine::updateEngineEvents()
{
	while (!engineEventQueue.empty() && !terminated)
	{
		ENGINE_EVENT *engineEvent = engineEventQueue.front();

		engineEventQueue.pop_front();

		switch (engineEvent->eventType) // todo: handle all engine events
		{
		case EE_ENTITY_TERMINATION:

			{
				EE_ENTITY_TERMINATION_STRUCT *entityTerminationInfo = (EE_ENTITY_TERMINATION_STRUCT*)engineEvent->eventData;

				delete entityTerminationInfo->terminatingEntity;
				delete entityTerminationInfo;
			}

			break;

		case EE_RESOURCE_TERMINATION:

			{
				EE_RESOURCE_TERMINATION_STRUCT *resourceTerminationInfo = (EE_RESOURCE_TERMINATION_STRUCT*)engineEvent->eventData;
	
				delete resourceTerminationInfo->terminatingResource;
				delete resourceTerminationInfo;
			}

			break;

		case EE_APPLICATION_TERMINATION:

			{
				EE_APPLICATION_TERMINATION_STRUCT *appTermInfo = (EE_APPLICATION_TERMINATION_STRUCT*)engineEvent->eventData;
	
				string debugString = "";

				switch(appTermInfo->terminationCode)
				{
				case APPTERM_SUCCESS:
					
						debugPrint("Application requesting normal termination...");
						
					break;

				case APPTERM_ERROR:

						debugString = "*** ERROR *** Application requesting abnormal termination! '";
						debugString += appTermInfo->detailedError;
						debugString += "'\n\n";

						debugPrint(debugString);

					break;
				}

				terminated = true;

				delete appTermInfo;
			}

			break;

		case EE_CONTROLLER_DISCONNECTION:

			{
				EE_CONTROLLER_DISCONNECTION_STRUCT *controllerDisconnectInfo = (EE_CONTROLLER_DISCONNECTION_STRUCT*)engineEvent->eventData;
	
				// todo: at this point, we may want to broadcast a message to entities giving them the port number

				delete controllerDisconnectInfo;
			}

			break;

		case EE_CONTROLLER_CONNECTION:

			{
				EE_CONTROLLER_CONNECTION_STRUCT *controllerConnectInfo = (EE_CONTROLLER_CONNECTION_STRUCT*)engineEvent->eventData;
	
				// todo: at this point, we may want to broadcast a message to entities giving them the port number

				delete controllerConnectInfo;
			}

			break;

		default:
			
			{
				string debugString = "*** ERROR *** Unknown engine event found in queue of type '";
	
				debugString += intString(engineEvent->eventType);
				debugString += "\n";
	
				debugPrint(debugString);
			}
		}

		delete engineEvent;
	}
}

void CXEngine::shutdownEngineEvents()
{
	// todo: release any memory used by the engine event queue
}

// ********* entities ********* 

void CXEngine::initEntities()
{
	// todo: perform any entity system initialization nessecary
}

void CXEngine::updateEntities()
{
	// todo: feels like this should be longer

	for(list<CEntity*>::iterator iEntity = CEntity::entities.begin(); iEntity != CEntity::entities.end(); iEntity++)
	{	
		CEntity* entity = (*iEntity);

		entity->update();
	}

	for(list<CEntity*>::iterator iPhysicalEntity = CPhysicalEntity::physicalEntities.begin(); iPhysicalEntity != CPhysicalEntity::physicalEntities.end(); iPhysicalEntity++)
	{	
		CPhysicalEntity* physicalEntity = (CPhysicalEntity*)(*iPhysicalEntity);

		physicalEntity->physicalUpdate();
	}
}

void CXEngine::shutdownEntities()
{
	// todo: delete all entities, clear all entity lists
}

// ********* resources ********* 

void CXEngine::initResources()
{
	// find all the shaders availible to the engine and stores them in our shader info list

	debugPrint("Finding shader resources...\n");

	WIN32_FIND_DATA findData;
	
	string shaderPath = XENGINE_RESOURCES_MEDIA_PATH;
	shaderPath += XENGINE_RESOURCES_SHADER_PATH;

	string searchPath = shaderPath; searchPath += "*.shader";
	HANDLE shaderSearch = FindFirstFile(searchPath.c_str(), &findData);

	while(shaderSearch != INVALID_HANDLE_VALUE)
	{	
		string shaderFilePath = shaderPath;
		shaderFilePath += findData.cFileName;

		ifstream shaderFile(shaderFilePath.c_str());

		if (!shaderFile.fail())
		{
			string debugString = "Reading '";

			debugString += shaderFilePath;
			debugString += "'...\n";

			debugPrint(debugString);

			int depth = 0;
			int lineNum = 0;

			string currentShaderName = "";
			SHADER_INFO *shaderInfo = NULL;

			while (!shaderFile.eof())
			{
				string line;

				if (getline(shaderFile, line, '\n'))
				{
					vector<string> *tmp = tokenize(line);

					vector<string> tokens = (*tmp);

					delete tmp;

					switch (depth)
					{
					case 0:

						if (tokens.size())
						{
							if (tokens.size() == 1)
							{
								if (tokens[0] == "{")
								{
									if (currentShaderName != "")
									{
										depth++;
									}
									else
									{
										string debugString = "*** ERROR *** Error in shader file '";

										debugString += shaderFilePath;
										debugString += "' on line ";
										debugString += intString(lineNum);
										debugString += ". Expected shader name, found '{'.\n";

										debugPrint(debugString);
									}
								}
								else
								{
									shaderInfo = new SHADER_INFO;

									shaderInfo->shaderName = correctPathSlashes(tokens[0]);

									shaderInfo->shaderFile = shaderFilePath;
									shaderInfo->fileOffset = shaderFile.tellg();

									currentShaderName = tokens[0];
								}
							}
							else
							{
								string debugString = "*** ERROR *** Error in shader file '";

								debugString += shaderFilePath;
								debugString += "' on line ";
								debugString += intString(lineNum);
								debugString += ". Incorrect number of parameters for shader name.\n";

								debugPrint(debugString);
							}
						}

						break;

					case 1:

						if (tokens.size())
						{
							if (tokens.size() == 1)
							{
								if (tokens[0] == "{")
								{
									depth++;
								}
								else if (tokens[0] == "}")
								{
									// todo: check if we received any meaningful information the shader body
									// if not, ERROR

									depth--;

									// we're done with this particular shader

									if (shaderInfo != NULL)
									{
										shaderInfos.push_back(shaderInfo);
									}

									currentShaderName = "";
								}
								else
								{
									// could be a valid 1 token statement, either way, here we're not concerned
								}
							}
							else
							{
								// could be all sorts of multi-token valid statements. we're not concerned.
							}
						}

						break;

					case 2:

						if (tokens.size())
						{
							if (tokens.size() == 1)
							{
								if (tokens[0] == "{")
								{
									string debugString = "*** ERROR *** Error in shader file '";

									debugString += shaderFilePath;
									debugString += "' on line ";
									debugString += intString(lineNum);
									debugString += ". Expected '}' or shader stage info. Found new block '{'.\n";

									debugPrint(debugString);

									if (shaderInfo)
									{
										delete shaderInfo;
										shaderInfo = NULL;

										currentShaderName = "";
									}
								}
								else if (tokens[0] == "}")
								{
									// todo: check if we received any meaningful information in this shader stage
									// if not, ERROR

									depth--;
								}
								else
								{
									// could be a valid 1 token statement, either way, here we're not concerned
								}
							}
							else
							{
								// could be all sorts of multi-token valid statements. we're not concerned.
							}
						}

						break;
					}

					lineNum++;
				}
				else
				{
					break;
				}
			}

			shaderFile.close();
		}
		else
		{
			string debugString = "*** ERROR *** Failed to open shader file '";

			debugString += shaderFilePath;
			debugString += "'\n";

			debugPrint(debugString);
		}

		if (!FindNextFile(shaderSearch, &findData)) break;
	}

	FindClose(shaderSearch);
}

void CXEngine::beginLoading()
{
	direct3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

	direct3DDevice->BeginScene();

	debugRenderText("Loading...", 500, 400);

	direct3DDevice->EndScene();

	direct3DDevice->Present(NULL, NULL, NULL, NULL);
}

int _cdecl cacheResourceSort(const VOID* arg1, const VOID* arg2)
{
    CCacheResource* r1 = *((CCacheResource**)arg1);
    CCacheResource* r2 = *((CCacheResource**)arg2);

	// todo: put non-unloadable resources at the end of the list

	if (r1->getLastReferenceTime() < r2->getLastReferenceTime())
	{
		return -1;
	}
	else
	{
		return +1;
	}
}

void CXEngine::reserveCacheResourceMemory()
{
	MEMORYSTATUS memoryStatus;

	GlobalMemoryStatus(&memoryStatus);

	if (memoryStatus.dwAvailPhys >= XENGINE_RESOURCE_MINIMUM_FREE_MEMORY)
	{
		return;
	}

	// todo: it's unpleasant, but that's the way things go, unless I substitute the entire cacheResources
	// main list for a static pointer allocation, which would unfortunately limit me to a max number of cache resources

	int numCacheResources = (int)CCacheResource::cacheResources.size();
	CCacheResource **cacheResources = new CCacheResource*[numCacheResources];

	for (int r = 0; r < numCacheResources; r++)
	{
		cacheResources[r] = (CCacheResource*)CCacheResource::cacheResources[r];
	}

	qsort(cacheResources, numCacheResources, sizeof(CCacheResource*), cacheResourceSort);
	
	int i = 0;
	
	int tmpSize = numCacheResources << 2;

	while (memoryStatus.dwAvailPhys + tmpSize < XENGINE_RESOURCE_MINIMUM_FREE_MEMORY &&
		   i < numCacheResources)
	{
		CCacheResource *cacheResource = cacheResources[i];

		if (cacheResource->isUnloadable())
		{
			cacheResource->unload();

			GlobalMemoryStatus(&memoryStatus);
		}

		i++;
	}
	
	delete [] cacheResources;
}
void CXEngine::validatePath(string path)
{
	int slashPos = path.find('\\', 3);

	while (slashPos > 0)
	{
		string dirPath = path.substr(0, slashPos);

		CreateDirectory(dirPath.c_str(), NULL);

		slashPos = path.find('\\', slashPos + 1);
	}
}

void CXEngine::updateResources()
{
	//XEngine->reserveCacheResourceMemory();

	// todo: any per frame resource system updates
}

void CXEngine::shutdownResources()
{
	//todo: release any resources still in memory
}

// ********* audio ********* 

IDirectSound8 *CXEngine::getDirectSound()
{
	return directSound;
}

void CXEngine::initAudio()
{
    if(FAILED(DirectSoundCreate(NULL, &directSound, NULL)))
	{
        debugPrint("*** ERROR *** Error initializing DirectSound!\n");

		directSound = NULL;

		return;
	}

	DirectSoundUseFullHRTF();

	directSound->SetDopplerFactor(XENGINE_AUDIO_DOPPLER_FACTOR, DS3D_IMMEDIATE);
	directSound->SetDistanceFactor(XENGINE_AUDIO_DISTANCE_FACTOR, DS3D_IMMEDIATE);
	directSound->SetRolloffFactor(XENGINE_AUDIO_ROLLOFF_FACTOR, DS3D_IMMEDIATE);

	directSound->SetPosition(0.0f, 0.0f, 0.0f, DS3D_IMMEDIATE);
	directSound->SetVelocity(0.0f, 0.0f, 0.0f, DS3D_IMMEDIATE);

	directSound->SetOrientation(0.0f, 0.0f, 1.0f,
								0.0f, 1.0f, 0.0f, DS3D_IMMEDIATE);

	DSEFFECTIMAGELOC effectLoc;

	effectLoc.dwI3DL2ReverbIndex = I3DL2_CHAIN_I3DL2_REVERB;
	effectLoc.dwCrosstalkIndex   = I3DL2_CHAIN_XTALK;

	if(FAILED(XAudioDownloadEffectsImage("d:\\dsstdfx.bin", &effectLoc, XAUDIO_DOWNLOADFX_EXTERNFILE, NULL)))
	{
		debugPrint("*** ERROR *** Error loading dsstdfx.bin!\n");

		directSound->Release();
		directSound = NULL;

        return;
	}

	listener = NULL;
}

void CXEngine::set3DListener(class CPhysicalEntity *listener)
{
	if (this->listener)
	{
		this->listener->release();
	}

	this->listener = listener;

	listener->addRef();
}

CPhysicalEntity *CXEngine::get3DListener()
{
	return listener;
}

void CXEngine::updateAudio()
{
	// TODO: some sort of mechanism needs to be in place in case of a split screen game with multiple listeners

	if (listener)
	{
		D3DXVECTOR3 listenerPosition;
		D3DXVECTOR3 listenerVelocity;
		
		D3DXVECTOR3 listenerForwardDirection;
		D3DXVECTOR3 listenerUpDirection = D3DXVECTOR3(0.0f, 1.0f, 0.0f);

		listener->getPosition(&listenerPosition);
		listener->getVelocity(&listenerVelocity);

		listener->getDirection(&listenerForwardDirection);
		listener->getWorldDirection(&listenerUpDirection, &listenerUpDirection);

		directSound->SetPosition(listenerPosition.x, listenerPosition.y, listenerPosition.z, DS3D_DEFERRED);
		directSound->SetVelocity(listenerVelocity.x, listenerVelocity.y, listenerVelocity.z, DS3D_DEFERRED);

		directSound->SetOrientation(listenerForwardDirection.x, listenerForwardDirection.y, listenerForwardDirection.z,
									listenerUpDirection.x, listenerUpDirection.y, listenerUpDirection.z, DS3D_DEFERRED);

		/*
		if (CBSPResource::world)
		{
			int listenerCluster = listener->getCurrentCluster();

			for(list<PHYSICALENTITY_CLUSTER_RESIDENCE_INFO*>::iterator iResidence = listener->clusterResidence.begin();
				iResidence != listener->clusterResidence.end(); iResidence++)
			{	
				PHYSICALENTITY_CLUSTER_RESIDENCE_INFO* residence = (*iResidence);

				for(list<CEntity*>::iterator iPhysicalEntity = CBSPResource::world->bspClusters[listenerCluster].entities.begin();
					iPhysicalEntity != CBSPResource::world->bspClusters[listenerCluster].entities.end(); iPhysicalEntity++)
				{	
					CPhysicalEntity* physicalEntity = (CPhysicalEntity*)(*iPhysicalEntity);

					if (physicalEntity->isEntityA(ET_SOUND3D_ENTITY))
					{
						CSound3DEntity *soundEntity = (CSound3DEntity*)physicalEntity;

						if (!soundEntity->isActivated())
						{
		*/
	}

	directSound->CommitDeferredSettings();

	DirectSoundDoWork();
}

void CXEngine::shutdownAudio()
{
	directSound->Release();
}
