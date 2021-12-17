#include <xtl.h>

#include "texture_resource.h"
#include "resource_types.h"
#include "xengine.h"
#include "string_help.h"

vector<CResource*> CTextureResource::textureResources;

int CTextureResource::getResourceType()
{
	return RT_TEXTURE_RESOURCE;
}

bool CTextureResource::hasAlphaChannel()
{
	return _hasAlphaChannel;
}

CTextureResource *CTextureResource::getTexture(string textureName)
{
	string explicitPath = XENGINE_RESOURCES_MEDIA_PATH;
	explicitPath += textureName;

	int extensionPosition = textureName.find('.');

	if (extensionPosition > 0)
	{
		textureName = textureName.substr(0, extensionPosition);
	}

	string tgaPath = XENGINE_RESOURCES_MEDIA_PATH;
	
	tgaPath += textureName;
	tgaPath += ".tga";

	string jpgPath = XENGINE_RESOURCES_MEDIA_PATH;
	
	jpgPath += textureName;
	jpgPath += ".jpg";

	string pngPath = XENGINE_RESOURCES_MEDIA_PATH;
	
	pngPath += textureName;
	pngPath += ".png";
	
	string texturePath = "";
	FILE *fp; 

	if (fp = fopen(pngPath.c_str(), "rb"))
	{
		texturePath = pngPath;

		fclose(fp);
	}
	
	if (fp = fopen(jpgPath.c_str(), "rb"))
	{
		texturePath = jpgPath;

		fclose(fp);
	}

	if (fp = fopen(tgaPath.c_str(), "rb"))
	{
		texturePath = tgaPath;

		fclose(fp);
	}

	if (fp = fopen(explicitPath.c_str(), "rb"))
	{
		texturePath = explicitPath;

		fclose(fp);
	}

	if (texturePath != "")
	{
		textureName = texturePath.substr(3, texturePath.length() - 3);

		for (int i = 0; i < (int)CTextureResource::textureResources.size(); i++)
		{
			if (CTextureResource::textureResources[i]->getResourceName() == textureName)
			{
				CTextureResource::textureResources[i]->addRef();

				return (CTextureResource*)CTextureResource::textureResources[i];
			}
		}
	}

	if (texturePath == "")
	{
		string debugString = "*** WARNING *** Unable to find texture '";

		debugString += textureName;
		debugString += "'!\n";

		XEngine->debugPrint(debugString);

		return NULL;
	}
	else
	{
		CTextureResource *textureResource = new CTextureResource;

		textureResource->create(textureName);
		textureResource->cache();

		return textureResource;
	}
}

void CTextureResource::create(string textureName)
{
	XEngine->reserveCacheResourceMemory();

	string texturePath = XENGINE_RESOURCES_MEDIA_PATH + textureName;

	bool useCompression = XENGINE_TEXTURE_RESOURCE_USE_COMPRESSED_TEXTURES; // todo: take a parameter to do this

	string extension = lowerCaseString(texturePath.substr(texturePath.length() - 3, 3));

	HRESULT result;

	if (extension == "tga")
	{
		if (useCompression)
		{
			result = D3DXCreateTextureFromFileEx(XEngine->getD3DDevice(), texturePath.c_str(),
												0, 0, XENGINE_TEXTURE_RESOURCE_DEFAULT_MIPS,
												0, XENGINE_TEXTURE_RESOURCE_DEFAULT_COMPRESSED_ALPHA_FORMAT,
												0, D3DX_DEFAULT, D3DX_DEFAULT,
												0, NULL, NULL, &texture);
		}
		else
		{
			result = D3DXCreateTextureFromFileEx(XEngine->getD3DDevice(), texturePath.c_str(),
												0, 0, XENGINE_TEXTURE_RESOURCE_DEFAULT_MIPS,
												0, XENGINE_TEXTURE_RESOURCE_DEFAULT_UNCOMPRESSED_ALPHA_FORMAT,
												0, D3DX_DEFAULT, D3DX_DEFAULT,
												0, NULL, NULL, &texture);
		}

		_hasAlphaChannel = true;
	}
	else
	{
		if (useCompression)
		{
			result = D3DXCreateTextureFromFileEx(XEngine->getD3DDevice(), texturePath.c_str(),
												0, 0, XENGINE_TEXTURE_RESOURCE_DEFAULT_MIPS,
												0, XENGINE_TEXTURE_RESOURCE_DEFAULT_COMPRESSED_OPAQUE_FORMAT,
												0, D3DX_DEFAULT, D3DX_DEFAULT,
												0, NULL, NULL, &texture);
		}
		else
		{
			result = D3DXCreateTextureFromFileEx(XEngine->getD3DDevice(), texturePath.c_str(),
												0, 0, XENGINE_TEXTURE_RESOURCE_DEFAULT_MIPS,
												0, XENGINE_TEXTURE_RESOURCE_DEFAULT_UNCOMPRESSED_OPAQUE_FORMAT,
												0, D3DX_DEFAULT, D3DX_DEFAULT,
												0, NULL, NULL, &texture);
		}

		_hasAlphaChannel = false;
	}

	if (result != D3D_OK || texture == NULL)
	{
		string debugString = "*** ERROR *** Error loading texture resource from file '";

		debugString += texturePath;
		debugString += "'\n";

		XEngine->debugPrint(debugString);

		texture = NULL;
	}

	textureType = TT_TEXTURE2D;

	textureResources.push_back(this);
	textureResourcesListPosition = textureResources.end();

	__super::create(textureName);

	cache();
}

