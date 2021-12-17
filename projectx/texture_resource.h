#ifndef _H_TEXTURE_RESOURCE
#define _H_TEXTURE_RESOURCE

#include <xtl.h>
#include <string>
#include <stdio.h>
#include <vector>

#include "cache_resource.h"

#define XENGINE_TEXTURE_RESOURCE_DEFAULT_COMPRESSED_ALPHA_FORMAT	D3DFMT_DXT4
#define XENGINE_TEXTURE_RESOURCE_DEFAULT_COMPRESSED_OPAQUE_FORMAT	D3DFMT_DXT1

#define XENGINE_TEXTURE_RESOURCE_DEFAULT_UNCOMPRESSED_ALPHA_FORMAT	D3DFMT_A8R8G8B8
#define XENGINE_TEXTURE_RESOURCE_DEFAULT_UNCOMPRESSED_OPAQUE_FORMAT	D3DFMT_R5G6B5

#define XENGINE_TEXTURE_RESOURCE_DEFAULT_MIPS				0

#define XENGINE_TEXTURE_RESOURCE_USE_COMPRESSED_TEXTURES   false// true

using namespace std;

enum TEXTURE_TYPE
{
	TT_TEXTURE2D = 0,
	TT_TEXTURE_CUBEMAP,
	TT_VOLUME_TEXTURE,
	TT_ANIM_TEXTURE,
};

// loads pngs, jpgs, tgas, and dds.
// supports cubemaps, animated textures, and volume textures.
// supports non-powers of 2 texture sizes.

class CTextureResource : public CCacheResource
{
public:

	int getResourceType();

	void create(string textureName);     // additionally, this loads and compiles
									     // the texture file specified, remembering
								         // any special cubemap, volume texture/
										 // anim texture information as well as re-sizing
									     // to a power of 2 texture size if nessecary.

	void create(string myResourceName, IDirect3DTexture8 *myTexture); // creates a texture resource from a prebuilt
																    // d3d texture. I added this because I figure
																	// lightmaps are gonna need to do this.

	void create(string myResourceName, IDirect3DVolumeTexture8 *myVolumeTexture); // and of course, you never know
	void create(string myResourceName, IDirect3DCubeTexture8 *myCubeTexture);
	void create(string myResourceName, IDirect3DTexture8 **myAnimTexture, int numFrames);

	void destroy(); // additionally, this releases the texture memory

	static vector<CResource*> textureResources;	// main texture resource list

	int getTextureType();			// returns textureType
	int getNumAnimTextureFrames();  // returns numAnimTextureFrames
	bool hasAlphaChannel();

	// texture object retrieval methods

	IDirect3DTexture8		*getTexture2D();
	IDirect3DVolumeTexture8 *getVolumeTexture();
	IDirect3DCubeTexture8	*getCubeTexture();
	IDirect3DTexture8		**getAnimTexture();

	static CTextureResource *getTexture(string textureName);

	// caching mechanisms

	void unload();
	void cache();


protected:

	void recover(bool blockOnRecover);

private:

	vector<CResource*>::iterator textureResourcesListPosition; // keeps track of where in the texture resource list this resource resides

	int textureType; // a member of the TEXTURE_TYPE enumeration
	int numAnimTextureFrames;

	bool _hasAlphaChannel;

	// only one of these ought to be referenced at any given time

	IDirect3DTexture8		*texture;
	IDirect3DVolumeTexture8 *volumeTexture;
	IDirect3DCubeTexture8	*cubeTexture;
	IDirect3DTexture8		**animTexture;
};

#endif