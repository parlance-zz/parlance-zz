#ifndef _H_MD3_RESOURCE
#define _H_MD3_RESOURCE

#include <xtl.h>
#include <string>
#include <stdio.h>
#include <vector>

#include "resource.h"

#define XENGINE_MD3_RESOURCE_MAGIC		0x33504449
#define XENGINE_MD3_RESOURCE_VERSION	0x0F

#define XENGINE_MD3_RESOURCE_VERTEX_SCALE 64.0f

#define XENGINE_MD3_VERTEX_FVF			(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)
#define XENGINE_MD3_DYNAMIC_VERTEX_FVF	(D3DFVF_XYZ | D3DFVF_NORMAL)
#define XENGINE_MD3_TEXCOORD_FVF		(D3DFVF_TEX1)

#define XENGINE_MD3_DEFAULT_ANIMATION_SPEED	15.0f // in frames per second

using namespace std;

struct MD3_VERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 normal;
	D3DXVECTOR2 texCoord;
};

struct MD3_DYNAMIC_VERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 normal;
};

struct TMD3_HEADER
{
	int  magic;		// XENGINE_MD3_RESOURCE_MAGIC
	int	 version;	// ought to always be XENGINE_MD3_RESOURCE_VERSION
	char name[64];	// md3 name, this will go unused
	int  flags;		// unknown

	int numFrames;		// corresponds to a number of TMD3_FRAMEs in the MD3 file
	int numTags;		// corresponds to a number of TMD3_TAGs in the MD3 file
	int numSurfaces;	// and etc.
	int numSkins;		// not completely sure

	int framesLumpPos;	 // offset from the beginning of the file to find the TMD3_FRAME data
	int tagsLumpPos;	 // etc.
	int surfacesLumpPos; // etc.

	int headerSize;		 // this seems a little redundant
};

struct TMD3_FRAME
{
	D3DXVECTOR3 bbMin; // frame bounding box coords
	D3DXVECTOR3 bbMax;

	D3DXVECTOR3 localOrigin; // not completely sure, usually 0,0,0
	float		radius;		 // bounding sphere radius, I think localOrigin might be the center of the bounding sphere

	char name[16]; // name of the frame, this will probably go unused
};

struct MD3_FRAME
{
	D3DXVECTOR3 localBoundingSpherePos;
	float		boundingSphereRadius;

	struct XSURFACE *surfaces;
	struct MD3_BONE *bones;
};

struct TMD3_TAG
{
	char name[64]; // name of the tag, this will be needed but we'll strip it down to a variable length
				   // string to save memory. it will be used for linking with multi-part md3s like humanoids.

	D3DXVECTOR3 origin; // position of the bone

	float m3x3[3 * 3]; // 3x3 matrix describing the orientation of this bone. needs to be decomposed into a quaternion
					   // for interpolation.
};

struct MD3_TAG
{
	string name;
	int    index; // corresponds to the bone number for this tag
};

struct MD3_BONE
{
	D3DXVECTOR3    position;
	D3DXQUATERNION rotation;
};

struct TMD3_SURFACE
{
	int magic;		// not sure why this is replicated in each surface
	char name[64];	// name of the surface? this will probably go unused

	int	flags;		// ??? not documented

	int numFrames;	// should match numFrames in md3 header
	int numShaders;	// number of TMD3_SHADERs used in this surface
	int numVerts;	  // numFrames * numVerts = number of TMD3_VERTEXs used in this surface
	int numTriangles; // number of TMD3_TRIANGLEs used in this surface

	int triangleLumpOffset; // relative to the offset of the beginning of this particular surface
	int shaderLumpOffset;	// etc
	int texCoordLumpOffset; // there of numVerts of these
	int vertexLumpOffset;	// etc

	int surfaceSize;	// complete size of this surface including sublumps
};

struct MD3_SURFACE
{
	class CShaderResource  *shader;
	class CTextureResource *texture;

	class CVertexBufferResource *texCoords;		// unused if the md3 is not animated
	class CIndexBufferResource  *indexBuffer;
};

struct TMD3_SHADER
{
	char name[64];	  // shader name, albeit with incorrect path slashes
	int	 shaderIndex; // redundant local index of this shader to the surface
					  // there should never be more than 1 shader to a surface anyway
};

struct TMD3_TRIANGLE
{
	int v0; // vertex indices are local to the current surface this triangle is a part of
	int v1;
	int v2;
};

struct TMD3_TEXCOORD
{
	D3DXVECTOR2 texCoord;
};

struct TMD3_VERTEX
{
	short x; // to get actual coordinate value divide by XENGINE_MD3_RESOURCE_VERTEX_SCALE
	short y;
	short z;
	short normal;
};

struct MD3_ANIMATION_INFO
{
	int startFrame;
	int endFrame;
	int numLoopingFrames;

	float framesPerSecond;
};

class CMD3Resource : public CResource
{
public:

	int getResourceType();

	void create(string md3Path);

	void destroy();

	static vector<CResource*> md3Resources;	// main md3 resource list

	bool isAnimated();

	int getNumFrames();
	int getNumSurfaces();
	int getNumTags();

	MD3_FRAME     *getFrames();
	MD3_SURFACE   *getSurfaces();
	MD3_TAG		  *getTags();
	
	static CMD3Resource *getMD3(string md3Path);

private:

	vector<CResource*>::iterator md3ResourcesListPosition; // keeps track of where in the md3 resource list this resource resides

	int numFrames;		// 1 if this is a static md3
	MD3_FRAME *frames;	// each frame stores the mutable surface data in that frame, and the bone positions in that
						// frame

	int numSurfaces;		// this number of surfaces is contained in each frame, and also the number of md3 surfaces
	MD3_SURFACE *surfaces;  // stores data persistant to each surface

	int numTags;	 // this number of bones in each frame and the number of tags in the md3
	MD3_TAG *tags;	 // each tag has a name and a bone index, this is used to bind other models to a bone
};

#endif