void CTextureResource::create(string myResourceName, IDirect3DTexture8 *myTexture)
{
	textureType = TT_TEXTURE2D;
	texture = myTexture;

	textureResources.push_back(this);
	textureResourcesListPosition = textureResources.end();

	__super::create(myResourceName);
}

void CTextureResource::create(string myResourceName, IDirect3DVolumeTexture8 *myVolumeTexture)
{
	textureType = TT_VOLUME_TEXTURE;
	volumeTexture = myVolumeTexture;

	textureResources.push_back(this);
	textureResourcesListPosition = textureResources.end();

	__super::create(myResourceName);
}

void CTextureResource::create(string myResourceName, IDirect3DCubeTexture8 *myCubeTexture)
{
	textureType = TT_TEXTURE_CUBEMAP;
	cubeTexture = myCubeTexture;

	textureResources.push_back(this);
	textureResourcesListPosition = textureResources.end();

	__super::create(myResourceName);
}

void CTextureResource::create(string myResourceName, IDirect3DTexture8 **myAnimTexture, int numFrames)
{
	textureType = TT_ANIM_TEXTURE;
	animTexture = myAnimTexture;

	numAnimTextureFrames = numFrames;

	textureResources.push_back(this);
	textureResourcesListPosition = textureResources.end();
	
	__super::create(myResourceName);
}

void CTextureResource::destroy()
{
	switch(textureType)
	{
	case TT_TEXTURE2D:

		texture->Release();

		break;

	case TT_VOLUME_TEXTURE:

		volumeTexture->Release();

		break;

	case TT_TEXTURE_CUBEMAP:

		cubeTexture->Release();

		break;

	case TT_ANIM_TEXTURE:

		for (int i = 0; i < numAnimTextureFrames; i++)
		{
			animTexture[i]->Release();
		}

		break;

	default:

		{
			string debugString = "*** ERROR *** Texture resource '";

			debugString += getResourceName();
			debugString += "' is being destroyed without having been initialized!\n";

			XEngine->debugPrint(debugString);
		}

		break;
	}

	textureResources.erase(textureResourcesListPosition);

	__super::destroy();
}

int CTextureResource::getTextureType()
{
	return textureType;
}

int CTextureResource::getNumAnimTextureFrames()
{
	return numAnimTextureFrames;
}

IDirect3DTexture8 *CTextureResource::getTexture2D()
{
	if (textureType == TT_TEXTURE2D)
	{
		return texture;
	}
	else
	{
		string debugString = "*** ERROR *** Texture resource '";

		debugString += getResourceName();
		debugString += "' is of texture type '";
		debugString += intString(textureType);
		debugString += "'! Attempted access as texture2D type!\n";

		XEngine->debugPrint(debugString);

		return NULL;
	}
}

IDirect3DVolumeTexture8 *CTextureResource::getVolumeTexture()
{
	if (textureType == TT_VOLUME_TEXTURE)
	{
		return volumeTexture;
	}
	else
	{
		string debugString = "*** ERROR *** Texture resource '";

		debugString += getResourceName();
		debugString += "' is of texture type '";
		debugString += intString(textureType);
		debugString += "'! Attempted access as volume texture type!\n";

		XEngine->debugPrint(debugString);

		return NULL;
	}
}

