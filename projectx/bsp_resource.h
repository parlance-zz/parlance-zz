#ifndef _H_BSP_RESOURCE
#define _H_BSP_RESOURCE

#include <xtl.h>
#include <stdio.h>
#include <list>

#undef min
#undef max

#include "resource.h"

#define XENGINE_BSP_LIGHTMAP_GAMMA	 3.0f
#define XENGINE_BSP_VERTEX_GAMMA	 1.5f

#define XENGINE_BSP_VERSION  0x2E

#define XENGINE_BSP_VERTEX_FVF			(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEX2)
#define XENGINE_BSP_MESH_VERTEX_FVF		(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)

#define XENGINE_BSP_ENABLE_CLUSTER_MERGING				false
#define XENGINE_BSP_CLUSTER_SPATIAL_MERGE_THRESHOLD		0.0f
#define XENGINE_BSP_CLUSTER_VISIBILITY_MERGE_THRESHOLD  0.0f

#define XENGINE_BSP_COLLISION_EPSILON						0.03125f
#define XENGINE_BSP_COLLISION_MAX_SLIDING_RECURSION_DEPTH	10

#define XENGINE_BSP_LIGHTMAPMIPS	2

#define XENGINE_BSP_BEZIER_PATCH_RENDER_TESS_LEVEL	2

struct BSP_VERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 normal;
	DWORD color;
	D3DXVECTOR2 texCoord;
	D3DXVECTOR2 lightmapCoord;
};

struct BSP_MESH_VERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 normal;
	DWORD color;
	D3DXVECTOR2 texCoord;
};

struct BSP_CLUSTER
{
	int numVisibleClusters;
	BSP_CLUSTER **visibleClusters;

	class CVertexBufferResource *bspVertexBuffer;
	CVertexBufferResource		*meshVertexBuffer;

	int	numSurfaces;

	struct XSURFACE			**surfaces;
	struct XSURFACE_INSTANCE *surfaceInstances;  // there should be numSurfaces of these as well

	D3DXVECTOR3 boundingSpherePos;
	float	    boundingSphereRadius;

	list<class CPhysicalEntity*> entities;
	
	//int numLights; 
	//class CLightEntity *lights[XENGINE_BSP_MAX_CLUSTER_LIGHTS];
};

// ***

enum TBSP_SURFACE_TYPE
{
	TBSP_ST_BSP = 0,
	TBSP_ST_MESH,
	TBSP_ST_BILLBOARD,
};

struct TBSP_SURFACE
{
	int textureID;
	int lightmapID;

	vector<int> bspFaces;
	int numIndices;

	int type;
	bool isShared;
};

enum TBSP_FACE_TYPE
{
	TBSP_FT_POLYGON = 1,
	TBSP_FT_BEZIER_PATCH,
	TBSP_FT_MESH_VERTICES,
	TBSP_FT_BILLBOARD
};

struct TBSP_PATCH
{
	int verts[3][3];
};

struct TBSP_VECTOR3I
{
	int x;
	int y;
	int z;
};

struct TBSP_HEADER
{
    char magic[4];              // This should always be 'IBSP'
    int version;                // This should be 0x2e for Quake 3 files
}; 

struct TBSP_LUMP
{
    int offset;
    int length;
};

struct TBSP_VERTEX
{
    D3DXVECTOR3 position;
    D3DXVECTOR2 texCoord;
    D3DXVECTOR2 lightmapCoord;
    D3DXVECTOR3 normal;
    DWORD       color;
};

struct TBSP_FACE
{
    int textureID;              // The index into the texture array 
    int effect;                 // The index for the effects (or -1 = n/a) 
    int type;                   // 1 = polygon, 2 = patch, 3 = mesh, 4 = billboard 
    int startVertIndex;         // The starting index into this face's first vertex 
    int numVerts;               // The number of vertices for this face 
    int meshVertIndex;          // The index into the first meshvertex 
    int numMeshVerts;           // The number of mesh vertices 
    int lightmapID;             // The texture index for the lightmap 
    int lMapCorner[2];          // The face's lightmap corner in the image 
    int lMapSize[2];            // The size of the lightmap section 
    D3DXVECTOR3 lMapPos;        // The 3D origin of lightmap. 
    D3DXVECTOR3 lMapVecs[2];    // The 3D space for s and t unit vectors. 
    D3DXVECTOR3 normal;         // The face normal. 
    int patchSize[2];           // The bezier patch dimensions. 
};

