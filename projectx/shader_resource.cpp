#include "shader_resource.h"

#include "texture_resource.h"
#include "bsp_resource.h"
#include "resource_types.h"
#include "xengine.h"
#include "string_help.h"
#include "math_help.h"

vector<CResource*> CShaderResource::shaderResources;

int CShaderResource::getResourceType()
{
	return RT_SHADER_RESOURCE;
}

int CShaderResource::getShaderType()
{
	return shaderType;
}

int CShaderResource::getNumStages()
{
	return numStages;
}

// first and foremost, checks if any existing loading shaders
// match the name of the requested shader. If found, return
// that shader resource and allow the requester to make a reference to it.
// if it isn't found, check the SHADER_INFOs. if found in the shader infos,
// create and compile the shader, load and create the appropriate texture
// resources for it, and be all happy. If it ISN'T found in the shader_infos,
// check return a reference to either the lightmapped or non-lightmapped default
// shaders.

CShaderResource *CShaderResource::getShader(string shaderName)
{
	int extensionPosition = shaderName.find('.');
	
	string extensionless = shaderName;

	if (extensionPosition > 0)
	{
		extensionless = shaderName.substr(0, extensionPosition);
	}

	// check existing and loaded shaders

	for (int i = 0; i < (int)CShaderResource::shaderResources.size(); i++)
	{
		CShaderResource *shaderResource = (CShaderResource*)CShaderResource::shaderResources[i];

		if (shaderResource->getResourceName() == shaderName ||
			shaderResource->getResourceName() == extensionless)
		{
			shaderResource->addRef();

			return shaderResource;
		}
	}

	// check shader database

	for (int i = 0; i < (int)XEngine->shaderInfos.size(); i++)
	{
		SHADER_INFO *shaderInfo = (SHADER_INFO*)XEngine->shaderInfos[i];

		if (shaderInfo->shaderName == shaderName ||
			shaderInfo->shaderName == extensionless)
		{
			CShaderResource *shaderResource = new CShaderResource;

			shaderResource->create(shaderInfo->shaderName, shaderInfo);

			return shaderResource;
		}
	}

	return NULL;
}

void CShaderResource::getSkyBoxInfo(SHADER_SKYBOX_INFO *skyBoxInfo)
{
	skyBoxInfo->nearBox = &nearBox;
	skyBoxInfo->farBox  = &farBox;

	skyBoxInfo->hasNearBox = hasNearBox;
	skyBoxInfo->hasFarBox  = hasFarBox;
}

float CShaderResource::evaluateWave(SHADER_WAVE_INFO *wave)
{
	if (!wave)
	{
		return 0.0f;
	}

	// todo: hm, not sure if base is supposed to represent the min value or the center of the wave. for now I'll assume it's the min value
	// for everything but sin and see how things go

	switch(wave->waveType)
	{
	case WT_SIN:

		{
			float funcPhase = XEngine->getTicks() / 60.0f;
			float funcVal   = (float)sin(D3DXToRadian((funcPhase * 360.0f ) * wave->frequency + wave->phase * 360.0f)) * wave->amplitude + wave->base;

			return funcVal;
		}
		
	case WT_TRIANGLE:

		{
			float funcPhase = fMod(XEngine->getTicks() / 60.0f * wave->frequency + wave->phase, 1.0f);
			float funcVal;

			if (funcPhase < 0.5f)
			{
				funcVal = funcPhase * 2.0f;
			}
			else
			{
				funcVal = 1.0f - (funcPhase - 0.5f) * 2.0f;
			}
			
			funcVal = funcVal * wave->amplitude + wave->base;

			return funcVal;
		}

	case WT_SQUARE:

		{
			float funcPhase = fMod(XEngine->getTicks() / 60.0f * wave->frequency + wave->phase, 1.0f);
			float funcVal;

			if (funcPhase < 0.5f)
			{
				funcVal = -1.0f;
			}
			else
			{
				funcVal = +1.0f;
			}
			
			funcVal = funcVal * wave->amplitude + wave->base;

			return funcVal;
		}

	case WT_SAWTOOTH:

		{
			float funcPhase = fMod(XEngine->getTicks() / 60.0f * wave->frequency + wave->phase, 1.0f);
			float funcVal =  funcPhase;
			
			funcVal = funcVal * wave->amplitude + wave->base;

			return funcVal;
		}

	case WT_INVERSESAWTOOTH:

		{
			float funcPhase = fMod(XEngine->getTicks() / 60.0f * wave->frequency + wave->phase, 1.0f);
			float funcVal = 1.0f - funcPhase;
			
			funcVal = funcVal * wave->amplitude + wave->base;

			return funcVal;
		}

	default:

		return 0.0f;
	}
}

void CShaderResource::activate()
{
	_activate(NULL, 0xFFFFFFFF);
}

void CShaderResource::activate(CTextureResource *lightmap)
{
	_activate(lightmap, 0xFFFFFFFF);
}

void CShaderResource::activate(D3DCOLOR entityColor)
{
	_activate(NULL, entityColor);
}

void CShaderResource::update()
{
	if (hasMovement)
	{
		D3DXMatrixIdentity(&movementMatrix);
	}

	for (int d = numVertexDeformations - 1; d >= 0; d--)
	{
		// todo: support other vertex deformation types

		D3DXMATRIX tmpMatrix;

		switch (vertexDeformations[d].type)
		{
		case VDT_MOVE:

			{
				float amount = evaluateWave(&vertexDeformations[d].wave);

				D3DXMatrixTranslation(&tmpMatrix, amount * vertexDeformations[d].x,
												  amount * vertexDeformations[d].y,
												  amount * vertexDeformations[d].z);

				D3DXMatrixMultiply(&movementMatrix, &movementMatrix, &tmpMatrix);
			}

			break;
		}
	}

	for (int s = 0; s < numStages; s++)
	{
		if (stages[s].numTcMods)
		{
			D3DXMatrixIdentity(&stages[s].tcMatrix);
	
			stages[s].tcMatrix(2, 0) = -0.5f;
			stages[s].tcMatrix(2, 1) = -0.5f;
		}

		for (int i = stages[s].numTcMods - 1; i >= 0; i--)
		//for (int i = 0 ; i < stages[s].numTcMods; i++)
		{
			switch (stages[s].tcMods[i].tcModType)
			{
			case STCMT_SCALE:
			case STCMT_TRANSFORM:

				{
					D3DXMatrixMultiply(&stages[s].tcMatrix, &stages[s].tcMatrix, &stages[s].tcMods[i].matrix);
				}

				break;

			case STCMT_ROTATE:

				{
					D3DXMatrixRotationZ(&stages[s].tcMods[i].matrix, D3DXToRadian(XEngine->getTicks() / 60.0f * stages[s].tcMods[i].rotSpeed));
					D3DXMatrixMultiply(&stages[s].tcMatrix, &stages[s].tcMatrix, &stages[s].tcMods[i].matrix);
				}

				break;

			case STCMT_SCROLL:

				{
					float scrollU = fMod(XEngine->getTicks() / 60.0f * stages[s].tcMods[i].scrollSpeedU, 1.0f);
					float scrollV = fMod(XEngine->getTicks() / 60.0f * stages[s].tcMods[i].scrollSpeedV, 1.0f);

					D3DXMatrixIdentity(&stages[s].tcMods[i].matrix);

					stages[s].tcMods[i].matrix(2, 0) = scrollU;
					stages[s].tcMods[i].matrix(2, 1) = scrollV;

					D3DXMatrixMultiply(&stages[s].tcMatrix, &stages[s].tcMatrix, &stages[s].tcMods[i].matrix);
				}

				break;

			case STCMT_STRETCH:

				{
					float scale = evaluateWave(&stages[s].tcMods[i].wave);

					D3DXMatrixScaling(&stages[s].tcMods[i].matrix, scale, scale, 1.0f);
					D3DXMatrixMultiply(&stages[s].tcMatrix, &stages[s].tcMatrix, &stages[s].tcMods[i].matrix);
				}

				break;

			case STCMT_TURB:
				
				{
					// todo: this is entirely empirical due to really poor documentation. hopefully
					// it's good enough. arbitrary constants may need adjusting

					stages[s].tcMods[i].wave.phase = 0.0f;  float turbulence1 = evaluateWave(&stages[s].tcMods[i].wave);
					stages[s].tcMods[i].wave.phase = 0.25f; float turbulence2 = evaluateWave(&stages[s].tcMods[i].wave);
					
					D3DXMatrixIdentity(&stages[s].tcMods[i].matrix);

					stages[s].tcMods[i].matrix(2, 0) = turbulence1 * 0.25f;
					stages[s].tcMods[i].matrix(2, 1) = turbulence2 * 0.25f;

					D3DXMatrixMultiply(&stages[s].tcMatrix, &stages[s].tcMatrix, &stages[s].tcMods[i].matrix);

					D3DXMatrixScaling(&stages[s].tcMods[i].matrix, turbulence2 * 0.1f + 1.0f, turbulence1 * 0.1f + 1.0f, 1.0f);
					D3DXMatrixMultiply(&stages[s].tcMatrix, &stages[s].tcMatrix, &stages[s].tcMods[i].matrix);
				}

				break;
			}
		}
		
		if (stages[s].numTcMods)
		{
			D3DXMATRIX correctionMatrix;

			D3DXMatrixIdentity(&correctionMatrix);
			
			correctionMatrix(2, 0) = +0.5f;
			correctionMatrix(2, 1) = +0.5f;
	
			D3DXMatrixMultiply(&stages[s].tcMatrix, &stages[s].tcMatrix, &correctionMatrix);
		}

		if (stages[s].rgbGenType == SRG_WAVE)
		{
			int val = int(evaluateWave(&stages[s].rgbGenWave) * 255.0f);

			if (val > 255) val = 255;
			if (val < 0) val = 0;

			stages[s].color = (stages[s].color & 0xFF000000) | val | (val << 8) | (val << 16);
		}

		if (stages[s].alphaGenType == SRG_WAVE)
		{
			int val = int(evaluateWave(&stages[s].alphaGenWave) * 255.0f);

			if (val > 255) val = 255;
			if (val < 0) val = 0;

			stages[s].color = (stages[s].color & 0x00FFFFFF) | (val << 24);
		}
	}

	
}

