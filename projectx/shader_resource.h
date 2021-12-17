#ifndef _H_SHADER_RESOURCE
#define _H_SHADER_RESOURCE

#include <xtl.h>
#include <vector>
#include <string>
#include <stdio.h>
#include <list>

#include "resource.h"

using namespace std;

struct SHADER_INFO
{
	string shaderName;
	string shaderFile;

	unsigned int fileOffset;
};

enum SHADER_WAVE_TYPE
{
	WT_NONE = 0,
	WT_SIN,
	WT_TRIANGLE,
	WT_SQUARE,
	WT_SAWTOOTH,
	WT_INVERSESAWTOOTH,
};

struct SHADER_WAVE_INFO
{
	int waveType;

	float base;			// vertical wave offset
	float amplitude;	// wave amplitude, straightforward
	float phase;		// not well documented, I'll have to wing it. fortunately, this is the least important.
	float frequency;	// number of peaks per second
};

enum SHADER_RGBA_GENTYPE
{
	SRG_IDENTITYLIGHTING = 0,
	SRG_IDENTITY,
	SRG_VERTEX,
	SRG_ONE_MINUS_VERTEX,
	SRG_LIGHTINGDIFFUSE,
	SRG_LIGHTINGSPECULAR,
	SRG_WAVE,
	SRG_ENTITY,
	SRG_ONE_MINUS_ENTITY,
	SRG_CONSTANT
};

enum SHADER_TC_MOD_TYPE
{
	STCMT_SCALE = 0,
	STCMT_TRANSFORM,
	STCMT_ROTATE,
	STCMT_SCROLL,
	STCMT_STRETCH,
	STCMT_TURB
};

struct SHADER_TC_MOD_INFO
{
	int tcModType;

	float rotSpeed;	// in degrees per second
	
	// in textures per second

	float scrollSpeedU;
	float scrollSpeedV;

	SHADER_WAVE_INFO wave;

	D3DXMATRIX matrix;
};

enum SHADER_SORT_PRIORITY
{
	SSP_PORTAL = 1,
	SSP_SKY = 2,
	SSP_OPAQUE = 3,
	SSP_BANNER = 6,
	SSP_UNDERWATER = 8,
	SSP_ADDITIVE = 9,
	SSP_NEAREST = 16
};

enum SHADER_BLEND_ARG
{
	SBA_NONE = 0,
	SBA_ONE,
	SBA_ZERO,
	SBA_DST_COLOR,           // the color in the frame buffer or the result of the previous shader stage
	SBA_ONE_MINUS_DST_COLOR,
	SBA_SRC_ALPHA,
	SBA_ONE_MINUS_SRC_ALPHA,
	SBA_SRC_COLOR,          // the local color output of the current shader stage
	SBA_ONE_MINUS_SRC_COLOR
};

enum SHADER_TEXCOORD_SOURCE
{
	STS_BASE,
	STS_LIGHTMAP,
	STS_ENVIRONMENT
};

struct TSHADER_STAGE
{
	vector<string> texturePaths;

	int  texCoordSource; // some member of the SHADER_TEXCOORD_SOURCE enum
	bool isClamped; // clamps this stage's texture coordinates

	bool  isAnimMap;
	float animFrequency; // frequency in cycles per second

	// members of the SHADER_BLEND_ARG enumeration

	int	   srcBlend; // modulates the local output of the current shader stage
	int	   dstBlend; // modulates the output of the previous shader stage

	// members of the SHADER_RGBA_GENTYPE enumeration

	int	   rgbGenType;
	int	   alphaGenType;

	DWORD constantColor;

	vector<SHADER_TC_MOD_INFO> tcMods;

	SHADER_WAVE_INFO rgbGenWave;
	SHADER_WAVE_INFO alphaGenWave;
};

struct SHADER_STAGE
{
	int numTextures;
	class CTextureResource **textures;

	int texCoordSource;
	bool isClamped;

	bool  isAnimMap;
	float animFrequency;

	int combinerStage;

	int rgbGenType;
	int alphaGenType;

	DWORD color; // this could be from the constant color and wave gens, is updated on shader update

	int numTcMods;
	SHADER_TC_MOD_INFO *tcMods;
	
	D3DXMATRIX tcMatrix;

	SHADER_WAVE_INFO rgbGenWave;
	SHADER_WAVE_INFO alphaGenWave;
};

struct SHADER_SKYBOX_TEXTURES
{
	CTextureResource *right;
	CTextureResource *left;
	CTextureResource *front;
	CTextureResource *back;
	CTextureResource *up;
	CTextureResource *down;
};