struct TBSP_TEXTURE
{
    char strName[64];           // The name of the texture w/o the extension 
    int flags;                  // The surface flags (unknown) 
    int contents;               // The content flags (unknown)
};

struct BSP_MATERIAL
{
	class CFootstepResource *footstepSounds;

	int flags;
	int contents;
};

struct TBSP_LIGHTMAP
{
    byte imageBits[128][128][3];   // The RGB data in a 128x128 image
};

struct TBSP_NODE
{
    int plane;                  // The index into the planes array 
    int front;                  // The child index for the front node 
    int back;                   // The child index for the back node 
    TBSP_VECTOR3I min;          // The bounding box min position. 
    TBSP_VECTOR3I max;          // The bounding box max position. 
}; 

struct TBSP_LEAF
{
    int cluster;                // The visibility cluster 
    int area;                   // The area portal 

    TBSP_VECTOR3I min;          // The bounding box min position 
    TBSP_VECTOR3I max;          // The bounding box max position 

    int leafFace;               // The first index into the face array 
    int numLeafFaces;           // The number of faces for this leaf 

    int leafBrush;              // The first index for into the brushes 
    int numLeafBrushes;         // The number of brushes for this leaf 
}; 

struct TBSP_PLANE
{
    D3DXVECTOR3 normal;
	float d; // signed distance to the origin
};

struct TBSP_VISDATA
{
    int numClusters;          // The number of clusters
    int bytesPerCluster;      // The amount of bytes (8 bits) in the cluster's bitset
    byte *bitsets;            // The array of bytes that holds the cluster bitsets                
};

struct TBSP_BRUSH
{
    int brushSide;         // The starting brush side for the brush 
    int numBrushSides;     // Number of brush sides for the brush
    int textureID;         // The texture index for the brush
}; 

struct TBSP_BRUSHSIDE 
{
    int plane;              // The plane index
    int textureID;          // The texture index
}; 

struct TBSP_SHADER
{
    char strName[64];     // The name of the shader file 
    int brushIndex;       // The brush index for this shader 
    int unknown;          // This is 99% of the time 5
}; 

struct TBSP_MODEL
{
	D3DXVECTOR3 bbMin;
	D3DXVECTOR3 bbMax;

	int firstFace;
	int numFaces;

	int firstBrush;
	int numBrushes;
};

struct TBSP_LIGHTVOLUMEENTRY
{
	BYTE ambient[3];		// ambient intensity
	BYTE directional[3];	// directional intensity
	unsigned short direction;	// encoded direction vector
};

struct BSP_LIGHTVOLUMEENTRY
{
	D3DCOLOR ambient;
	D3DCOLOR directional;
	D3DXVECTOR3 direction;
};

enum TBSP_LUMPS
{
    TBSP_LUMP_ENTITIES = 0,		// Stores player/object positions, etc...
    TBSP_LUMP_TEXTURES,			// Stores texture information
    TBSP_LUMP_PLANES,		    // Stores the splitting planes
    TBSP_LUMP_NODES,			// Stores the BSP nodes
    TBSP_LUMP_LEAFS,			// Stores the leafs of the nodes
    TBSP_LUMP_LEAFFACES,		// Stores the leaf's indices into the faces
    TBSP_LUMP_LEAFBRUSHES,		// Stores the leaf's indices into the brushes
    TBSP_LUMP_MODELS,			// Stores the info of world models
    TBSP_LUMP_BRUSHES,			// Stores the brushes info (for collision)
    TBSP_LUMP_BRUSHSIDES,		// Stores the brush surfaces info
    TBSP_LUMP_VERTICES,			// Stores the level vertices
    TBSP_LUMP_MESHVERTS,		// Stores the model vertices offsets
    TBSP_LUMP_SHADERS,			// Stores the shader files (blending, anims..)
    TBSP_LUMP_FACES,			// Stores the faces for the level
    TBSP_LUMP_LIGHTMAPS,		// Stores the lightmaps for the level
    TBSP_LUMP_LIGHTVOLUMES,		// Stores extra world lighting information
    TBSP_LUMP_VISDATA,			// Stores PVS and cluster info (visibility)
    TBSP_MAXLUMPS				// A constant to store the number of lumps
};

// ***

struct XENGINE_COLLISION_INFO
{
      float fraction;	// percent of how far we made it from the start to the destination