IDirect3DCubeTexture8 *CTextureResource::getCubeTexture()
{
	if (textureType == TT_TEXTURE_CUBEMAP)
	{
		return cubeTexture;
	}
	else
	{
		string debugString = "*** ERROR *** Texture resource '";

		debugString += getResourceName();
		debugString += "' is of texture type '";
		debugString += intString(textureType);
		debugString += "'! Attempted access as cubemap type!\n";

		XEngine->debugPrint(debugString);

		return NULL;
	}
}

IDirect3DTexture8 **CTextureResource::getAnimTexture()
{
	if (textureType == TT_ANIM_TEXTURE)
	{
		return animTexture;
	}
	else
	{
		string debugString = "*** ERROR *** Texture resource '";

		debugString += getResourceName();
		debugString += "' is of texture type '";
		debugString += intString(textureType);
		debugString += "'! Attempted access as anim texture type!\n";

		XEngine->debugPrint(debugString);

		return NULL;
	}
}

void CTextureResource::unload()
{
	// todo: implement a better caching mechanism

	switch(textureType)
	{
	case TT_TEXTURE2D:

		if (texture)
		{
			texture->Release();
		}

		break;

	case TT_VOLUME_TEXTURE:

		volumeTexture->Release();

		break;

	case TT_TEXTURE_CUBEMAP:

		cubeTexture->Release();

		break;

	case TT_ANIM_TEXTURE:

		for (int i = 0; i < numAnimTextureFrames; i++)
		{
			animTexture[i]->Release();
		}

		break;

	default:

		{
			string debugString = "*** ERROR *** Texture resource '";

			debugString += getResourceName();
			debugString += "' is being unloaded without having been initialized!\n";

			XEngine->debugPrint(debugString);
		}

		return;
	}

	__super::unload();
}

void CTextureResource::recover(bool blockOnRecover)
{
	// todo: take into account blockOnRecover and other texture types

	string texturePath = XENGINE_RESOURCES_MEDIA_PATH + getResourceName();

	bool useCompression = XENGINE_TEXTURE_RESOURCE_USE_COMPRESSED_TEXTURES; // todo: take a parameter to do this

	string extension = lowerCaseString(texturePath.substr(texturePath.length() - 3, 3));

	HRESULT result;

	if (extension == "tga")
	{
		if (useCompression)
		{
			result = D3DXCreateTextureFromFileEx(XEngine->getD3DDevice(), texturePath.c_str(),
												0, 0, XENGINE_TEXTURE_RESOURCE_DEFAULT_MIPS,
												0, XENGINE_TEXTURE_RESOURCE_DEFAULT_COMPRESSED_ALPHA_FORMAT,
												0, D3DX_DEFAULT, D3DX_DEFAULT,
												0, NULL, NULL, &texture);
		}
		else
		{
			result = D3DXCreateTextureFromFileEx(XEngine->getD3DDevice(), texturePath.c_str(),
												0, 0, XENGINE_TEXTURE_RESOURCE_DEFAULT_MIPS,
												0, XENGINE_TEXTURE_RESOURCE_DEFAULT_UNCOMPRESSED_ALPHA_FORMAT,
												0, D3DX_DEFAULT, D3DX_DEFAULT,
												0, NULL, NULL, &texture);
		}

		_hasAlphaChannel = true;
	}
	else
	{
		if (useCompression)
		{
			result = D3DXCreateTextureFromFileEx(XEngine->getD3DDevice(), texturePath.c_str(),
												0, 0, XENGINE_TEXTURE_RESOURCE_DEFAULT_MIPS,
												0, XENGINE_TEXTURE_RESOURCE_DEFAULT_COMPRESSED_OPAQUE_FORMAT,
												0, D3DX_DEFAULT, D3DX_DEFAULT,
												0, NULL, NULL, &texture);
		}
		else
		{
			result = D3DXCreateTextureFromFileEx(XEngine->getD3DDevice(), texturePath.c_str(),
												0, 0, XENGINE_TEXTURE_RESOURCE_DEFAULT_MIPS,
												0, XENGINE_TEXTURE_RESOURCE_DEFAULT_UNCOMPRESSED_OPAQUE_FORMAT,
												0, D3DX_DEFAULT, D3DX_DEFAULT,
												0, NULL, NULL, &texture);
		}

		_hasAlphaChannel = false;
	}

	if (result != D3D_OK || texture == NULL)
	{
		string debugString = "*** ERROR *** Error loading texture resource from file '";

		debugString += texturePath;
		debugString += "'\n";

		XEngine->debugPrint(debugString);

		texture = NULL;

		return;
	}

	__super::recover(blockOnRecover);
}

void CTextureResource::cache()
{
	__super::cache();
}