void CShaderResource::setLightmap(CTextureResource *lightmap)
{
	IDirect3DTexture8 *newLightmap;

	if (lightmap)
	{
		newLightmap = lightmap->getTexture2D();
	}
	else
	{
		newLightmap = XEngine->getWhiteTexture();
	}

	for (int s = 0; s < numStages; s++)
	{
		if (stages[s].texCoordSource == STS_LIGHTMAP)
		{
			IDirect3DTexture8 *oldLightmap; device->GetTexture(s, (IDirect3DBaseTexture8**)&oldLightmap);
			if (oldLightmap) oldLightmap->Release();

			if (oldLightmap != newLightmap)
			{
				device->SetTexture(s, newLightmap);
			}
		}
	}
}

void CShaderResource::_activate(CTextureResource *lightmap, D3DCOLOR entityColor)
{
	if (lastFrameRendered < XEngine->getRenderFrame())
	{
		update();

		lastFrameRendered = XEngine->getRenderFrame();
	}

	if (cullMode != D3DCULL_CCW)
	{
		device->SetRenderState(D3DRS_CULLMODE, cullMode);
	}

	if (!depthWrite)
	{
		device->SetRenderState(D3DRS_ZWRITEENABLE, false);
	}

	if (shaderType == ST_BLENDED)
	{
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		device->SetRenderState(D3DRS_ALPHAFUNC, alphaFunc);

		device->SetRenderState(D3DRS_SRCBLEND, srcBlend);
		device->SetRenderState(D3DRS_DESTBLEND, dstBlend);

		device->SetRenderState(D3DRS_ALPHATESTENABLE, true);
		device->SetRenderState(D3DRS_ALPHAREF, alphaRef);
	}
	else if (alphaTestEnable)
	{
		device->SetRenderState(D3DRS_ALPHAREF, alphaRef);
		device->SetRenderState(D3DRS_ALPHATESTENABLE, true);
	}

	if (hasMovement)
	{
		device->SetTransform(D3DTS_WORLD, &movementMatrix);
	}

	if (!numStages)
	{
		for (int s = 0; s < 2; s++)
		{
			// todo: could cause lots of redundancy

			device->SetTexture(s, NULL);
		}
	}

	for (int s = 0; s < numStages; s++)
	{
		if (stages[s].texCoordSource == STS_LIGHTMAP)
		{
			if (lightmap)
			{
				device->SetTexture(s, lightmap->getTexture2D());
			}
			else
			{
				device->SetTexture(s, XEngine->getWhiteTexture());
			}
		}
		else
		{
			if (stages[s].textures) // this check shouldn't need to be here either
			{
				int texNum;

				if (stages[s].isAnimMap)
				{
					texNum = int(XEngine->getTicks() / 60.0f * stages[s].animFrequency) % stages[s].numTextures;
				}
				else
				{
					texNum = 0;
				}

				if (stages[s].textures[texNum]) // this check shouldn't need to be here
				{
					stages[s].textures[texNum]->acquire(true);

					device->SetTexture(s, stages[s].textures[texNum]->getTexture2D());
				}
				else
				{
					device->SetTexture(s, XEngine->getWhiteTexture());
				}
			}
		}

		switch (stages[s].texCoordSource)
		{
		case STS_BASE:

			device->SetTextureStageState(s, D3DTSS_TEXCOORDINDEX, 0);
			break;

		case STS_LIGHTMAP:

			device->SetTextureStageState(s, D3DTSS_TEXCOORDINDEX, 1);
			break;

		case STS_ENVIRONMENT:

			{
				// todo: environment mapped stages with tcmods need to work correctly

				/*
				D3DXMATRIX mat;

				mat._11 = 0.5f; mat._12 = 0.0f;
				mat._21 = 0.0f; mat._22 =-0.5f;
				mat._31 = 0.0f; mat._32 = 0.0f;
				mat._41 = 0.5f; mat._42 = 0.5f;

				D3DTRANSFORMSTATETYPE transformState = D3DTRANSFORMSTATETYPE(int(D3DTS_TEXTURE0) + s);

				device->SetTransform(transformState, &mat);
				device->SetTextureStageState(s, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
				*/

				//device->SetTextureStageState(s, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL);
				device->SetTextureStageState(s, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
			}
	
			break;
		}

		if (stages[s].isClamped)
		{
			device->SetTextureStageState(s, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
			device->SetTextureStageState(s, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
		}

		if (stages[s].numTcMods)
		{
			D3DTRANSFORMSTATETYPE transformState = D3DTRANSFORMSTATETYPE(int(D3DTS_TEXTURE0) + s);

			device->SetTransform(transformState, &stages[s].tcMatrix);
			device->SetTextureStageState(s, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
		}
	}

	device->SetPixelShader(pixelShader);

	for (int s = 0; s < numStages; s++)
	{
		if (stages[s].rgbGenType >= SRG_WAVE ||stages[s].alphaGenType >= SRG_WAVE)
		{
			DWORD color = stages[s].color;

			// as odd as this looks the inversion for one_minus_entity is done in the shader microcode,
			// slightly faster this way.

			if (stages[s].rgbGenType == SRG_ENTITY || stages[s].rgbGenType == SRG_ONE_MINUS_ENTITY)
			{
				color = (color & 0xFF000000) | (entityColor & 0x00FFFFFF);
			}
			
			if (stages[s].alphaGenType == SRG_ENTITY || stages[s].alphaGenType == SRG_ONE_MINUS_ENTITY)
			{
				color = (color & 0x00FFFFFF) | (entityColor & 0xFF000000);
			}

			D3DRENDERSTATETYPE psRegister = D3DRENDERSTATETYPE(int(D3DRS_PSCONSTANT0_0) + stages[s].combinerStage);

			device->SetRenderStateNotInline(psRegister, color);
		}
	}
}

void CShaderResource::deactivate()
{
	if (cullMode != D3DCULL_CCW)
	{
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	}

	if (!depthWrite)
	{
		device->SetRenderState(D3DRS_ZWRITEENABLE, true);
	}

	if (shaderType == ST_BLENDED)
	{
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
		device->SetRenderState(D3DRS_ALPHATESTENABLE, false);
	}
	else if (alphaTestEnable)
	{
		device->SetRenderState(D3DRS_ALPHATESTENABLE, false);
	}

	if (hasMovement)
	{
		D3DXMatrixIdentity(&movementMatrix);

		device->SetTransform(D3DTS_WORLD, &movementMatrix);
	}

	device->SetPixelShader(NULL);

	for (int s = 0; s < numStages; s++)
	{
		if (stages[s].textures)
		{
			int texNum;

			if (stages[s].isAnimMap)
			{
				texNum = int(XEngine->getTicks() / 60.0f * stages[s].animFrequency) % stages[s].numTextures;
			}
			else
			{
				texNum = 0;
			}

			if (stages[s].textures[texNum])
			{
				stages[s].textures[texNum]->drop();
			}
		}

		device->SetTexture(s, NULL);

		if (stages[s].isClamped)
		{
			device->SetTextureStageState(s, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
			device->SetTextureStageState(s, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
		}

		if (stages[s].numTcMods || stages[s].texCoordSource == STS_ENVIRONMENT)
		{
			// todo: furthermore, environment mapped surfaces should turn off the gen thing

			device->SetTextureStageState(s, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
		}
	}

	device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	device->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
}

void CShaderResource::create(string resourceName, SHADER_INFO *shaderInfo)
{
	device = XEngine->getD3DDevice();

	// set surface defaults

	lastFrameRendered = 0;

	shaderType = ST_OPAQUE;

	cullMode	 = D3DCULL_CCW;
	sortPriority = SSP_OPAQUE;

	depthFunc = D3DCMP_LESSEQUAL;
	depthWrite = true;

	alphaTestEnable = false;
	alphaFunc = D3DCMP_GREATER;
	alphaRef  = 0;

	hasNearBox = false;
	hasFarBox  = false;

	ZeroMemory(&farBox, sizeof(farBox));
	ZeroMemory(&nearBox, sizeof(nearBox));

	skyBoxCurvature = 128.0f;

	vector<TSHADER_STAGE*> tmpShaderStages;
	int currentStage = -1;

	_isDynamic = false;

	hasMovement = false;
	vector<DEFORMVERTEXES_INFO> vertexDeformations;

	// compile shader

	bool forceDepthWrite;  // shaders with a top level stage that has a blend func normally doesn't do depth writes

	ifstream shaderFile(shaderInfo->shaderFile.c_str());

	if (!shaderFile.fail())
	{
		string debugString = "Compiling shader '";

		debugString += resourceName;
		debugString += "'...\n";

		XEngine->debugPrint(debugString);

		shaderFile.seekg(shaderInfo->fileOffset);

		int depth = 0;

		bool isDone = false;

		while (!shaderFile.eof() && !isDone)
		{
			string line;

			if (getline(shaderFile, line, '\n'))
			{
				line = lowerCaseString(line);

				// todo: force line to lowercase

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
								depth++;
							}
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

								if (tmpShaderStages.size() < 4)
								{
									TSHADER_STAGE *shaderStage = new TSHADER_STAGE;

									ZeroMemory(shaderStage, sizeof(TSHADER_STAGE));
									
									shaderStage->constantColor = 0xFFFFFFFF;

									// todo: set any appropriate default stage states

									tmpShaderStages.push_back(shaderStage);	
								}
								else
								{
									// exceeded maximum number of stages that could be accomplished in a single
									// pass. for now, the shader compilation will ignore stages past this point
								}

								currentStage++;
							}
							else if (tokens[0] == "}")
							{
								// todo: check if we received any meaningful information the shader body
								// if not, ERROR

								depth--;

								// we're done compiling this shader

								isDone = true;
							}

							// single token shader statements
						}
						else
						{
							// could be all sorts of multi-token valid statements.

							if (tokens[0] == "cull")
							{
								if (tokens.size() != 2)
								{
									// error
								}
								else
								{
									if (tokens[1] == "front")
									{
										cullMode = D3DCULL_CW;
									}
									else if (tokens[1] == "back")
									{
										cullMode = D3DCULL_CCW;
									}
									else if (tokens[1] == "disable" || tokens[1] == "none")
									{
										cullMode = D3DCULL_NONE;
									}
									else
									{
										// error
									}
								}
							}
							else if (tokens[0] == "sort")
							{
								if (tokens.size() != 2)
								{
									// error
								}
								else
								{
									if (tokens[1] == "portal")
									{
										sortPriority = 1;
									}
									else if (tokens[1] == "sky")
									{
										sortPriority = 2;
									}
									else if (tokens[1] == "opaque")

									{
										sortPriority = 3;
									}
									else if (tokens[1] == "banner")
									{
										sortPriority = 6;
									}
									else if (tokens[1] == "underwater")
									{
										sortPriority = 8;
									}
									else if (tokens[1] == "additive")
									{
										sortPriority = 9;
									}
									else if (tokens[1] == "nearest")
									{
										sortPriority = 16;
									}
									else
									{
										// error
									}
								}
							}
							else if (tokens[0] == "skyparms")
							{
								if (!hasNearBox && !hasFarBox && tokens.size() == 4)
								{
									string farBoxPath = tokens[1];

									if (farBoxPath == "-")
									{
										hasFarBox = false;
									}
									else
									{
										hasFarBox = true;

										farBoxPath = correctPathSlashes(farBoxPath);

										string leftPath  = farBoxPath + "_lf";
										string rightPath = farBoxPath + "_rt";
										string backPath  = farBoxPath + "_bk";
										string frontPath = farBoxPath + "_ft";
										string upPath    = farBoxPath + "_up";
										string downPath  = farBoxPath + "_dn";

										farBox.left  = CTextureResource::getTexture(leftPath);
										farBox.right = CTextureResource::getTexture(rightPath);
										farBox.back  = CTextureResource::getTexture(backPath);
										farBox.front = CTextureResource::getTexture(frontPath);
										farBox.up    = CTextureResource::getTexture(upPath);
										farBox.down  = CTextureResource::getTexture(downPath);
									}

									if (tokens[2] == "-")
									{
										skyBoxCurvature = 128.0f;
									}
									else
									{
										skyBoxCurvature = (float)atof((const char *)tokens[2].c_str());
									}

									string nearBoxPath = tokens[3];

									if (nearBoxPath == "-")
									{
										hasNearBox = false;
									}
									else
									{
										hasNearBox = true;

										nearBoxPath = correctPathSlashes(nearBoxPath);

										string leftPath  = nearBoxPath + "_lf";
										string rightPath = nearBoxPath + "_rt";
										string backPath  = nearBoxPath + "_bk";
										string frontPath = nearBoxPath + "_ft";
										string upPath    = nearBoxPath + "_up";
										string downPath  = nearBoxPath + "_dn";

										nearBox.left  = CTextureResource::getTexture(leftPath);
										nearBox.right = CTextureResource::getTexture(rightPath);
										nearBox.back  = CTextureResource::getTexture(backPath);
										nearBox.front = CTextureResource::getTexture(frontPath);
										nearBox.up    = CTextureResource::getTexture(upPath);
										nearBox.down  = CTextureResource::getTexture(downPath);
									}

									shaderType = ST_SKYBOX;
								}
								else
								{
									// error
								}
							}
							else if (tokens[0] == "deformvertexes")
							{
								if (tokens[1] == "move")
								{
									if (tokens.size() != 10)
									{
										// error
									}
									else
									{
										hasMovement = true;

										DEFORMVERTEXES_INFO vertexDeformation;

										vertexDeformation.type = VDT_MOVE;

										vertexDeformation.x = -(float)atof(tokens[2].c_str());
										vertexDeformation.y = +(float)atof(tokens[4].c_str());
										vertexDeformation.z = -(float)atof(tokens[3].c_str());

										string func = tokens[5];

										if (func == "sin")
										{
											vertexDeformation.wave.waveType = WT_SIN;
										}
										else if (func == "triangle")
										{
											vertexDeformation.wave.waveType = WT_TRIANGLE;
										}
										else if (func == "square")
										{
											vertexDeformation.wave.waveType = WT_SQUARE;
										}
										else if (func == "sawtooth")
										{
											vertexDeformation.wave.waveType = WT_SAWTOOTH;
										}
										else if (func == "inversesawtooth")
										{
											vertexDeformation.wave.waveType = WT_INVERSESAWTOOTH;
										}
										else
										{
											break;
											// error
										}

										vertexDeformation.wave.base		 = (float)atof(tokens[6].c_str());
										vertexDeformation.wave.amplitude = (float)atof(tokens[7].c_str());
										vertexDeformation.wave.phase	 = (float)atof(tokens[8].c_str());
										vertexDeformation.wave.frequency = (float)atof(tokens[9].c_str());

										vertexDeformations.push_back(vertexDeformation);
									}
								}
							}
								// todo: support other vertex deformation types here


							// multi-token shader statements
						}
					}

					break;

				case 2:

					if (tokens.size() && currentStage < 4)
					{
						if (tokens.size() == 1)
						{
							if (tokens[0] == "}")
							{
								// todo: check if we received any meaningful information in this shader stage
								// if not, ERROR

								depth--;
							}
							else
							{
								if (tokens[0] == "depthwrite")
								{
									forceDepthWrite = true;
								}
								else if (tokens[0] == "detail")
								{
									// ignored
								}
								else if (!tmpShaderStages[currentStage]->isAnimMap)
								{
									// error
								}
								else
								{
									string texturePath = correctPathSlashes(tokens[0]);

									tmpShaderStages[currentStage]->texturePaths.push_back(texturePath);
								}

								// single token stage statements
							}
						}
						else
						{
							if (tokens[0] == "map")
							{
								if (tokens.size() != 2 || tmpShaderStages[currentStage]->texturePaths.size() > 0)
								{
									// error
								}
								else
								{
									if (tokens[1] == "$lightmap")
									{
										tmpShaderStages[currentStage]->texCoordSource = STS_LIGHTMAP;

										//tmpShaderStages[currentStage]->texturePaths.push_back("$lightmap");
									}
									else
									{
										string texturePath = correctPathSlashes(tokens[1]);
	
										tmpShaderStages[currentStage]->texturePaths.push_back(texturePath);
									}
								}
							}
							else if (tokens[0] == "animmap")
							{
								if (tokens.size() < 3)
								{
									// error
								}
								else
								{
									tmpShaderStages[currentStage]->animFrequency = (float)atof((const char *)tokens[1].c_str());

									for (int i = 2; i < (int)tokens.size(); i++)
									{
										string texturePath = correctPathSlashes(tokens[i]);

										tmpShaderStages[currentStage]->texturePaths.push_back(texturePath);
									}

									tmpShaderStages[currentStage]->isAnimMap = true;
								}
							}
							else if (tokens[0] == "clampmap")
							{
								if (tokens.size() != 2 || tmpShaderStages[currentStage]->texturePaths.size() > 0)
								{
									// error
								}
								else
								{
									string texturePath = correctPathSlashes(tokens[1]);

									tmpShaderStages[currentStage]->texturePaths.push_back(texturePath);

									tmpShaderStages[currentStage]->isClamped = true;
								}
							}
							else if (tokens[0] == "blendfunc")
							{
								if (tokens.size() == 2)
								{
									if (tokens[1] == "add")
									{
										tmpShaderStages[currentStage]->srcBlend = SBA_ONE;
										tmpShaderStages[currentStage]->dstBlend = SBA_ONE;
									}
									else if (tokens[1] == "filter")
									{
										tmpShaderStages[currentStage]->srcBlend = SBA_DST_COLOR;
										tmpShaderStages[currentStage]->dstBlend = SBA_ZERO;
									}
									else if (tokens[1] == "blend")
									{
										tmpShaderStages[currentStage]->srcBlend = SBA_SRC_ALPHA;
										tmpShaderStages[currentStage]->dstBlend = SBA_ONE_MINUS_SRC_ALPHA;
									}
									else
									{
										// error
									}
								}
								else if (tokens.size() == 3)
								{
									if (tokens[1] == "gl_one")
									{
										tmpShaderStages[currentStage]->srcBlend = SBA_ONE;
									}
									else if (tokens[1] == "gl_zero")
									{
										tmpShaderStages[currentStage]->srcBlend = SBA_ZERO;
									}
									else if (tokens[1] == "gl_dst_color")
									{
										tmpShaderStages[currentStage]->srcBlend = SBA_DST_COLOR;
									}
									else if (tokens[1] == "gl_one_minus_dst_color")
									{
										tmpShaderStages[currentStage]->srcBlend = SBA_ONE_MINUS_DST_COLOR;
									}
									else if (tokens[1] == "gl_src_alpha")
									{
										tmpShaderStages[currentStage]->srcBlend = SBA_SRC_ALPHA;
									}
									else if (tokens[1] == "gl_one_minus_src_alpha")
									{
										tmpShaderStages[currentStage]->srcBlend = SBA_ONE_MINUS_SRC_ALPHA;
									}
									else if (tokens[1] == "gl_dst_alpha")
									{
										tmpShaderStages[currentStage]->srcBlend = SBA_ONE;
									}
									else if (tokens[1] == "gl_one_minus_dst_alpha")
									{
										tmpShaderStages[currentStage]->srcBlend = SBA_ZERO;
									}
									else if (tokens[1] == "gl_src_color" || tokens[1] == "gl_one_minus_src_color")
									{
										// error
									}
									else
									{
										// error
									}

									if (tokens[2] == "gl_one")
									{
										tmpShaderStages[currentStage]->dstBlend = SBA_ONE;
									}
									else if (tokens[2] == "gl_zero")
									{
										tmpShaderStages[currentStage]->dstBlend = SBA_ZERO;
									}
									else if (tokens[2] == "gl_src_color")
									{
										tmpShaderStages[currentStage]->dstBlend = SBA_DST_COLOR;
									}
									else if (tokens[2] == "gl_one_minus_src_color")
									{
										tmpShaderStages[currentStage]->dstBlend = SBA_ONE_MINUS_DST_COLOR;
									}
									else if (tokens[2] == "gl_src_alpha")
									{
										tmpShaderStages[currentStage]->dstBlend = SBA_SRC_ALPHA;
									}
									else if (tokens[2] == "gl_one_minus_src_alpha")
									{
										tmpShaderStages[currentStage]->dstBlend = SBA_ONE_MINUS_SRC_ALPHA;
									}
									else if (tokens[1] == "gl_dst_alpha")
									{
										tmpShaderStages[currentStage]->dstBlend = SBA_ONE;
									}
									else if (tokens[1] == "gl_one_minus_dst_alpha")
									{
										tmpShaderStages[currentStage]->dstBlend = SBA_ZERO;
									}
									else if (tokens[2] == "gl_dst_color" || tokens[2] == "gl_one_minus_dst_color")
									{
										// error
									}
									else
									{
										// error
									}
								}
								else
								{
									// error
								}

								if (currentStage == 0 )
								{
									shaderType = ST_BLENDED;

									// todo: may need to change this if smoke ends up looking fucked up
									// .. or something. for now it's saving my ass on weird sorting issues

									if (tmpShaderStages[currentStage]->dstBlend == SBA_ONE)
									{
										depthWrite = false;
									}
								}
							}
							else if (tokens[0] == "rgbgen")
							{
								if (tokens.size() > 2)
								{
									if (tokens[1] == "const")
									{
										if (tokens.size() != 7)
										{
											// error
										}
										else
										{
											float fRed   = (float)atof(tokens[3].c_str());
											float fGreen = (float)atof(tokens[4].c_str());
											float fBlue  = (float)atof(tokens[5].c_str());

											int r = int(fRed * 255.0f);
											int g = int(fGreen * 255.0f);
											int b = int(fBlue * 255.0f);
											
											int a = tmpShaderStages[currentStage]->constantColor & 0xFF000000;

											if (r < 0) r = 0;
											if (g < 0) g = 0;
											if (b < 0) b = 0;

											if (r > 0xFF) r = 0xFF;
											if (g > 0xFF) g = 0xFF;
											if (b > 0xFF) b = 0xFF;

											tmpShaderStages[currentStage]->constantColor = (r << 16) | (g << 8) | b | a;

											tmpShaderStages[currentStage]->rgbGenType = SRG_CONSTANT;
										}
									}
									else if (tokens[1] == "wave")
									{
										if (tokens.size() == 7)
										{
											string func = tokens[2];

											if (func == "sin")
											{
												tmpShaderStages[currentStage]->rgbGenWave.waveType = WT_SIN;
											}
											else if (func == "triangle")
											{
												tmpShaderStages[currentStage]->rgbGenWave.waveType = WT_TRIANGLE;
											}
											else if (func == "square")
											{
												tmpShaderStages[currentStage]->rgbGenWave.waveType = WT_SQUARE;
											}
											else if (func == "sawtooth")
											{
												tmpShaderStages[currentStage]->rgbGenWave.waveType = WT_SAWTOOTH;
											}
											else if (func == "inversesawtooth")
											{
												tmpShaderStages[currentStage]->rgbGenWave.waveType = WT_INVERSESAWTOOTH;
											}
											else
											{
												break;
												// error
											}

											tmpShaderStages[currentStage]->rgbGenWave.base		= (float)atof(tokens[3].c_str());
											tmpShaderStages[currentStage]->rgbGenWave.amplitude	= (float)atof(tokens[4].c_str());
											tmpShaderStages[currentStage]->rgbGenWave.phase		= (float)atof(tokens[5].c_str());
											tmpShaderStages[currentStage]->rgbGenWave.frequency = (float)atof(tokens[6].c_str());

											tmpShaderStages[currentStage]->rgbGenType = SRG_WAVE;
										}
										else
										{
											// error
										}
									}
								}
								else if (tokens.size() == 2)
								{
									if (tokens[1] == "identitylighting")
									{
										tmpShaderStages[currentStage]->rgbGenType = SRG_IDENTITYLIGHTING;
									}
									else if (tokens[1] == "identity")
									{
										tmpShaderStages[currentStage]->rgbGenType = SRG_IDENTITY;
									}
									else if (tokens[1] == "entity")
									{
										tmpShaderStages[currentStage]->rgbGenType = SRG_ENTITY;
									}
									else if (tokens[1] == "oneminusentity")
									{
										tmpShaderStages[currentStage]->rgbGenType = SRG_ONE_MINUS_ENTITY;
									}
									else if (tokens[1] == "vertex")
									{
										tmpShaderStages[currentStage]->rgbGenType = SRG_VERTEX;
									}
									else if (tokens[1] == "oneminusvertex")
									{
										tmpShaderStages[currentStage]->rgbGenType = SRG_ONE_MINUS_VERTEX;
									}
									else if (tokens[1] == "lightingdiffuse")
									{
										tmpShaderStages[currentStage]->rgbGenType = SRG_LIGHTINGDIFFUSE;
									}
									else if (tokens[1] == "lightingspecular")
									{
										tmpShaderStages[currentStage]->rgbGenType = SRG_LIGHTINGSPECULAR;
									}
									else
									{
										// error
									}
								}
							}
							else if (tokens[0] == "alphagen")
							{
								if (tokens.size() > 2)
								{
									if (tokens[1] == "const")
									{
										// TODO: I don't think constant alpha gen is supported, or if it is
										// , I don't know what format exactly it takes, for now, we'll just ignore it.

										tmpShaderStages[currentStage]->alphaGenType = SRG_VERTEX;
									}
									else if (tokens[1] == "wave")
									{
										if (tokens.size() == 7)
										{
											string func = tokens[2];

											if (func == "sin")
											{
												tmpShaderStages[currentStage]->alphaGenWave.waveType = WT_SIN;
											}
											else if (func == "triangle")
											{
												tmpShaderStages[currentStage]->alphaGenWave.waveType = WT_TRIANGLE;
											}
											else if (func == "square")
											{
												tmpShaderStages[currentStage]->alphaGenWave.waveType = WT_SQUARE;
											}
											else if (func == "sawtooth")
											{
												tmpShaderStages[currentStage]->alphaGenWave.waveType = WT_SAWTOOTH;
											}
											else if (func == "inversesawtooth")
											{
												tmpShaderStages[currentStage]->alphaGenWave.waveType = WT_INVERSESAWTOOTH;
											}
											else
											{
												break;
												// error
											}

											tmpShaderStages[currentStage]->alphaGenWave.base		= (float)atof(tokens[3].c_str());
											tmpShaderStages[currentStage]->alphaGenWave.amplitude	= (float)atof(tokens[4].c_str());
											tmpShaderStages[currentStage]->alphaGenWave.phase		= (float)atof(tokens[5].c_str());
											tmpShaderStages[currentStage]->alphaGenWave.frequency	= (float)atof(tokens[6].c_str());

											tmpShaderStages[currentStage]->alphaGenType = SRG_WAVE;
										}
										else
										{
											//error
										}
									}
								}
								else if (tokens.size() == 2)
								{
									if (tokens[1] == "identitylighting")
									{
										tmpShaderStages[currentStage]->alphaGenType = SRG_IDENTITYLIGHTING;
									}
									else if (tokens[1] == "identity")
									{
										tmpShaderStages[currentStage]->alphaGenType = SRG_IDENTITY;
									}
									else if (tokens[1] == "entity")
									{
										tmpShaderStages[currentStage]->alphaGenType = SRG_ENTITY;
									}
									else if (tokens[1] == "oneminusentity")
									{
										tmpShaderStages[currentStage]->alphaGenType = SRG_ONE_MINUS_ENTITY;
									}
									else if (tokens[1] == "vertex")
									{
										tmpShaderStages[currentStage]->alphaGenType = SRG_VERTEX;
									}
									else if (tokens[1] == "oneminusvertex")
									{
										tmpShaderStages[currentStage]->alphaGenType = SRG_ONE_MINUS_VERTEX;
									}
									else if (tokens[1] == "lightingdiffuse")
									{
										tmpShaderStages[currentStage]->alphaGenType = SRG_LIGHTINGDIFFUSE;
									}
									else if (tokens[1] == "lightingspecular")
									{
										tmpShaderStages[currentStage]->alphaGenType = SRG_LIGHTINGSPECULAR;
									}
									else if (tokens[1] == "portal")
									{
										// ignore
									}
									else
									{
										// error
									}
								}
							}
							else if (tokens[0] == "tcgen")
							{
								if (tokens.size() == 2)
								{
									if (tokens[1] == "base")
									{
										tmpShaderStages[currentStage]->texCoordSource = STS_BASE;
									}
									else if (tokens[1] == "lightmap")
									{
										tmpShaderStages[currentStage]->texCoordSource = STS_LIGHTMAP;
									}
									else if (tokens[1] == "environment")
									{
										tmpShaderStages[currentStage]->texCoordSource = STS_ENVIRONMENT;
									}
									else
									{
										// error
									}
								}
								else
								{
									// todo
								}
							}
							else if (tokens[0] == "tcmod")
							{
								if (tokens[1] == "rotate")
								{
									if (tokens.size() != 3)
									{
										// error
									}
									else
									{
										SHADER_TC_MOD_INFO tcMod;

										tcMod.tcModType = STCMT_ROTATE;
										tcMod.rotSpeed  = (float)atof((const char *)tokens[2].c_str());

										tmpShaderStages[currentStage]->tcMods.push_back(tcMod);
									}
								}
								else if (tokens[1] == "scale")
								{
									if (tokens.size() != 4)
									{
										// error
									}
									else
									{
										SHADER_TC_MOD_INFO tcMod;

										tcMod.tcModType = STCMT_SCALE;

										float scaleU = (float)atof((const char *)tokens[2].c_str());
										float scaleV = (float)atof((const char *)tokens[3].c_str());

										D3DXMatrixScaling(&tcMod.matrix, scaleU, scaleV, 1.0f);

										tmpShaderStages[currentStage]->tcMods.push_back(tcMod);
									}
								}
								else if (tokens[1] == "scroll")
								{
									if (tokens.size() != 4)
									{
										// error
									}
									else
									{
										SHADER_TC_MOD_INFO tcMod;

										tcMod.tcModType = STCMT_SCROLL;

										tcMod.scrollSpeedU = (float)atof((const char *)tokens[2].c_str());
										tcMod.scrollSpeedV = (float)atof((const char *)tokens[3].c_str());

										tmpShaderStages[currentStage]->tcMods.push_back(tcMod);
									}
								}
								else if (tokens[1] == "stretch")
								{
									if (tokens.size() != 7)
									{
										// error
									}
									else
									{
										SHADER_TC_MOD_INFO tcMod;

										string func = tokens[2];

										if (func == "sin")
										{
											tcMod.wave.waveType = WT_SIN;
										}
										else if (func == "triangle")
										{
											tcMod.wave.waveType = WT_TRIANGLE;
										}
										else if (func == "square")
										{
											tcMod.wave.waveType = WT_SQUARE;
										}
										else if (func == "sawtooth")
										{
											tcMod.wave.waveType = WT_SAWTOOTH;
										}
										else if (func == "inversesawtooth")
										{
											tcMod.wave.waveType = WT_INVERSESAWTOOTH;
										}
										else
										{
											break;
											// error
										}

										tcMod.wave.base		 = (float)atof(tokens[3].c_str());
										tcMod.wave.amplitude = (float)atof(tokens[4].c_str());
										tcMod.wave.phase	 = (float)atof(tokens[5].c_str());
										tcMod.wave.frequency = (float)atof(tokens[6].c_str());

										tcMod.tcModType = STCMT_STRETCH;

										tmpShaderStages[currentStage]->tcMods.push_back(tcMod);
									}
								}
								else if (tokens[1] == "turb")
								{
									if (tokens.size() != 6)
									{
										// error
									}
									else
									{
										SHADER_TC_MOD_INFO tcMod;

										tcMod.wave.waveType = WT_SIN;
			
										tcMod.wave.base		 = (float)atof(tokens[2].c_str());
										tcMod.wave.amplitude = (float)atof(tokens[3].c_str());
										tcMod.wave.phase	 = (float)atof(tokens[4].c_str());
										tcMod.wave.frequency = (float)atof(tokens[5].c_str());

										tcMod.tcModType = STCMT_TURB;

										tmpShaderStages[currentStage]->tcMods.push_back(tcMod);
									}
								}
								else if (tokens[1] == "transform")
								{
									if (tokens.size() != 8)
									{
										// error
									}
									else
									{
										SHADER_TC_MOD_INFO tcMod;

										tcMod.tcModType = STCMT_TRANSFORM;

										float mat00 = (float)atof((const char *)tokens[2].c_str());
										float mat01 = (float)atof((const char *)tokens[3].c_str());
										float mat10 = (float)atof((const char *)tokens[4].c_str());
										float mat11 = (float)atof((const char *)tokens[5].c_str());

										float transU = (float)atof((const char *)tokens[6].c_str());
										float transV = (float)atof((const char *)tokens[7].c_str());

										D3DXMatrixIdentity(&tcMod.matrix);

										// todo: may need to fix row major / column major problems here

										//           r  c
										tcMod.matrix(0, 0) = mat00;
										tcMod.matrix(0, 1) = mat01;
										tcMod.matrix(1, 0) = mat10;
										tcMod.matrix(1, 1) = mat11;

										tcMod.matrix(2, 0) = transU;
										tcMod.matrix(2, 1) = transV;

										tmpShaderStages[currentStage]->tcMods.push_back(tcMod);
									}
								}
							}
							else if (tokens[0] == "depthfunc")
							{
								if (tokens.size() == 2)
								{
									if (tokens[1] == "lequal")
									{
										depthFunc = D3DCMP_LESSEQUAL;
									}
									else if (tokens[1] == "equal")
									{
										depthFunc = D3DCMP_EQUAL;
									}
									else
									{
										// error
									}
								}
							}
							else if (tokens[0] == "alphafunc")
							{
								if (tokens.size() == 2)
								{
									alphaTestEnable = true;

									if (tokens[1] == "gt0")
									{
										alphaFunc = D3DCMP_GREATER;
										alphaRef  = 0;
									}
									else if (tokens[1] == "lt128")
									{
										alphaFunc = D3DCMP_LESS;
										alphaRef  = 128;
									}
									else if (tokens[1] == "ge128")
									{
										alphaFunc = D3DCMP_GREATEREQUAL;
										alphaRef  = 128;
									}
									else
									{
										// error
									}
								}
								else
								{
									// error
								}
							}
							else
							{
								// error
							}

							// multi-token stage statements
						}
					}

					break;
				}
			}
			else
			{
				break;
			}
		}

		shaderFile.close();

		// compile stages into a bunch of texture stage states / texture matrix alterations
		// determine shader type based on top level blend mode and whether or not skyparms were received

		if (forceDepthWrite)
		{
			depthWrite = true;
		}

		numVertexDeformations = (int)vertexDeformations.size();

		if (numVertexDeformations)
		{
			this->vertexDeformations = new DEFORMVERTEXES_INFO[numVertexDeformations];

			for (int d = 0; d < numVertexDeformations; d++)
			{
				this->vertexDeformations[d] = vertexDeformations[d];
			}
		}
		else
		{
			this->vertexDeformations = NULL;
		}

		D3DPIXELSHADERDEF pixelShaderDef;
		ZeroMemory(&pixelShaderDef, sizeof(D3DPIXELSHADERDEF));

		int combinerStage = 0;

		numStages = (int)tmpShaderStages.size();
		
		if (numStages)
		{
			stages = new SHADER_STAGE[numStages];
		}
		else
		{
			stages = NULL;
		}

		for (int s = 0; s < numStages; s++)
		{
			TSHADER_STAGE *tmpStage = tmpShaderStages[s];

			stages[s].numTextures = (int)tmpStage->texturePaths.size();

			if (stages[s].numTextures)
			{
				stages[s].textures = new CTextureResource*[stages[s].numTextures];

				for (int t = 0; t < stages[s].numTextures; t++)
				{
					stages[s].textures[t] = CTextureResource::getTexture(tmpStage->texturePaths[t]);
				}
			}
			else
			{
				stages[s].textures = NULL;
			}

			stages[s].combinerStage = 0; // unused unless this stage specifies an rgbgen/wavegen with a certain type

			stages[s].texCoordSource = tmpStage->texCoordSource;
			stages[s].isClamped      = tmpStage->isClamped;

			stages[s].isAnimMap     = tmpStage->isAnimMap;
			stages[s].animFrequency = tmpStage->animFrequency;

			stages[s].rgbGenType   = tmpStage->rgbGenType;     // only used if it's a wave or constant or from entity
			stages[s].alphaGenType = tmpStage->alphaGenType;   // likewise only used if its a wave or constant or from entity

			stages[s].numTcMods = (int)tmpStage->tcMods.size();

			D3DXMatrixIdentity(&stages[s].tcMatrix);

			if (stages[s].numTcMods)
			{
				stages[s].tcMods = new SHADER_TC_MOD_INFO[stages[s].numTcMods];

				for (int t = 0; t < stages[s].numTcMods; t++)
				{
					stages[s].tcMods[t] = tmpStage->tcMods[t];
				}
			}
			else
			{
				stages[s].tcMods = NULL;
			}

			stages[s].color = tmpStage->constantColor;

			stages[s].rgbGenWave   = tmpStage->rgbGenWave;
			stages[s].alphaGenWave = tmpStage->alphaGenWave;

			if (resourceName == "textures\\sockter\\ter_mossmud")
			{
				int x = 5;
			}

			// set our PSD fields based on rgbGenType and alphaGenType, as well as the src and dst blends

			if (s == 0)
			{
				if (shaderType == ST_BLENDED)
				{
					switch (tmpStage->srcBlend)
					{
					case SBA_ONE:

						srcBlend = D3DBLEND_ONE;
						break;

					case SBA_ZERO:

						srcBlend = D3DBLEND_ZERO;
						break;

					case SBA_DST_COLOR:

						srcBlend = D3DBLEND_DESTCOLOR;
						break;

					case SBA_ONE_MINUS_DST_COLOR:

						srcBlend = D3DBLEND_INVDESTCOLOR;
						break;

					case SBA_SRC_ALPHA:

						srcBlend = D3DBLEND_SRCALPHA;
						break;

					case SBA_ONE_MINUS_SRC_ALPHA:

						srcBlend = D3DBLEND_INVSRCALPHA;
						break;

					default:

						srcBlend = D3DBLEND_ONE;
						break;
					}

					switch (tmpStage->dstBlend)
					{
					case SBA_ONE:

						dstBlend = D3DBLEND_ONE;
						break;

					case SBA_ZERO:

						dstBlend = D3DBLEND_ZERO;
						break;

					case SBA_SRC_COLOR:

						dstBlend = D3DBLEND_SRCCOLOR;
						break;

					case SBA_ONE_MINUS_SRC_COLOR:

						dstBlend = D3DBLEND_INVSRCCOLOR;
						break;

					case SBA_SRC_ALPHA:

						dstBlend = D3DBLEND_SRCALPHA;
						break;

					case SBA_ONE_MINUS_SRC_ALPHA:

						dstBlend = D3DBLEND_INVSRCALPHA;
						break;

					default:

						dstBlend = D3DBLEND_ONE;
						break;
					}
				}
			}

			// combiner stage inputs

			int a = 0;
			int b = 0;
			int c = 0;
			int d = 0;

			bool useTmpSrcAlpha = false;

			if (tmpStage->rgbGenType > SRG_IDENTITY || tmpStage->alphaGenType > SRG_IDENTITY)
			{
				stages[s].combinerStage = combinerStage;

				switch (s)
				{
				case 0:

					a = PS_REGISTER_T0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
					c = PS_REGISTER_T0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;

					break;

				case 1:

					a = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
					c = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;

					break;

				case 2:

					a = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
					c = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;

					break;

				case 3:

					a = PS_REGISTER_T3 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
					c = PS_REGISTER_T3 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;

					break;
				}

				switch (tmpStage->rgbGenType)
				{
				case SRG_CONSTANT:
				case SRG_WAVE:
				case SRG_ENTITY:

					b = PS_REGISTER_C0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
					break;

				case SRG_ONE_MINUS_ENTITY:

					b = PS_REGISTER_C0 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
					break;

				case SRG_VERTEX:
				case SRG_LIGHTINGDIFFUSE:

					b = PS_REGISTER_V0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
					break;

				case SRG_ONE_MINUS_VERTEX:

					b = PS_REGISTER_V0 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
					break;

				case SRG_LIGHTINGSPECULAR:

					b = PS_REGISTER_V1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
					break;

				default:

					b = PS_REGISTER_ONE | PS_CHANNEL_RGB;
					break;

				}

				switch (tmpStage->alphaGenType)
				{
				case SRG_WAVE:
				case SRG_ENTITY:
				
					d = PS_REGISTER_C0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;
					break;

				case SRG_ONE_MINUS_ENTITY:

					d = PS_REGISTER_C0 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_ALPHA;
					break;

				case SRG_VERTEX:
				case SRG_LIGHTINGDIFFUSE:

					d = PS_REGISTER_V0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;
					break;

				case SRG_ONE_MINUS_VERTEX:

					d = PS_REGISTER_V0 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_ALPHA;
					break;

				case SRG_LIGHTINGSPECULAR:

					d = PS_REGISTER_V1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB; // TODO: may need to 
																							// set channel source to alpha
					break;

				default:

					d = PS_REGISTER_ONE | PS_CHANNEL_ALPHA;
					break;
				}

				pixelShaderDef.PSRGBInputs[combinerStage] = PS_COMBINERINPUTS(a, b, c, d);

				pixelShaderDef.PSAlphaOutputs[combinerStage] = PS_COMBINEROUTPUTS(PS_REGISTER_DISCARD,
																				  PS_REGISTER_DISCARD,
																				  PS_REGISTER_DISCARD,
																				  PS_COMBINEROUTPUT_IDENTITY |
																				  PS_COMBINEROUTPUT_AB_CD_MUX | 
																				  PS_COMBINEROUTPUT_AB_MULTIPLY | 
																				  PS_COMBINEROUTPUT_CD_MULTIPLY);

				pixelShaderDef.PSAlphaInputs[combinerStage] = PS_COMBINERINPUTS(PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
															  PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
															  PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
														      PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA);

				switch (s)
				{
				case 0:

					pixelShaderDef.PSRGBOutputs[combinerStage] = PS_COMBINEROUTPUTS(PS_REGISTER_T0,
																					PS_REGISTER_R1,
																					PS_REGISTER_DISCARD,
																					PS_COMBINEROUTPUT_IDENTITY |
																					PS_COMBINEROUTPUT_AB_CD_SUM | 
																					PS_COMBINEROUTPUT_AB_MULTIPLY | 
																					PS_COMBINEROUTPUT_CD_MULTIPLY);
					break;

				case 1:

					pixelShaderDef.PSRGBOutputs[combinerStage] = PS_COMBINEROUTPUTS(PS_REGISTER_T1,
																					PS_REGISTER_R1,
																					PS_REGISTER_DISCARD,
																					PS_COMBINEROUTPUT_IDENTITY |
																					PS_COMBINEROUTPUT_AB_CD_SUM | 
																					PS_COMBINEROUTPUT_AB_MULTIPLY | 
																					PS_COMBINEROUTPUT_CD_MULTIPLY);
					break;

				case 2:

					pixelShaderDef.PSRGBOutputs[combinerStage] = PS_COMBINEROUTPUTS(PS_REGISTER_T2,
																					PS_REGISTER_R1,
																					PS_REGISTER_DISCARD,
																					PS_COMBINEROUTPUT_IDENTITY |
																					PS_COMBINEROUTPUT_AB_CD_SUM | 
																					PS_COMBINEROUTPUT_AB_MULTIPLY | 
																					PS_COMBINEROUTPUT_CD_MULTIPLY);
					break;

				case 3:

					pixelShaderDef.PSRGBOutputs[combinerStage] = PS_COMBINEROUTPUTS(PS_REGISTER_T3,
																					PS_REGISTER_R1,
																					PS_REGISTER_DISCARD,
																					PS_COMBINEROUTPUT_IDENTITY |
																					PS_COMBINEROUTPUT_AB_CD_SUM | 
																					PS_COMBINEROUTPUT_AB_MULTIPLY | 
																					PS_COMBINEROUTPUT_CD_MULTIPLY);
					break;
				}

				useTmpSrcAlpha = true;
				combinerStage++;
			}

			switch (s)
			{
			case 0:

				a = PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
				b = PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;

				c = PS_REGISTER_T0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;

				if (useTmpSrcAlpha)
				{
					d = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
				}
				else
				{
					d = PS_REGISTER_ONE | PS_CHANNEL_RGB;
				}

				break;

			case 1:

				a = PS_REGISTER_T0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
				c = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;

				break;

			case 2:

				a = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
				c = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;

				break;

			case 3:

				a = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
				c = PS_REGISTER_T3 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;

				break;
			}

			if (s)
			{
				switch (tmpStage->dstBlend)
				{
				case SBA_ONE:

					b = PS_REGISTER_ONE | PS_CHANNEL_RGB;
					break;

				case SBA_ZERO:

					b = PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
					break;

				case SBA_SRC_COLOR:

					switch (s)
					{
					case 1:

						b = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						break;

					case 2:

						b = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						break;

					case 3:

						b = PS_REGISTER_T3 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						break;

					default:

						// error
						break;
					}
					
					break;

				case SBA_ONE_MINUS_SRC_COLOR:

					switch (s)
					{
					case 1:

						b = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						break;

					case 2:

						b = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						break;

					case 3:

						b = PS_REGISTER_T3 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						break;

					default:

						// error
						break;
					}

					break;

				case SBA_SRC_ALPHA:

					switch (s)
					{
					case 1:

						if (useTmpSrcAlpha)
						{
							b = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						}
						else
						{
							b = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;
						}

						break;
					
					case 2:

						if (useTmpSrcAlpha)
						{
							b = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						}
						else
						{
							b = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;
						}

						break;

					case 3:

						if (useTmpSrcAlpha)
						{
							b = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						}
						else
						{
							b = PS_REGISTER_T3 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;
						}

						break;
					}

					break;

				case SBA_ONE_MINUS_SRC_ALPHA:

					switch (s)
					{
					case 1:

						if (useTmpSrcAlpha)
						{
							b = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						}
						else
						{
							b = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_ALPHA;
						}

						break;
					
					case 2:

						if (useTmpSrcAlpha)
						{
							b = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						}
						else
						{
							b = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_ALPHA;
						}

						break;

					case 3:

						if (useTmpSrcAlpha)
						{
							b = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						}
						else
						{
							b = PS_REGISTER_T3 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_ALPHA;
						}
						
						break;
					}

					break;

				default:

					// error
					break;
				}

				switch (tmpStage->srcBlend)
				{
				case SBA_ONE:

					d = PS_REGISTER_ONE | PS_CHANNEL_RGB;
					break;

				case SBA_ZERO:

					d = PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
					break;

				case SBA_DST_COLOR:

					switch (s)
					{
					case 1:

						d = PS_REGISTER_T0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						break;

					case 2:

						d = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						break;

					case 3:

						d = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						break;

					default:

						// error
						break;
					}
					
					break;

				case SBA_ONE_MINUS_DST_COLOR:

					switch (s)
					{
					case 1:

						d = PS_REGISTER_T0 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						break;

					case 2:

						b = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						break;

					case 3:

						d = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						break;

					default:

						// error
						break;
					}

					break;

				case SBA_SRC_ALPHA:

					switch (s)
					{
					case 1:

						if (useTmpSrcAlpha)
						{
							d = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						}
						else
						{
							d = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;
						}

						break;
					
					case 2:

						if (useTmpSrcAlpha)
						{
							d = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						}
						else
						{
							d = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;
						}

						break;

					case 3:

						if (useTmpSrcAlpha)
						{
							d = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
						}
						else
						{
							d = PS_REGISTER_T3 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;
						}

						break;
					}

					break;

				case SBA_ONE_MINUS_SRC_ALPHA:

					switch (s)
					{
					case 1:

						if (useTmpSrcAlpha)
						{
							d = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						}
						else
						{
							d = PS_REGISTER_T1 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_ALPHA;
						}

						break;
					
					case 2:

						if (useTmpSrcAlpha)
						{
							d = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						}
						else
						{
							d = PS_REGISTER_T2 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_ALPHA;
						}

						break;

					case 3:

						if (useTmpSrcAlpha)
						{
							d = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_RGB;
						}
						else
						{
							d = PS_REGISTER_T3 | PS_INPUTMAPPING_UNSIGNED_INVERT | PS_CHANNEL_ALPHA;
						}
						
						break;
					}

					break;

				default:

					// error
					break;
				}
			}

			pixelShaderDef.PSRGBInputs[combinerStage] = PS_COMBINERINPUTS(a, b, c, d);

			if (s == 0)
			{
				int aa = PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
				int ab = PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
				int ac = PS_REGISTER_T0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA;
				int ad = PS_REGISTER_ONE | PS_CHANNEL_ALPHA;

				//if (useTmpSrcAlpha)
				//{
				//	ad = PS_REGISTER_R1 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
				//}
				
				pixelShaderDef.PSAlphaInputs[0] = PS_COMBINERINPUTS(aa, ab, ac, ad);

				pixelShaderDef.PSAlphaOutputs[0] = PS_COMBINEROUTPUTS(PS_REGISTER_DISCARD,
															          PS_REGISTER_R0,
																	  PS_REGISTER_DISCARD,
																	  PS_COMBINEROUTPUT_IDENTITY |
																	  PS_COMBINEROUTPUT_AB_CD_SUM | 
																	  PS_COMBINEROUTPUT_AB_MULTIPLY | 
																	  PS_COMBINEROUTPUT_CD_MULTIPLY);
			}
			else
			{
				int aa = PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
				int ab = PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
				int ac = PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;
				int ad = PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB;

				pixelShaderDef.PSAlphaInputs[combinerStage]  = PS_COMBINERINPUTS(aa, ab, ac, ad);

				pixelShaderDef.PSAlphaOutputs[combinerStage] = PS_COMBINEROUTPUTS(PS_REGISTER_DISCARD,
																				  PS_REGISTER_DISCARD,
																				  PS_REGISTER_DISCARD,
																				  PS_COMBINEROUTPUT_IDENTITY |
																				  PS_COMBINEROUTPUT_AB_CD_SUM | 
																				  PS_COMBINEROUTPUT_AB_MULTIPLY | 
																				  PS_COMBINEROUTPUT_CD_MULTIPLY);
			}

			if (s == numStages - 1)
			{
				pixelShaderDef.PSRGBOutputs[combinerStage] = PS_COMBINEROUTPUTS(PS_REGISTER_DISCARD,
																				PS_REGISTER_DISCARD,
																				PS_REGISTER_R0,
																				PS_COMBINEROUTPUT_IDENTITY |
																				PS_COMBINEROUTPUT_AB_CD_SUM | 
																				PS_COMBINEROUTPUT_AB_MULTIPLY | 
																				PS_COMBINEROUTPUT_CD_MULTIPLY);
			}
			else
			{
				switch (s)
				{
				case 0:

					pixelShaderDef.PSRGBOutputs[combinerStage] = PS_COMBINEROUTPUTS(PS_REGISTER_DISCARD,
																					PS_REGISTER_DISCARD,
																					PS_REGISTER_T0,
																					PS_COMBINEROUTPUT_IDENTITY |
																					PS_COMBINEROUTPUT_AB_CD_SUM | 
																					PS_COMBINEROUTPUT_AB_MULTIPLY | 
																					PS_COMBINEROUTPUT_CD_MULTIPLY);
					break;

				case 1:

					pixelShaderDef.PSRGBOutputs[combinerStage] = PS_COMBINEROUTPUTS(PS_REGISTER_DISCARD,
																					PS_REGISTER_DISCARD,
																					PS_REGISTER_T1,
																					PS_COMBINEROUTPUT_IDENTITY |
																					PS_COMBINEROUTPUT_AB_CD_SUM | 
																					PS_COMBINEROUTPUT_AB_MULTIPLY | 
																					PS_COMBINEROUTPUT_CD_MULTIPLY);
					break;

				case 2:

					pixelShaderDef.PSRGBOutputs[combinerStage] = PS_COMBINEROUTPUTS(PS_REGISTER_DISCARD,
																					PS_REGISTER_DISCARD,
																					PS_REGISTER_T2,
																					PS_COMBINEROUTPUT_IDENTITY |
																					PS_COMBINEROUTPUT_AB_CD_SUM | 
																					PS_COMBINEROUTPUT_AB_MULTIPLY | 
																					PS_COMBINEROUTPUT_CD_MULTIPLY);
					break;

				case 3:

					pixelShaderDef.PSRGBOutputs[combinerStage] = PS_COMBINEROUTPUTS(PS_REGISTER_DISCARD,
																					PS_REGISTER_DISCARD,
																					PS_REGISTER_T3,
																					PS_COMBINEROUTPUT_IDENTITY |
																					PS_COMBINEROUTPUT_AB_CD_SUM | 
																					PS_COMBINEROUTPUT_AB_MULTIPLY | 
																					PS_COMBINEROUTPUT_CD_MULTIPLY);
					break;
				}
			}

			combinerStage++;
		}

		pixelShaderDef.PSCombinerCount = PS_COMBINERCOUNT(combinerStage, PS_COMBINERCOUNT_MUX_MSB |
																	     PS_COMBINERCOUNT_UNIQUE_C0 |
																         PS_COMBINERCOUNT_UNIQUE_C1);

		int texModes[4];

		for (int s = 0; s < numStages; s++)
		{
			if (stages[s].textures || stages[s].texCoordSource == STS_LIGHTMAP)
			{
				texModes[s] = PS_TEXTUREMODES_PROJECT2D;
			}
			else
			{
				texModes[s] = PS_TEXTUREMODES_NONE;
			}
		}

		for (int s = numStages; s < 4; s++)
		{
			texModes[s] = PS_TEXTUREMODES_NONE;
		}

		pixelShaderDef.PSTextureModes = PS_TEXTUREMODES(texModes[0],
														texModes[1],
														texModes[2],
														texModes[3]);

		// we're not using the final combiner

		/*
		pixelShaderDef.PSFinalCombinerInputsABCD = PS_COMBINERINPUTS(
			PS_REGISTER_ONE  | PS_CHANNEL_RGB | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			PS_REGISTER_R0   | PS_CHANNEL_RGB | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			PS_REGISTER_ZERO | PS_CHANNEL_RGB | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			PS_REGISTER_ZERO | PS_CHANNEL_RGB | PS_INPUTMAPPING_UNSIGNED_IDENTITY );

		// E = 0, F = 0, G = 1. (From above, EF is not used. G is alpha and is set to 1.)
		pixelShaderDef.PSFinalCombinerInputsEFG = PS_COMBINERINPUTS(
			PS_REGISTER_ZERO | PS_CHANNEL_RGB | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			PS_REGISTER_ZERO | PS_CHANNEL_RGB | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			PS_REGISTER_R0   | PS_CHANNEL_ALPHA | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			0 | 0 | 0 );
		*/

		pixelShaderDef.PSC0Mapping = 0x00000000;
		pixelShaderDef.PSC1Mapping = 0x00000000;

		pixelShaderDef.PSInputTexture  = PS_INPUTTEXTURE(0, 0, 0, 0);

		pixelShaderDef.PSDotMapping    = PS_DOTMAPPING(0,
										 PS_DOTMAPPING_ZERO_TO_ONE,
										 PS_DOTMAPPING_ZERO_TO_ONE,
										 PS_DOTMAPPING_ZERO_TO_ONE);

		pixelShaderDef.PSCompareMode = PS_COMPAREMODE(
        PS_COMPAREMODE_S_LT | PS_COMPAREMODE_T_LT | PS_COMPAREMODE_R_LT | PS_COMPAREMODE_Q_LT,
        PS_COMPAREMODE_S_LT | PS_COMPAREMODE_T_LT | PS_COMPAREMODE_R_LT | PS_COMPAREMODE_Q_LT,
        PS_COMPAREMODE_S_LT | PS_COMPAREMODE_T_LT | PS_COMPAREMODE_R_LT | PS_COMPAREMODE_Q_LT,
        PS_COMPAREMODE_S_LT | PS_COMPAREMODE_T_LT | PS_COMPAREMODE_R_LT | PS_COMPAREMODE_Q_LT);

		if (numStages)
		{
			device->CreatePixelShader(&pixelShaderDef, &pixelShader);
		}
		else
		{
			pixelShader = NULL;
		}

		for (int s = 0; s < (int)tmpShaderStages.size(); s++)
		{
			delete (TSHADER_STAGE*)tmpShaderStages[s];
		}

		tmpShaderStages.clear();

		// we're done =)
	}
	else
	{
		string debugString = "*** ERROR *** Failed to open shader file '";

		debugString += shaderInfo->shaderFile;
		debugString += "'\n";

		XEngine->debugPrint(debugString);
	}

	__super::create(resourceName);

	shaderResources.push_back(this);
	shaderResourcesListPosition = shaderResources.end();
}

bool CShaderResource::isDynamic()
{
	return _isDynamic;
}

void CShaderResource::destroy()
{
	//todo: release all shader resource memory

	shaderResources.erase(shaderResourcesListPosition);

	__super::destroy();
}