      D3DXVECTOR3 intersection;	// the position at which we were stopped in the trace
      D3DXVECTOR3 collisionNormal;	// the normal of the surface of which we have intersected

      bool startOut;	// if this is false we started inside of the collider
      bool allSolid;	// if this is true we started inside the collider and didn't exit it
	  bool collided;	// false if no collision took place, pretty self-explanatory

	  BSP_MATERIAL *bspMaterial;	// if this is not NULL, the collision took place with the BSP
									// and this is a pointer to a BSP_MATERIAL structure describing the
									// content properties of the brush we have intersected

	  CPhysicalEntity *collider;	// if bspMaterial is NULL, instead of colliding with the bsp we have collided
									// with another entity and this is a pointer to that entity. when this occurs
									// both the collider's and the collidee's 
};

enum XENGINE_COLLISION_TRACE_TYPE
{
	CTT_RAY = 0,   	// defines an infinitely small singularity that exists at any infinite number of points
					// on a line between the start and destination for the trace.

	CTT_BOX,			// defines an axis-aligned rectangular prism volume by a mins vector and a max vector
	CTT_SLIDING_BOX,	// the same as box, but will recursively trace in the sliding plane until the trace
						// succeeds or the maximum trace recursion depth is exceeded
};

struct XENGINE_COLLISION_TRACE_INFO
{
	int traceType;	// some member of the XENGINE_COLLISION_TRACE_TYPE enum

	D3DXVECTOR3 start;	// the starting point of the trace
	D3DXVECTOR3 end;	// the desired end point of the trace

	D3DXVECTOR3 mins;	// defines the bounding box coordinates, unused for ray traces
	D3DXVECTOR3 maxs;
};

class CBSPResource : public CResource
{
public:

	int getResourceType();

	void create(string bspPath);

	void destroy(); // additionally, this releases all data structures allocated for the bsp memory

	void render();

	static CBSPResource *world; // holds the reference to the currently loaded bsp

	int registerEntity(CPhysicalEntity *entity); // registers an entity to the bsp for this frame, which entails figuring
											// out which cluster(s) the entity is in, and adding the entity
											// to those cluster(s)'s entity lists. returns the cluster the physical
											// point of the entity lies in.
	
	// traceInfo is the original trace used in the collision calculation, entity is the physical entity to receive
	// collision events, can be NULL, and collisionInfo contains the information from the last collision that occured in the trace

	void trace(XENGINE_COLLISION_TRACE_INFO *traceInfo, XENGINE_COLLISION_INFO *collisionInfo);

private:

//	XENGINE_COLLISION_INFO *collisionInfo;
//	XENGINE_TRACE_INFO	   *traceInfo;
//	CPhysicalEntity		   *collidingEntity;

	D3DXVECTOR3 moveStart;
	D3DXVECTOR3 moveEnd;

	int moveType;
	float moveSphereRadius;

	void checkMoveByNode(int nodeIndex, float startFraction, float endFraction, D3DXVECTOR3 *start, D3DXVECTOR3 *end);
	void checkTouchBrush(TBSP_BRUSH *brush);

	void renderCluster(BSP_CLUSTER *renderCluster);

    int  isClusterVisible(int current, int test);
	void setClusterVisibility(int current, int test, bool visibility);

    int registerEntity(int nodeIndex, CPhysicalEntity *entity);

	// engine data

	IDirect3DDevice8* device;

	class CCameraEntity *currentCamera;

	// bsp data

	int numBSPLightmaps;
	class CTextureResource **lightmaps;

	int numBSPLeafs;
	TBSP_LEAF *bspLeafs;

	int numBSPClusters;
	BSP_CLUSTER *bspClusters;

	int numBSPBrushes;
	TBSP_BRUSH *bspBrushes;

	int numBSPBrushSides;
	TBSP_BRUSHSIDE *bspBrushSides;

	int numBSPLeafBrushes;
	int *bspLeafBrushes;

	int numBSPNodes;
	TBSP_NODE *bspNodes;

	int numBSPPlanes;
	TBSP_PLANE *bspPlanes;

	TBSP_VISDATA bspVisData;

	int lightVolumeSizeX;
	int lightVolumeSizeY;
	int lightVolumeSizeZ;

	BSP_LIGHTVOLUMEENTRY *lightVolume;

	int numBSPMaterials;

	BSP_MATERIAL *bspMaterials;	// corresponds to a TBSP_TEXTURE, holds certain important properties
								// of brushes when using collision detection, used for nothing more

};


#endif