enum SHADER_TYPE
{
	ST_OPAQUE = 0,
	ST_BLENDED,
	ST_SKYBOX
};

struct SHADER_SKYBOX_INFO
{
	SHADER_SKYBOX_TEXTURES *nearBox;
	SHADER_SKYBOX_TEXTURES *farBox;

	bool hasNearBox;
	bool hasFarBox;
};

enum SHADER_VERTEX_DEFORMATION_TYPE
{
	VDT_WAVE = 0,
	VDT_NORMAL,
	VDT_BULGE,
	VDT_MOVE,
	VDT_AUTOSPRITE,
	VDT_AUTOSPRITE2,
};

struct DEFORMVERTEXES_INFO
{
	int type;

	// used for wave and normal types
	
	float div;

	// used for wave, normal, and move types
	
	SHADER_WAVE_INFO wave;

	// used for move type as direction vector for wave translation
	// used for bulge type as bulge size in each direction

	float x;
	float y;
	float z;
};

class CShaderResource : public CResource
{
public:

	int getResourceType();

	// additionally, this loads and compiles the shader file specified, loading and creating
	// any shader resources nessecary. if shaderInfo is NULL and texture is null, a SHADER NOT FOUND
	// shader is generated. if shaderinfo is null and texture isn't, default shader is generated
	// with that texture, otherwise load and compile the shader in the shaderinfo. lightmap can
	// either be a lightmap texture or null.

	void create(string resourceName, SHADER_INFO *shaderInfo);
																	   
	void destroy(); //additionally, this releases all the memory for the compiled shader

	static vector<CResource*> shaderResources; // main shader resources list

	CTextureResource *getLightmap(); // get the lightmap in use for this shader, if there is one

	bool isDynamic();	// find out whether or not this shader uses any deformvertices modifiers that affect
						// geometry shape significantly (not move or autosprites, those are fine)

	int getShaderType(); // returns shaderType

	void setLightmap(CTextureResource *lightmap);

	void activate(D3DCOLOR entityColor); // rgbgen or alphagen of type entity resolves to this value if specified
										 // else is identity

	void activate();    // bounce from the current render mode default states to the shader state

	void activate(CTextureResource *lightmap);

	void deactivate();  // bounce back

	void getSkyBoxInfo(SHADER_SKYBOX_INFO *skyBoxInfo);

	int  getNumStages();

	// first and foremost, checks if any existing loading shaders
	// match the name of the requested shader. If found, return
	// that shader resource and allow the requester to make a reference to it.
	// if it isn't found, check the SHADER_INFOs. if found in the shader infos,
	// create and compile the shader, load and create the appropriate texture
	// resources for it, and be all happy. If it ISN'T found in the shader_infos,
	// check return a reference to either the lightmapped or non-lightmapped default
	// shaders. the requester should NOT manually addref the shader
	// afterwards.

	static CShaderResource *getShader(string shaderName);

private:

	void update();

	float evaluateWave(SHADER_WAVE_INFO *wave);

	void _activate(CTextureResource *lightmap, D3DCOLOR entityColor);

	IDirect3DDevice8 *device;

	vector<CResource*>::iterator shaderResourcesListPosition; // keeps track of where in the shader resource list this resource resides

	// general shader properties

	int shaderType;			// a member of the XENGINE_SHADER_TYPE enum

	// unused unless shader type is ST_BLENDED

	int lastFrameRendered;

	int srcBlend;
	int dstBlend;

	int cullMode;			// either D3D_CULL_NONE, D3D_CULL_CW (front), or D3D_CULL_CCW (back), the default is D3D_CULL_CCW
	int sortPriority;		// a member of the SHADER_SORT_PRIORITY enumeration

	int depthFunc;			// some member of the D3DCMPFUNC enum
	bool depthWrite;

	int alphaFunc;			// some member of the d3d alpha func enumeration
	int alphaRef;			// alpha reference value for the alpha func
	bool alphaTestEnable;

	DWORD pixelShader;

	// stage properties

	int numStages;
	SHADER_STAGE *stages;

	// skybox properties

	SHADER_SKYBOX_TEXTURES nearBox;
	SHADER_SKYBOX_TEXTURES farBox;

	bool hasNearBox;
	bool hasFarBox;

	float skyBoxCurvature;

	// dynamic surface properties

	D3DXMATRIX movementMatrix;
	bool hasMovement;

	int numVertexDeformations;
	DEFORMVERTEXES_INFO *vertexDeformations;

	bool _isDynamic; // holds whether or not this shader applies any vertex deformations
};

#endif