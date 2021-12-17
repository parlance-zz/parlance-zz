#include <xtl.h>
#include <xgraphics.h>

#include "bsp_resource.h"
#include "texture_resource.h"
#include "shader_resource.h"
#include "vertexbuffer_resource.h"
#include "indexbuffer_resource.h"
#include "resource_types.h"
#include "camera_entity.h"
#include "string_help.h"
#include "xengine.h"
#include "math_help.h"
#include "physical_entity.h"
#include "player_resource.h"
#include "footstep_resource.h"

CBSPResource* CBSPResource::world = NULL;

DWORD adjustVertexGamma(DWORD color)
{
	int r = color & 0xFF;
	int g = (color & 0xFF00) >> 8;
	int b = (color & 0xFF0000) >> 16;
	int a = color & 0xFF000000;

	r = int(r * XENGINE_BSP_VERTEX_GAMMA);
	g = int(g * XENGINE_BSP_VERTEX_GAMMA);
	b = int(b * XENGINE_BSP_VERTEX_GAMMA);

	if (r > 0xFF) r = 0xFF;
	if (g > 0xFF) g = 0xFF;
	if (b > 0xFF) b = 0xFF;

	int c = b | (g << 8) | (r << 16) | a;

	return c;
}

void bezierLine(TBSP_VERTEX *out, TBSP_VERTEX *v0, TBSP_VERTEX *v1, TBSP_VERTEX *v2, float t)
{
	float B0 = (1.0f - t) * (1.0f - t);
	float B1 = 2.0f * t * (1.0f - t);
	float B2 = t * t;

	out->position.x = B0 * v0->position.x + B1 * v1->position.x + B2 * v2->position.x;
	out->position.y = B0 * v0->position.y + B1 * v1->position.y + B2 * v2->position.y;
	out->position.z = B0 * v0->position.z + B1 * v1->position.z + B2 * v2->position.z;

	out->normal.x = B0 * v0->normal.x + B1 * v1->normal.x + B2 * v2->normal.x;
	out->normal.y = B0 * v0->normal.y + B1 * v1->normal.y + B2 * v2->normal.y;
	out->normal.z = B0 * v0->normal.z + B1 * v1->normal.z + B2 * v2->normal.z;

	out->texCoord.x = B0 * v0->texCoord.x + B1 * v1->texCoord.x + B2 * v2->texCoord.x;
	out->texCoord.y = B0 * v0->texCoord.y + B1 * v1->texCoord.y + B2 * v2->texCoord.y;

	out->lightmapCoord.x = B0 * v0->lightmapCoord.x + B1 * v1->lightmapCoord.x + B2 * v2->lightmapCoord.x;
	out->lightmapCoord.y = B0 * v0->lightmapCoord.y + B1 * v1->lightmapCoord.y + B2 * v2->lightmapCoord.y;

	// todo: interpolate colors as well

	int red0 = (v0->color & 0xFF);
	int red1 = (v1->color & 0xFF);
	int red2 = (v2->color & 0xFF);

	int green0 = (v0->color & 0xFF00) >> 8;
	int green1 = (v1->color & 0xFF00) >> 8;
	int green2 = (v2->color & 0xFF00) >> 8;

	int blue0 = (v0->color & 0xFF0000) >> 16;
	int blue1 = (v1->color & 0xFF0000) >> 16;
	int blue2 = (v2->color & 0xFF0000) >> 16;

	int alpha0 = (v0->color & 0xFF000000) >> 24;
	int alpha1 = (v1->color & 0xFF000000) >> 24;
	int alpha2 = (v2->color & 0xFF000000) >> 24;

	int red   = int(B0 * red0 + B1 * red1 + B2 * red2);
	int green = int(B0 * green0 + B1 * green1 + B2 * green2);
	int blue  = int(B0 * blue0 + B1 * blue1 + B2 * blue2);
	int alpha = int(B0 * alpha0 + B1 * alpha1 + B2 * alpha2);

	DWORD color = red | (green << 8) | (blue << 16) | (alpha << 24);

	out->color = adjustVertexGamma(color);
}

void bezierPatch(TBSP_VERTEX *out, TBSP_PATCH *in, float u, float v, TBSP_VERTEX *bspVertices)
{
	TBSP_VERTEX v0;
	TBSP_VERTEX v1;
	TBSP_VERTEX v2;

	bezierLine(&v0, &bspVertices[in->verts[0][0]], &bspVertices[in->verts[1][0]], &bspVertices[in->verts[2][0]], u);
	bezierLine(&v1, &bspVertices[in->verts[0][1]], &bspVertices[in->verts[1][1]], &bspVertices[in->verts[2][1]], u);
	bezierLine(&v2, &bspVertices[in->verts[0][2]], &bspVertices[in->verts[1][2]], &bspVertices[in->verts[2][2]], u);

	bezierLine(out, &v0, &v1, &v2, v);
}

int getNumPatchVerts(int numControlPoints)
{
	int result = numControlPoints;

	for (int i = 0; i < XENGINE_BSP_BEZIER_PATCH_RENDER_TESS_LEVEL; i++)
	{
		result = result * 2 - 1;
	}

	return result;
}

int CBSPResource::getResourceType()
{
	return RT_BSP_RESOURCE;
}

/*
bool CBSPResource::checkRayMove(D3DXVECTOR3 *start, D3DXVECTOR3 *end, XENGINE_COLLISION_INFO *collisionInfo)
{
    this->collisionInfo.startOut = true;
    this->collisionInfo.allSolid = false;
    this->collisionInfo.fraction = 1.0f;

    moveStart = (*start);
    moveEnd   = (*end);

    moveType = CMT_RAY;
    moveSphereRadius = 0.0f;

	checkMoveByNode(0, 0.0f, 1.0f, &moveStart, &moveEnd);

	// todo: check out all our clusters we were in and check for collisions with physical entities
	// that are collidable in those clusters

	if (this->collisionInfo.fraction == 1.0f)
	{
		this->collisionInfo.endPoint = (*end);
	}
    else
	{
		this->collisionInfo.endPoint = (*start) + ((*end) - (*start)) * this->collisionInfo.fraction;
	}

	(*collisionInfo) = this->collisionInfo;

	return this->collisionInfo.collided;
}

void CBSPResource::checkMoveByNode(int nodeIndex, float startFraction, float endFraction, D3DXVECTOR3 *start, D3DXVECTOR3 *end)
{
    if (startFraction > collisionInfo.fraction) 
	{
		return;
	}
  
    if (nodeIndex < 0)
	{
		TBSP_LEAF *leaf = &bspLeafs[~nodeIndex]; 
		
		// todo: make this leaf's cluster as a cluster we need to check

        for (int i = 0; i < leaf->numLeafBrushes; i++)
        {
			TBSP_BRUSH *brush = &bspBrushes[bspLeafBrushes[leaf->leafBrush + i]];
  
            if (brush->numBrushSides > 0 && (bspMaterials[brush->textureID].contents & 1))
            {
				checkTouchBrush(brush);
            }
        }
  
        return;
	}

	TBSP_NODE  *node  = &bspNodes[nodeIndex];
	TBSP_PLANE *plane = &bspPlanes[node->plane];

	float startDistance =  D3DXVec3Dot(&plane->normal, start) - plane->d;
	float endDistance   =  D3DXVec3Dot(&plane->normal, end)   - plane->d;

    if (startDistance >= moveSphereRadius && endDistance >= moveSphereRadius)
    {     
        checkMoveByNode(node->front, startFraction, endFraction, start, end);
    }
    else if (startDistance < -moveSphereRadius && endDistance < -moveSphereRadius)
    {
        checkMoveByNode(node->back, startFraction, endFraction, start, end);
    }
    else
    {
        int side1;
		int side2;

        float fraction1;
		float fraction2;

        D3DXVECTOR3 middle;

        if (startDistance < endDistance)
        {
			side1 = node->back;
            side2 = node->front;  

            float inverseDistance = 1.0f / (startDistance - endDistance);

            fraction1 = (startDistance - XENGINE_BSP_COLLISION_EPSILON - moveSphereRadius) * inverseDistance;
            fraction2 = (startDistance + XENGINE_BSP_COLLISION_EPSILON + moveSphereRadius) * inverseDistance;
		}
        else if (endDistance < startDistance)
        {
            side1 = node->front;
            side2 = node->back;

            float inverseDistance = 1.0f / (startDistance - endDistance);

            fraction1 = (startDistance + XENGINE_BSP_COLLISION_EPSILON + moveSphereRadius) * inverseDistance;
            fraction2 = (startDistance - XENGINE_BSP_COLLISION_EPSILON - moveSphereRadius) * inverseDistance;
        }
        else
        {
            side1 = node->front;
            side2 = node->back;

            fraction1 = 1.0f;
            fraction2 = 0.0f;
        }

        if (fraction1 < 0.0f) fraction1 = 0.0f;
        else if (fraction1 > 1.0f) fraction1 = 1.0f;

        if (fraction2 < 0.0f) fraction2 = 0.0f;
        else if (fraction2 > 1.0f) fraction2 = 1.0f;

        middle = (*start) + ((*end) - (*start)) * fraction1;
        float middleFraction = startFraction + (endFraction - startFraction) * fraction1;

        checkMoveByNode(side1, startFraction, middleFraction, start, &middle);

        middle = (*start) + ((*end) - (*start)) * fraction2;
        middleFraction = startFraction + (endFraction - startFraction) * fraction2;

        checkMoveByNode(side2, middleFraction, endFraction, &middle, end);
    }
}

void CBSPResource::checkTouchBrush(TBSP_BRUSH *brush)
{
    float startFraction = -1.0f;
    float endFraction = 1.0f;

    bool startsOut = false;
    bool endsOut = false;

    D3DXVECTOR3 candidateToHitNormal;

    for (int i = 0; i < brush->numBrushSides; i++)
    {
        TBSP_BRUSHSIDE *brushSide = &bspBrushSides[brush->brushSide + i];
		TBSP_PLANE     *plane     = &bspPlanes[brushSide->plane];

        float startDistance =  D3DXVec3Dot(&plane->normal, &moveStart) - plane->d - moveSphereRadius;
        float endDistance   =  D3DXVec3Dot(&plane->normal, &moveEnd)   - plane->d - moveSphereRadius;

        if (startDistance > 0.0f)
		{
			startsOut = true;
		}

        if (endDistance > 0.0f)
		{
			endsOut = true;
		}

        if (startDistance > 0.0f && endDistance > 0.0f)
        {
			return;
        }

        if (startDistance <= 0.0f && endDistance <= 0.0f)
        {
			continue;
        }
    
        if (startDistance > endDistance)
        {  
            float fraction = (startDistance - XENGINE_BSP_COLLISION_EPSILON) / (startDistance - endDistance);

            if (fraction > startFraction)
			{
                startFraction = fraction;

                candidateToHitNormal = plane->normal;
            }
        }
        else
        {   
            float fraction = (startDistance + XENGINE_BSP_COLLISION_EPSILON) / (startDistance - endDistance);

            if (fraction < endFraction)
			{
                endFraction = fraction;
			}
        }
    }

    if (!startsOut)
    {
        collisionInfo.startOut = false;

        if (!endsOut)
		{
			collisionInfo.allSolid = true;
		}

        return;
    }

    if (startFraction < endFraction)
    {
        if (startFraction > -1.0f && startFraction < collisionInfo.fraction)
        {
            if (startFraction < 0.0f)
			{
				startFraction = 0.0f;
			}

            collisionInfo.fraction = startFraction;
            collisionInfo.collisionNormal = candidateToHitNormal;
        }
    }
}
*/

/*
void CBSPResource::trace(XENGINE_COLLISION_TRACE_INFO *traceInfo, CPhysicalEntity *entity, XENGINE_COLLISION_INFO *collisionInfo)
{
	this->traceInfo = traceInfo;
	this->collisionInfo = collisionInfo;
	
	collidingEntity = entity;

	traceByNode(0, 0.0f, 1.0f);

}
*/

void CBSPResource::create(string bspPath)
{
	// **** TEMP ******

	device = XEngine->getD3DDevice();

	if (world)
	{
		XEngine->debugPrint("*** ERROR *** BSP already loaded. Previous BSP must be released before loading another!\n");

		return;
	}

	string filePath = XENGINE_RESOURCES_MEDIA_PATH;
	filePath += bspPath;

    FILE *bspFile = fopen(filePath.c_str(), "rb");

	// obviously, file not found, pretty critical error

	if (!bspFile)
	{
		string debugString = "*** ERROR *** BSP file not found '";

		debugString += bspPath;
		debugString += "'!\n";

		XEngine->debugPrint(debugString);

		return;
	}
	else
	{
		string debugString = "Loading bsp file '";

		debugString += bspPath;
		debugString += "'...\n";

		XEngine->debugPrint(debugString);
	}

	// validate BSP file

	TBSP_HEADER bspHeader;

	fread(&bspHeader, 1, sizeof(TBSP_HEADER), bspFile);

	string magicString = bspHeader.magic;

	if (magicString != "IBSP.") // hmmm, are we going to have string equality issues? fuck..
	{
		XEngine->debugPrint("*** ERROR *** Invalid BSP header found!\n");

		fclose(bspFile);

		return;
	}
	else if (bspHeader.version != XENGINE_BSP_VERSION)
	{
		XEngine->debugPrint("*** ERROR *** Incorrect BSP version found!\n");

		fclose(bspFile);

		return;
	}

	// get our lump table. this is like the index for the rest of the bsp file data

	TBSP_LUMP bspLumps[TBSP_MAXLUMPS] = {0};

    fread(&bspLumps, TBSP_MAXLUMPS, sizeof(TBSP_LUMP), bspFile);

	// allocate storage space for our different bsp lump types and read them in from the file

	// bsp verts

    int numBSPVerts = bspLumps[TBSP_LUMP_VERTICES].length / sizeof(TBSP_VERTEX);
    TBSP_VERTEX *bspVertices = new TBSP_VERTEX[numBSPVerts];

	fseek(bspFile, bspLumps[TBSP_LUMP_VERTICES].offset, SEEK_SET);
	fread(bspVertices, numBSPVerts, sizeof(TBSP_VERTEX), bspFile);

	// bsp faces

    int numBSPFaces = bspLumps[TBSP_LUMP_FACES].length / sizeof(TBSP_FACE);
    TBSP_FACE *bspFaces = new TBSP_FACE[numBSPFaces];

    fseek(bspFile, bspLumps[TBSP_LUMP_FACES].offset, SEEK_SET);
    fread(bspFaces, numBSPFaces, sizeof(TBSP_FACE), bspFile);

	// bsp textures

    int numBSPTextures = bspLumps[TBSP_LUMP_TEXTURES].length / sizeof(TBSP_TEXTURE);
    TBSP_TEXTURE *bspTextures = new TBSP_TEXTURE[numBSPTextures];

    fseek(bspFile, bspLumps[TBSP_LUMP_TEXTURES].offset, SEEK_SET);    
    fread(bspTextures, numBSPTextures, sizeof(TBSP_TEXTURE), bspFile);

	numBSPMaterials = numBSPTextures;

	bspMaterials = new BSP_MATERIAL[numBSPMaterials];

	for (int m = 0; m < numBSPMaterials; m++)
	{
		// todo: check flags to find correct foot step sound

		string soundPath = XENGINE_PLAYERRESOURCE_FOOTSTEPSOUNDS_PATH;
		soundPath += "step";

//		bspMaterials[m].footstepSounds = CFootstepResource::getFootstep(soundPath);

		bspMaterials[m].contents = bspTextures[m].contents;
		bspMaterials[m].flags	 = bspTextures[m].flags;
	}

	// bsp lightmaps

    numBSPLightmaps = bspLumps[TBSP_LUMP_LIGHTMAPS].length / sizeof(TBSP_LIGHTMAP);
    TBSP_LIGHTMAP *bspLightmaps = new TBSP_LIGHTMAP[numBSPLightmaps];
	
	fseek(bspFile, bspLumps[TBSP_LUMP_LIGHTMAPS].offset, SEEK_SET);
    fread(bspLightmaps, numBSPLightmaps, sizeof(TBSP_LIGHTMAP), bspFile);	

	// bsp brushes

    numBSPBrushes = bspLumps[TBSP_LUMP_BRUSHES].length / sizeof(TBSP_BRUSH);
    bspBrushes    = new TBSP_BRUSH[numBSPBrushes];

    fseek(bspFile, bspLumps[TBSP_LUMP_BRUSHES].offset, SEEK_SET);
    fread(bspBrushes, numBSPBrushes, sizeof(TBSP_BRUSH), bspFile);

	// bsp brush sides

	numBSPBrushSides = bspLumps[TBSP_LUMP_BRUSHSIDES].length / sizeof(TBSP_BRUSHSIDE);
    bspBrushSides    = new TBSP_BRUSHSIDE[numBSPBrushSides];

    fseek(bspFile, bspLumps[TBSP_LUMP_BRUSHSIDES].offset, SEEK_SET);
    fread(bspBrushSides, numBSPBrushSides, sizeof(TBSP_BRUSHSIDE), bspFile);

	// bsp shaders

    int numBSPShaders = bspLumps[TBSP_LUMP_SHADERS].length / sizeof(TBSP_SHADER);
    TBSP_SHADER *bspShaders = new TBSP_SHADER[numBSPShaders];

    fseek(bspFile, bspLumps[TBSP_LUMP_SHADERS].offset, SEEK_SET);
    fread(bspShaders, numBSPShaders, sizeof(TBSP_SHADER), bspFile);

	// bsp leaf faces

	int numBSPLeafFaces	= bspLumps[TBSP_LUMP_LEAFFACES].length / sizeof(int);
	int *bspLeafFaces = new int[numBSPLeafFaces];

    fseek(bspFile, bspLumps[TBSP_LUMP_LEAFFACES].offset, SEEK_SET);
    fread(bspLeafFaces, numBSPLeafFaces, sizeof(int), bspFile);

	// bsp leaf brushes

	numBSPLeafBrushes = bspLumps[TBSP_LUMP_LEAFBRUSHES].length / sizeof(int);
	bspLeafBrushes	= new int[numBSPLeafBrushes];

	fseek(bspFile, bspLumps[TBSP_LUMP_LEAFBRUSHES].offset, SEEK_SET);
	fread(bspLeafBrushes, numBSPLeafBrushes, sizeof(int), bspFile);

	// bsp mesh verts

	int numBSPMeshVerts	= bspLumps[TBSP_LUMP_MESHVERTS].length / sizeof(int);
	int *bspMeshVerts = new int[numBSPMeshVerts];

	fseek(bspFile, bspLumps[TBSP_LUMP_MESHVERTS].offset, SEEK_SET);
	fread(bspMeshVerts, numBSPMeshVerts, sizeof(int), bspFile);

	// bsp nodes

    numBSPNodes = bspLumps[TBSP_LUMP_NODES].length / sizeof(TBSP_NODE);
    bspNodes = new TBSP_NODE[numBSPNodes];

    fseek(bspFile, bspLumps[TBSP_LUMP_NODES].offset, SEEK_SET);
    fread(bspNodes, numBSPNodes, sizeof(TBSP_NODE), bspFile);

    for (int n = 0; n < numBSPNodes; n++)
    {
        float temp = (float)bspNodes[n].min.y;

        bspNodes[n].min.y = bspNodes[n].min.z;
        bspNodes[n].min.z = int(-temp);
		bspNodes[n].min.x = -bspNodes[n].min.x;

        temp = (float)bspNodes[n].max.y;

        bspNodes[n].max.y = bspNodes[n].max.z;
        bspNodes[n].max.z = int(-temp);
		bspNodes[n].max.x = -bspNodes[n].max.x;
    }

	// bsp leafs

	numBSPLeafs = bspLumps[TBSP_LUMP_LEAFS].length / sizeof(TBSP_LEAF);
    bspLeafs = new TBSP_LEAF[numBSPLeafs];

	fseek(bspFile, bspLumps[TBSP_LUMP_LEAFS].offset, SEEK_SET);
    fread(bspLeafs, numBSPLeafs, sizeof(TBSP_LEAF), bspFile);

	for (int l = 0; l < numBSPLeafs; l++)
	{
		int temp = -bspLeafs[l].min.y;

		bspLeafs[l].min.x = -bspLeafs[l].min.x;
		bspLeafs[l].min.y = +bspLeafs[l].min.z;
		bspLeafs[l].min.z = temp;

		temp = -bspLeafs[l].max.y;

		bspLeafs[l].max.x = -bspLeafs[l].max.x;
		bspLeafs[l].max.y = +bspLeafs[l].max.z;
		bspLeafs[l].max.z = temp;
	}

	// bsp planes

    numBSPPlanes = bspLumps[TBSP_LUMP_PLANES].length / sizeof(TBSP_PLANE);
    bspPlanes = new TBSP_PLANE[numBSPPlanes];

	fseek(bspFile, bspLumps[TBSP_LUMP_PLANES].offset, SEEK_SET);
	fread(bspPlanes, numBSPPlanes, sizeof(TBSP_PLANE), bspFile);

    for (int p = 0; p < numBSPPlanes; p++)
    {
        float temp = bspPlanes[p].normal.y;

        bspPlanes[p].normal.y = bspPlanes[p].normal.z;
        bspPlanes[p].normal.z = -temp;
		bspPlanes[p].normal.x = -bspPlanes[p].normal.x;
    }

	// bsp models

	int numBSPModels = bspLumps[TBSP_LUMP_MODELS].length / sizeof(TBSP_MODEL);
	TBSP_MODEL *bspModels = new TBSP_MODEL[numBSPModels];

	fseek(bspFile, bspLumps[TBSP_LUMP_MODELS].offset, SEEK_SET);
	fread(bspModels, numBSPModels, sizeof(TBSP_MODEL), bspFile);

	for (int m = 0; m < numBSPModels; m++)
	{
		float temp = bspModels[m].bbMin.y;

		bspModels[m].bbMin.y = bspModels[m].bbMin.z;
		bspModels[m].bbMin.z = -temp;
		bspModels[m].bbMin.x = -bspModels[m].bbMin.x;

		temp = bspModels[m].bbMax.y;

		bspModels[m].bbMax.y = bspModels[m].bbMax.z;
		bspModels[m].bbMax.z = -temp;
		bspModels[m].bbMax.x = -bspModels[m].bbMax.x;
	}

	// light volumes

	lightVolumeSizeX = int(floor(bspModels[0].bbMax.x / 64)  - ceil(bspModels[0].bbMin.x /  64)) + 1;
	lightVolumeSizeY = int(floor(bspModels[0].bbMax.y / 128) - ceil(bspModels[0].bbMin.y / 128)) + 1;
	lightVolumeSizeZ = int(floor(bspModels[0].bbMax.z / 64)  - ceil(bspModels[0].bbMin.z /  64)) + 1;

	int numLightVolumeEntries = bspLumps[TBSP_LUMP_LIGHTVOLUMES].length / sizeof(TBSP_LIGHTVOLUMEENTRY);
	TBSP_LIGHTVOLUMEENTRY *tmpLightVolume = new TBSP_LIGHTVOLUMEENTRY[numLightVolumeEntries];

	lightVolume = new BSP_LIGHTVOLUMEENTRY[numLightVolumeEntries];

	for (int l = 0; l < numLightVolumeEntries; l++)
	{
		lightVolume[l].ambient     = tmpLightVolume[l].ambient[0] | (tmpLightVolume[l].ambient[1] << 8) | (tmpLightVolume[l].ambient[0] << 16) | 0xFF000000;
		lightVolume[l].directional = tmpLightVolume[l].directional[0] | (tmpLightVolume[l].directional[1] << 8) | (tmpLightVolume[l].directional[0] << 16) | 0xFF000000;

		float latitude   = (tmpLightVolume[l].direction & 0xFF) * 2.0f * D3DX_PI / 255.0f;
		float longtitude = ((tmpLightVolume[l].direction >> 8) & 0xFF) * 2.0f * D3DX_PI / 255.0f;

		float tmpDirectionX = float(cos(latitude) * sin(longtitude));
		float tmpDirectionY = float(sin(latitude) * sin(longtitude));
		float tmpDirectionZ = float(cos(longtitude));

		lightVolume[l].direction.x = -tmpDirectionX;
		lightVolume[l].direction.y = +tmpDirectionY;
		lightVolume[l].direction.z = -tmpDirectionZ;
	}

	delete [] tmpLightVolume;

	// bsp vis data

    fseek(bspFile, bspLumps[TBSP_LUMP_VISDATA].offset, SEEK_SET);

    if(bspLumps[TBSP_LUMP_VISDATA].length) 
    {
        fread(&bspVisData.numClusters,     1, sizeof(int), bspFile);
        fread(&bspVisData.bytesPerCluster, 1, sizeof(int), bspFile);

        int visDataSize = bspVisData.numClusters * bspVisData.bytesPerCluster;
        bspVisData.bitsets = new byte[visDataSize];

        fread(bspVisData.bitsets, 1, sizeof(byte) * visDataSize, bspFile);
    }
    else
	{
		// todo : consider whether this will be a critical error that will terminate bsp loading

		memset(&bspVisData, 0, sizeof(TBSP_VISDATA));

		string debugString = "*** ERROR *** BSP vis data not found in BSP file '";

		debugString += bspPath;
		debugString += "'!\n";

		XEngine->debugPrint(debugString);
	}

	// bsp entities

	char *buff = new char[bspLumps[TBSP_LUMP_ENTITIES].length];

	fseek(bspFile, bspLumps[TBSP_LUMP_ENTITIES].offset, SEEK_SET);
	fread(buff, 1, bspLumps[TBSP_LUMP_ENTITIES].length, bspFile);

	// annnnd we're done. reading the file that is.

    fclose(bspFile);

	// at last the info has been raped from the file, now to compile this into optimized data structures
	// that will actually be used by the engine and store this

	// now, to list the important steps...

	// load the lightmaps into texture memory

	lightmaps = new CTextureResource*[numBSPLightmaps];

    for(int i = 0; i < numBSPLightmaps ; i++)
    {
		// adjust the lightmap gamma

		byte *lightmap = (byte*)bspLightmaps[i].imageBits;

		for (int b = 0; b < 128 * 128 * 3; b++)
		{
			float light = float(lightmap[b]) * XENGINE_BSP_LIGHTMAP_GAMMA;

			if (light > 255.0f)
			{
				light = 255.0f;
			}

			lightmap[b] = (byte)light;
		}

		// convert lightmap to ARGB format

		byte *ARGBLightmap = new byte[128 * 128 * 4];

		for (int p = 0; p < (128 * 128); p++)
		{
			ARGBLightmap[(p << 0x02) + 0] = lightmap[p * 3 + 2];
			ARGBLightmap[(p << 0x02) + 1] = lightmap[p * 3 + 1];
			ARGBLightmap[(p << 0x02) + 2] = lightmap[p * 3 + 0];

			ARGBLightmap[(p << 0x02) + 3] = 0xFF;
		}

		IDirect3DTexture8 *myLightmap;

		XEngine->getD3DDevice()->CreateTexture(128, 128, XENGINE_BSP_LIGHTMAPMIPS, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &myLightmap);

		D3DLOCKED_RECT lockedRect = {0};

		myLightmap->LockRect(0, &lockedRect, NULL, NULL); // last param use to be D3DLOCK_NOOVERWRITE
														  // probably should be NULL though

		XGSwizzleRect(ARGBLightmap, 0, NULL, lockedRect.pBits, 128, 128, NULL, 4);

		myLightmap->UnlockRect(0);

		delete [] ARGBLightmap;

		//device->CreateTexture(4, 4, 1, NULL, D3DFMT_X8R8G8B8, NULL, &myLightmap);

		D3DXFilterTexture(myLightmap, NULL, 0, D3DX_FILTER_BOX);

		string resourceName = "lightmap";
		resourceName += intString(i);

		lightmaps[i] = new CTextureResource;

		lightmaps[i]->create(resourceName, myLightmap);
	}

	// todo: woah, this actually causes problems for some reason =|

	/*
	for (int c = 0; c < bspVisData.numClusters; c++)
	{
		for (int cluster = 0; cluster < bspVisData.numClusters; cluster++)
		{
			if (c != cluster)
			{
				int tmp = isClusterVisible(cluster, c);

				if (!isClusterVisible(c, cluster))
				{
					setClusterVisibility(cluster, c, false);
				}

				if (!tmp)
				{
					setClusterVisibility(c, cluster, false);
				}
			}
		}
	}
	*/

	int numRealClusters = bspVisData.numClusters; // todo: temp

	if (XENGINE_BSP_ENABLE_CLUSTER_MERGING)
	{
		int *mergedCluster = new int[bspVisData.numClusters];

		for (int c = 0; c < bspVisData.numClusters; c++)
		{
			mergedCluster[c] = -1;
		}

		for (int cluster = 0; cluster < bspVisData.numClusters; cluster++)
		{	
			for (int c = cluster + 1; c < bspVisData.numClusters; c++)
			{
				int numVisibleTogether = 0;
				int numVisibleTotal = bspVisData.numClusters;

				int numSharedVisibleClusters = 0;
				int numUniqueVisibleClusters = 0;

				for (int test = 0; test < bspVisData.numClusters; test++)
				{
					if (test != c && test != cluster)
					{
						if (mergedCluster[test] >= 0)
						{
							numVisibleTotal--;
						}
						else
						{
							if (isClusterVisible(test, c))
							{
								numUniqueVisibleClusters++;

								if (isClusterVisible(test, cluster))
								{
									numVisibleTogether++;

									numSharedVisibleClusters++;
								}
							}
							else if (isClusterVisible(test, cluster))
							{
								numUniqueVisibleClusters++;
							}
						}
					}
				}

				float spatialLocality = (float)numVisibleTogether / (float)(numVisibleTotal);
				float visibleLocality = (float)numSharedVisibleClusters / (float)numUniqueVisibleClusters;

				if (spatialLocality >= XENGINE_BSP_CLUSTER_SPATIAL_MERGE_THRESHOLD &&
					visibleLocality >= XENGINE_BSP_CLUSTER_VISIBILITY_MERGE_THRESHOLD)
				{
					for (int test = 0; test < bspVisData.numClusters; test++)
					{
						if (test != c && test != cluster)
						{
							if (isClusterVisible(cluster, test))
							{
								setClusterVisibility(c, test, true);
								setClusterVisibility(test, c, true);

								setClusterVisibility(cluster, test, false);
								setClusterVisibility(test, cluster, false);
							}
						}
					}

					mergedCluster[cluster] = c;

					for (int l = 0; l < numBSPLeafs; l++)
					{
						if (bspLeafs[l].cluster == cluster)
						{
							bspLeafs[l].cluster = c;
						}
					}

					numRealClusters--; // todo: temp

					break;
				}
			}
		}

		for (int c = 0; c < bspVisData.numClusters; c++)
		{
			if (mergedCluster[c] >= 0)
			{
				for (int cluster = 0; cluster < bspVisData.numClusters; cluster++)
				{
					setClusterVisibility(c, cluster, false);
					setClusterVisibility(cluster, c, false);
				}
			}
		}

		delete [] mergedCluster;
	}

	int numTotalSurfaces = 0; // todo: temp

	numBSPClusters = bspVisData.numClusters;
	bspClusters =  new BSP_CLUSTER[numBSPClusters];

	int *faceClusters	   = new int[numBSPFaces];
	int *faceNumReferences = new int[numBSPFaces];

	XSURFACE **faceBSPSurfaces  = new XSURFACE*[numBSPFaces];
	XSURFACE **faceMeshSurfaces = new XSURFACE*[numBSPFaces];

	for (int f = 0; f < numBSPFaces; f++)
	{
		faceClusters[f] = -1;
		faceNumReferences[f] = 0;

		faceBSPSurfaces[f] = NULL;
		faceMeshSurfaces[f] = NULL;

		for (int l = 0; l < numBSPLeafs; l++)
		{
			for (int face = bspLeafs[l].leafFace; face < bspLeafs[l].leafFace + bspLeafs[l].numLeafFaces; face++)
			{
				if (bspLeafFaces[face] == f)
				{
					faceNumReferences[f]++;
				}
			}
		}
	}

	for (int c = 0; c < numBSPClusters; c++)
	{
		vector<int> visibleClusters;

		for (int cluster = 0; cluster < numBSPClusters; cluster++)
		{
			if (c != cluster)
			{
				if (isClusterVisible(c, cluster))
				{
					visibleClusters.push_back(cluster);
				}
			}
		}

		bspClusters[c].numVisibleClusters = (int)visibleClusters.size();

		if (bspClusters[c].numVisibleClusters)
		{
			bspClusters[c].visibleClusters    = new BSP_CLUSTER*[bspClusters[c].numVisibleClusters];

			for (int cluster = 0; cluster < bspClusters[c].numVisibleClusters; cluster++)
			{
				bspClusters[c].visibleClusters[cluster] = &bspClusters[visibleClusters[cluster]];
			}

			visibleClusters.clear();
		}
		else
		{
			bspClusters[c].visibleClusters = NULL;
		}

		// used to get the bounding sphere middle position for the cluster

		D3DXVECTOR3 bbMin;
		D3DXVECTOR3 bbMax;

		bool hasMins = false;
		bool hasMaxs = false;

		vector<int> tmpFaces;

		vector<XSURFACE*> sharedBSPSurfaces;
		vector<XSURFACE*> sharedMeshSurfaces;

		for (int l = 0; l < numBSPLeafs; l++)
		{
			if (bspLeafs[l].cluster == c)
			{
				bool clusterLeaf = true;//false;

				for (int f = bspLeafs[l].leafFace; f < bspLeafs[l].leafFace + bspLeafs[l].numLeafFaces; f++)
				{
					// TODO: WARNING !! TEMP!

					//if (!bspFaces[bspLeafFaces[f]].numVerts) //  || !bspFaces[bspLeafFaces[f]].numMeshVerts
					//{
					//	continue;
					//}

					bool faceFound = false;

					for (int face = 0; face < (int)tmpFaces.size(); face++)
					{
						if (tmpFaces[face] == bspLeafFaces[f])
						{
							faceFound = true;

							break;
						}
					}

					if (!faceFound)
					{
						if (faceClusters[bspLeafFaces[f]] >= 0)
						{
							if (faceBSPSurfaces[bspLeafFaces[f]])
							{
								bool sharedSurfaceFound = false;

								for (int s = 0; s < (int)sharedBSPSurfaces.size(); s++)
								{
									if (sharedBSPSurfaces[s] == faceBSPSurfaces[bspLeafFaces[f]])
									{
										sharedSurfaceFound = true;

										break;
									}
								}

								if (!sharedSurfaceFound)
								{
									sharedBSPSurfaces.push_back(faceBSPSurfaces[bspLeafFaces[f]]);
								}
							}

							if (faceMeshSurfaces[bspLeafFaces[f]])
							{
								bool sharedSurfaceFound = false;

								for (int s = 0; s < (int)sharedMeshSurfaces.size(); s++)
								{
									if (sharedMeshSurfaces[s] == faceMeshSurfaces[bspLeafFaces[f]])
									{
										sharedSurfaceFound = true;
										
										break;
									}
								}

								if (!sharedSurfaceFound)
								{
									sharedMeshSurfaces.push_back(faceMeshSurfaces[bspLeafFaces[f]]);
								}
							}
						}
						else
						{
							faceClusters[bspLeafFaces[f]] = 100;

							tmpFaces.push_back(bspLeafFaces[f]);
						}

						//clusterLeaf = true;
					}
				}

				if (clusterLeaf)
				{
					if (!hasMins)
					{
						bbMin.x = (float)bspLeafs[l].min.x;
						bbMin.y = (float)bspLeafs[l].min.y;
						bbMin.z = (float)bspLeafs[l].min.z;

						hasMins = true;
					}
					
					if (bspLeafs[l].min.x < bbMin.x) bbMin.x = (float)bspLeafs[l].min.x;
					if (bspLeafs[l].min.y < bbMin.y) bbMin.y = (float)bspLeafs[l].min.y;
					if (bspLeafs[l].min.z < bbMin.z) bbMin.z = (float)bspLeafs[l].min.z;
					
					if (!hasMaxs)
					{
						bbMax.x = (float)bspLeafs[l].max.x;
						bbMax.y = (float)bspLeafs[l].max.y;
						bbMax.z = (float)bspLeafs[l].max.z;

						hasMaxs = true;
					}

					if (bspLeafs[l].max.x > bbMax.x) bbMax.x = (float)bspLeafs[l].max.x;
					if (bspLeafs[l].max.y > bbMax.y) bbMax.y = (float)bspLeafs[l].max.y;
					if (bspLeafs[l].max.z > bbMax.z) bbMax.z = (float)bspLeafs[l].max.z;
				}
			}
		}
		
		bspClusters[c].boundingSpherePos.x = (bbMax.x + bbMin.x) / 2.0f;
		bspClusters[c].boundingSpherePos.y = (bbMax.y + bbMin.y) / 2.0f;
		bspClusters[c].boundingSpherePos.z = (bbMax.z + bbMin.z) / 2.0f;
		
		D3DXVECTOR3 deltaVec = bbMax - bspClusters[c].boundingSpherePos;
		bspClusters[c].boundingSphereRadius = D3DXVec3Length(&deltaVec);

		if (!hasMaxs || !hasMins)
		{
			int x = 5;
		}

		bspClusters[c].numSurfaces  = 0;

		int numBSPVertices  = 0;
		int numMeshVertices = 0;

		int numBSPSurfaces  = 0;
		int numMeshSurfaces = 0;

		if (!tmpFaces.size())
		{
			continue;
		}

		vector<TBSP_SURFACE*> tmpSurfaces;

		for (int face = 0; face < (int)tmpFaces.size(); face++)
		{
			int f = tmpFaces[face];

			int surfaceType = NULL;

			switch(bspFaces[f].type)
			{
			case TBSP_FT_POLYGON:

				{
					surfaceType = TBSP_ST_BSP;

					numBSPVertices += bspFaces[f].numVerts;
				}

				break;

			case TBSP_FT_BEZIER_PATCH:
				
				{
					surfaceType = TBSP_ST_BSP;

					int numPatchVerts = getNumPatchVerts(bspFaces[f].patchSize[0]) * getNumPatchVerts(bspFaces[f].patchSize[1]);

					numBSPVertices += numPatchVerts;
				}

				break;

			case TBSP_FT_MESH_VERTICES:

				{
					surfaceType = TBSP_ST_MESH;
	
					numMeshVertices += bspFaces[f].numVerts;
				}
					
				break;

			case TBSP_FT_BILLBOARD:

				{
					surfaceType = TBSP_ST_BILLBOARD;
					//etc
				}

				break;
			}

			bool surfaceFound = false;
	
			for (int s = 0; s < (int)tmpSurfaces.size(); s++)
			{
				if (surfaceType == tmpSurfaces[s]->type &&
					bspFaces[f].textureID == tmpSurfaces[s]->textureID &&
					bspFaces[f].lightmapID == tmpSurfaces[s]->lightmapID)
				{
					if ((faceNumReferences[f] > 1 && tmpSurfaces[s]->isShared) || (faceNumReferences[f] == 1 && tmpSurfaces[s]->isShared == false))
					{
						tmpSurfaces[s]->bspFaces.push_back(f);
	
						if (bspFaces[f].type == TBSP_FT_BEZIER_PATCH)
						{
							int numPatchVertsS = getNumPatchVerts(bspFaces[f].patchSize[0]);
							int numPatchVertsT = getNumPatchVerts(bspFaces[f].patchSize[1]);

							int numPatchQuads   = (numPatchVertsS - 1) * (numPatchVertsT - 1);
							int numPatchIndices = numPatchQuads * 6;

							tmpSurfaces[s]->numIndices += numPatchIndices;
						}
						else
						{
							tmpSurfaces[s]->numIndices += bspFaces[f].numMeshVerts;
						}
	
						surfaceFound = true;
	
						break;
					}
				}
			}

			// todo: temp! for now we'll just omit all billboard surfaces

			if (!surfaceFound && surfaceType != TBSP_ST_BILLBOARD)
			{
				TBSP_SURFACE *newSurface = new TBSP_SURFACE;

				newSurface->textureID  = bspFaces[f].textureID;
				newSurface->lightmapID = bspFaces[f].lightmapID;

				newSurface->type = surfaceType;

				if (bspFaces[f].type == TBSP_FT_BEZIER_PATCH)
				{
					int numPatchVertsS = getNumPatchVerts(bspFaces[f].patchSize[0]);
					int numPatchVertsT = getNumPatchVerts(bspFaces[f].patchSize[1]);

					int numPatchQuads = (numPatchVertsS - 1) * (numPatchVertsT - 1);
					int numPatchIndices = numPatchQuads * 6;

					newSurface->numIndices = numPatchIndices;
				}
				else
				{
					newSurface->numIndices = bspFaces[f].numMeshVerts;
				}
				
				newSurface->bspFaces.push_back(f);

				tmpSurfaces.push_back(newSurface);

				if (faceNumReferences[f] > 1)
				{
					newSurface->isShared = true;
				}
				else
				{
					newSurface->isShared = false;
				}

				switch(surfaceType)
				{
				case TBSP_ST_BSP:

					numBSPSurfaces++;

					break;

				case TBSP_ST_MESH:

					numMeshSurfaces++;

					break;

				case TBSP_ST_BILLBOARD:

					//etc

					break;
				}
			}
		}

		if (!tmpSurfaces.size())
		{
			continue;
		}

		BYTE *bspVbuff;
		BSP_VERTEX *bspVertexData;
		IDirect3DVertexBuffer8 *bspVertexBuffer;

		if (numBSPSurfaces)
		{
			string resourceName = "bsp\\cluster";

			resourceName += intString(c);
			resourceName += "\\";
			resourceName += "bspvbuff";

			bspClusters[c].bspVertexBuffer = new CVertexBufferResource;
			bspClusters[c].bspVertexBuffer->create(XENGINE_BSP_VERTEX_FVF, numBSPVertices, sizeof(BSP_VERTEX), false, resourceName);

			bspClusters[c].bspVertexBuffer->acquire(true);

			bspVertexBuffer = bspClusters[c].bspVertexBuffer->getVertexBuffer();

			bspVertexBuffer->Lock(0, 0, &bspVbuff, NULL);
			bspVertexData = (BSP_VERTEX*)bspVbuff;
		}
		else
		{
			bspClusters[c].bspVertexBuffer = NULL;

			bspVertexBuffer = NULL;
			bspVertexData   = NULL; // todo: these two are redundant
			bspVbuff        = NULL;
		}

		BYTE *bspMVBuff;
		BSP_MESH_VERTEX *bspMeshVertexData;
		IDirect3DVertexBuffer8 *meshVertexBuffer;

		if (numMeshSurfaces)
		{
			string resourceName = "bsp\\cluster";

			resourceName += intString(c);
			resourceName += "\\";
			resourceName += "meshvbuff";

			bspClusters[c].meshVertexBuffer = new CVertexBufferResource;
			bspClusters[c].meshVertexBuffer->create(XENGINE_BSP_MESH_VERTEX_FVF, numMeshVertices, sizeof(BSP_MESH_VERTEX), false, resourceName);

			bspClusters[c].meshVertexBuffer->acquire(true);

			meshVertexBuffer = bspClusters[c].meshVertexBuffer->getVertexBuffer();

			meshVertexBuffer->Lock(0, 0, &bspMVBuff, NULL);
			bspMeshVertexData = (BSP_MESH_VERTEX*)bspMVBuff;
		}
		else
		{
			bspClusters[c].meshVertexBuffer = NULL;

			meshVertexBuffer = NULL;
		}

		bspClusters[c].numSurfaces = numBSPSurfaces + numMeshSurfaces;

		if (bspClusters[c].numSurfaces)
		{			
			bspClusters[c].surfaces = new XSURFACE*[bspClusters[c].numSurfaces + (int)sharedBSPSurfaces.size() + (int)sharedMeshSurfaces.size()];
		}
		else
		{
			bspClusters[c].surfaces = NULL;
		}

		// todo: create billboard vertex buffers / surfaces
		
		int surfaceIndex = 0;

		int bspVertexBufferOffset  = 0;
		int meshVertexBufferOffset = 0;

		for (int s = 0; s < (int)tmpSurfaces.size(); s++)
		{
			XSURFACE *surface = new XSURFACE;
			
			bspClusters[c].surfaces[surfaceIndex] = surface;

			string shaderName = bspTextures[tmpSurfaces[s]->textureID].strName;

			shaderName = correctPathSlashes(shaderName);

			CTextureResource *lightmap = NULL;

			if (tmpSurfaces[s]->lightmapID >= 0)
			{
				lightmap = lightmaps[tmpSurfaces[s]->lightmapID];
			}

			CShaderResource *surfaceShader = CShaderResource::getShader(shaderName);

			surface->lightmap = lightmap;

			if (!surfaceShader)
			{
				surface->texture = CTextureResource::getTexture(shaderName);
				surface->shader = NULL;
			}
			else
			{
				surface->shader = surfaceShader;
				surface->texture = NULL;
			}

			surface->indexBuffer = new CIndexBufferResource;
			
			int vertexBufferOffset;

			switch (tmpSurfaces[s]->type)
			{
			case TBSP_ST_BSP:

				vertexBufferOffset = bspVertexBufferOffset;
				surface->vertexBuffer = bspClusters[c].bspVertexBuffer;

				break;

			case TBSP_ST_MESH:

				vertexBufferOffset = meshVertexBufferOffset;
				surface->vertexBuffer = bspClusters[c].meshVertexBuffer;

				break;
			}

			string resourceName = "bsp\\cluster";

			resourceName += intString(c);
			resourceName += "\\surface";
			resourceName += intString(surfaceIndex);
			resourceName += "indexbuff";

			surface->indexBuffer->create(vertexBufferOffset, tmpSurfaces[s]->numIndices, false, resourceName);

			surface->indexBuffer->acquire(true);

			IDirect3DIndexBuffer8 *indexBuffer = surface->indexBuffer->getIndexBuffer();

			BYTE *ibuff; indexBuffer->Lock(0, 0, &ibuff, NULL);
			unsigned short *indexData = (unsigned short *)ibuff;

			int indexBufferOffset = 0;

			int numSurfaceVerts = 0;

			// used for computing the surface position, which is used for sorting blended surfaces

			bool hasMins = false;
			bool hasMaxs = false;

			D3DXVECTOR3 bbMin = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
			D3DXVECTOR3 bbMax = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

			switch (tmpSurfaces[s]->type)
			{
			case TBSP_ST_BSP:

				for (int f = 0; f < (int)tmpSurfaces[s]->bspFaces.size(); f++)
				{
					TBSP_FACE *face = &bspFaces[tmpSurfaces[s]->bspFaces[f]];

					faceBSPSurfaces[tmpSurfaces[s]->bspFaces[f]] = surface;

					switch (face->type)
					{
					case TBSP_FT_POLYGON:

						surface->lightingType = XLT_LIGHTMAP;

						if (face->numVerts && face->numMeshVerts)
						{
							int numTmpVertices = 0;

							int *tmpVertices = new int[face->numVerts];
							int *tmpIndices  = new int[face->numMeshVerts];

							for (int i = 0; i < face->numMeshVerts; i++)
							{
								bool vertexFound = false;
								
								for (int v = 0; v < numTmpVertices; v++)
								{
									if (tmpVertices[v] == bspMeshVerts[i + face->meshVertIndex])
									{
										vertexFound = true;

										tmpIndices[i] = v;

										break;
									}
								}

								if (!vertexFound)
								{
									tmpVertices[numTmpVertices] = bspMeshVerts[i + face->meshVertIndex];
									tmpIndices[i] = numTmpVertices;

									numTmpVertices++;
								}
							}

							for (int v = 0; v < face->numVerts; v++)
							{
								bspVertexData[v + bspVertexBufferOffset].pos.x = -bspVertices[tmpVertices[v] + face->startVertIndex].position.x;
								bspVertexData[v + bspVertexBufferOffset].pos.y = +bspVertices[tmpVertices[v] + face->startVertIndex].position.z;
								bspVertexData[v + bspVertexBufferOffset].pos.z = -bspVertices[tmpVertices[v] + face->startVertIndex].position.y;

								bspVertexData[v + bspVertexBufferOffset].normal.x = -bspVertices[tmpVertices[v] + face->startVertIndex].normal.x;
								bspVertexData[v + bspVertexBufferOffset].normal.y = +bspVertices[tmpVertices[v] + face->startVertIndex].normal.z;
								bspVertexData[v + bspVertexBufferOffset].normal.z = -bspVertices[tmpVertices[v] + face->startVertIndex].normal.y;

								if (!hasMins)
								{
									bbMin = bspVertexData[v + bspVertexBufferOffset].pos;
								}
								else
								{
									if (bspVertexData[v + bspVertexBufferOffset].pos.x < bbMin.x) bbMin.x = bspVertexData[v + bspVertexBufferOffset].pos.x;
									if (bspVertexData[v + bspVertexBufferOffset].pos.y < bbMin.y) bbMin.y = bspVertexData[v + bspVertexBufferOffset].pos.y;
									if (bspVertexData[v + bspVertexBufferOffset].pos.z < bbMin.z) bbMin.z = bspVertexData[v + bspVertexBufferOffset].pos.z;
								}

								if (!hasMaxs)
								{
									bbMax = bspVertexData[v + bspVertexBufferOffset].pos;
								}
								else
								{
									if (bspVertexData[v + bspVertexBufferOffset].pos.x > bbMax.x) bbMax.x = bspVertexData[v + bspVertexBufferOffset].pos.x;
									if (bspVertexData[v + bspVertexBufferOffset].pos.y > bbMax.y) bbMax.y = bspVertexData[v + bspVertexBufferOffset].pos.y;
									if (bspVertexData[v + bspVertexBufferOffset].pos.z > bbMax.z) bbMax.z = bspVertexData[v + bspVertexBufferOffset].pos.z;
								}

								// works for a nice tight fit compared to using one of the bb box corners
								
								D3DXVECTOR3 deltaVec = bspClusters[c].boundingSpherePos - bspVertexData[v + bspVertexBufferOffset].pos;

								float distance = D3DXVec3Length(&deltaVec);

								if (distance > bspClusters[c].boundingSphereRadius)
								{
									bspClusters[c].boundingSphereRadius = distance;
								}

								bspVertexData[v + bspVertexBufferOffset].color = adjustVertexGamma(bspVertices[tmpVertices[v] + face->startVertIndex].color);

								bspVertexData[v + bspVertexBufferOffset].texCoord      = bspVertices[tmpVertices[v] + face->startVertIndex].texCoord;
								bspVertexData[v + bspVertexBufferOffset].lightmapCoord = bspVertices[tmpVertices[v] + face->startVertIndex].lightmapCoord;
							}

							for (int i = 0; i < face->numMeshVerts; i++)
							{
								indexData[i + indexBufferOffset] = tmpIndices[i] + numSurfaceVerts;
							}
							
							delete [] tmpVertices;
							delete [] tmpIndices;

							numSurfaceVerts += face->numVerts;

							bspVertexBufferOffset += face->numVerts;
							indexBufferOffset     += face->numMeshVerts;
						}

						break;

					case TBSP_FT_BEZIER_PATCH:

						{
							surface->lightingType = XLT_LIGHTMAP;

							int numPatchControlPointsS = face->patchSize[0];
							int numPatchControlPointsT = face->patchSize[1];

							int numPatchVertsS = getNumPatchVerts(numPatchControlPointsS);
							int numPatchVertsT = getNumPatchVerts(numPatchControlPointsT);

							int numPatchVerts = numPatchVertsS * numPatchVertsT;

							int numPatchQuadsS = (numPatchVertsS - 1);
							int numPatchQuadsT = (numPatchVertsT - 1);

							int numPatchQuads   = numPatchQuadsS * numPatchQuadsT;
							int numPatchIndices = numPatchQuads * 6;

							int numSubPatchesS  = (numPatchControlPointsS - 1) / 2;
							int numSubPatchesT  = (numPatchControlPointsT - 1) / 2;

							int numSubPatches   = numSubPatchesS * numSubPatchesT;

							for (int subPatchS = 0; subPatchS < numSubPatchesS; subPatchS++)
							{
								for (int subPatchT = 0; subPatchT < numSubPatchesT; subPatchT++)
								{
									TBSP_PATCH subPatch;

									for (int pS = 0; pS < 3; pS++)
									{
										for (int pT = 0; pT < 3; pT++)
										{
											subPatch.verts[pS][pT] = subPatchS * 2 + pS + subPatchT * 2 * numPatchControlPointsS + pT * numPatchControlPointsS + face->startVertIndex;
										}
									}

									int numSubPatchVertsS = getNumPatchVerts(3);
									int numSubPatchVertsT = getNumPatchVerts(3);

									for (int vS = 0; vS < numSubPatchVertsS; vS++)
									{
										for (int vT = 0; vT < numSubPatchVertsT; vT++)
										{
											TBSP_VERTEX tmpVertex;

											float u = float(vS) / float(numSubPatchVertsS - 1);
											float v = float(vT) / float(numSubPatchVertsT - 1);

											bezierPatch(&tmpVertex, &subPatch, u, v, bspVertices);
											
											int vertIndex = bspVertexBufferOffset + (vT + subPatchT * (numSubPatchVertsS - 1)) * numPatchVertsS
																				  + (vS + subPatchS * (numSubPatchVertsT - 1));

											bspVertexData[vertIndex].pos.x = -tmpVertex.position.x;
											bspVertexData[vertIndex].pos.y = +tmpVertex.position.z;
											bspVertexData[vertIndex].pos.z = -tmpVertex.position.y;

											bspVertexData[vertIndex].normal.x = -tmpVertex.normal.x;
											bspVertexData[vertIndex].normal.y = +tmpVertex.normal.z;
											bspVertexData[vertIndex].normal.z = -tmpVertex.normal.y;

											bspVertexData[vertIndex].texCoord	   = tmpVertex.texCoord;
											bspVertexData[vertIndex].lightmapCoord = tmpVertex.lightmapCoord;

											bspVertexData[vertIndex].color = tmpVertex.color;

											if (!hasMins)
											{
												bbMin = bspVertexData[vertIndex].pos;
											}
											else
											{
												if (bspVertexData[vertIndex].pos.x < bbMin.x) bbMin.x = bspVertexData[vertIndex].pos.x;
												if (bspVertexData[vertIndex].pos.y < bbMin.y) bbMin.y = bspVertexData[vertIndex].pos.y;
												if (bspVertexData[vertIndex].pos.z < bbMin.z) bbMin.z = bspVertexData[vertIndex].pos.z;
											}

											if (!hasMaxs)
											{
												bbMax = bspVertexData[vertIndex].pos;
											}
											else
											{
												if (bspVertexData[vertIndex].pos.x > bbMax.x) bbMax.x = bspVertexData[vertIndex].pos.x;
												if (bspVertexData[vertIndex].pos.y > bbMax.y) bbMax.y = bspVertexData[vertIndex].pos.y;
												if (bspVertexData[vertIndex].pos.z > bbMax.z) bbMax.z = bspVertexData[vertIndex].pos.z;
											}

											// works for a nice tight fit compared to using one of the bb box corners
											
											D3DXVECTOR3 deltaVec = bspClusters[c].boundingSpherePos - bspVertexData[vertIndex].pos;

											float distance = D3DXVec3Length(&deltaVec);

											if (distance > bspClusters[c].boundingSphereRadius)
											{
												bspClusters[c].boundingSphereRadius = distance;
											}

										}
									}
								}
							}

							for (int qS = 0; qS < numPatchQuadsS; qS++)
							{
								for (int qT = 0; qT < numPatchQuadsT; qT++)
								{
									int indexIndex = (qS + qT * numPatchQuadsS) * 6 + indexBufferOffset;

									indexData[indexIndex + 0] = (qS + 0) + (qT + 1) * (numPatchVertsS) + numSurfaceVerts;
									indexData[indexIndex + 2] = (qS + 0) + (qT + 0) * (numPatchVertsS) + numSurfaceVerts;
									indexData[indexIndex + 1] = (qS + 1) + (qT + 0) * (numPatchVertsS) + numSurfaceVerts;

									indexData[indexIndex + 3] = (qS + 0) + (qT + 1) * (numPatchVertsS) + numSurfaceVerts;
									indexData[indexIndex + 5] = (qS + 1) + (qT + 0) * (numPatchVertsS) + numSurfaceVerts;
									indexData[indexIndex + 4] = (qS + 1) + (qT + 1) * (numPatchVertsS) + numSurfaceVerts;
								}
							}

							numSurfaceVerts		  += numPatchVerts;
							bspVertexBufferOffset += numPatchVerts;

							indexBufferOffset     += numPatchIndices;
						}

						break;
					}
				}

				break;

			case TBSP_ST_MESH:

				surface->lightingType = XLT_VERTEXLIGHTING;

				for (int f = 0; f < (int)tmpSurfaces[s]->bspFaces.size(); f++)
				{
					TBSP_FACE *face = &bspFaces[tmpSurfaces[s]->bspFaces[f]];

					faceMeshSurfaces[tmpSurfaces[s]->bspFaces[f]] = surface;

					if (face->numVerts && face->numMeshVerts)
					{
						int numTmpVertices = 0;

						int *tmpVertices = new int[face->numVerts];
						int *tmpIndices  = new int[face->numMeshVerts];

						ZeroMemory(tmpVertices, sizeof(int) * face->numVerts);
						ZeroMemory(tmpIndices, sizeof(int) * face->numMeshVerts);

						for (int i = 0; i < face->numMeshVerts; i++)
						{
							bool vertexFound = false;
							
							for (int v = 0; v < numTmpVertices; v++)
							{
								if (tmpVertices[v] == bspMeshVerts[i + face->meshVertIndex])
								{
									vertexFound = true;

									tmpIndices[i] = v;

									break;
								}
							}

							if (!vertexFound)
							{
								tmpVertices[numTmpVertices] = bspMeshVerts[i + face->meshVertIndex];
								tmpIndices[i] = numTmpVertices;

								numTmpVertices++;
							}
						}
						

						for (int v = 0; v < face->numVerts; v++)
						{
							bspMeshVertexData[v + meshVertexBufferOffset].pos.x = -bspVertices[tmpVertices[v] + face->startVertIndex].position.x;
							bspMeshVertexData[v + meshVertexBufferOffset].pos.y = +bspVertices[tmpVertices[v] + face->startVertIndex].position.z;
							bspMeshVertexData[v + meshVertexBufferOffset].pos.z = -bspVertices[tmpVertices[v] + face->startVertIndex].position.y;

							bspMeshVertexData[v + meshVertexBufferOffset].normal.x = -bspVertices[tmpVertices[v] + face->startVertIndex].normal.x;
							bspMeshVertexData[v + meshVertexBufferOffset].normal.y = +bspVertices[tmpVertices[v] + face->startVertIndex].normal.z;
							bspMeshVertexData[v + meshVertexBufferOffset].normal.z = -bspVertices[tmpVertices[v] + face->startVertIndex].normal.y;

							if (!hasMins)
							{
								bbMin = bspMeshVertexData[v + meshVertexBufferOffset].pos;
							}
							else
							{
								if (bspMeshVertexData[v + meshVertexBufferOffset].pos.x < bbMin.x) bbMin.x = bspMeshVertexData[v + meshVertexBufferOffset].pos.x;
								if (bspMeshVertexData[v + meshVertexBufferOffset].pos.y < bbMin.y) bbMin.y = bspMeshVertexData[v + meshVertexBufferOffset].pos.y;
								if (bspMeshVertexData[v + meshVertexBufferOffset].pos.z < bbMin.z) bbMin.z = bspMeshVertexData[v + meshVertexBufferOffset].pos.z;
							}

							if (!hasMaxs)
							{
								bbMax = bspMeshVertexData[v + meshVertexBufferOffset].pos;
							}
							else
							{
								if (bspMeshVertexData[v + meshVertexBufferOffset].pos.x > bbMax.x) bbMax.x = bspMeshVertexData[v + meshVertexBufferOffset].pos.x;
								if (bspMeshVertexData[v + meshVertexBufferOffset].pos.y > bbMax.y) bbMax.y = bspMeshVertexData[v + meshVertexBufferOffset].pos.y;
								if (bspMeshVertexData[v + meshVertexBufferOffset].pos.z > bbMax.z) bbMax.z = bspMeshVertexData[v + meshVertexBufferOffset].pos.z;
							}

							// works for a nice tight fit compared to using one of the bb box corners
							
							D3DXVECTOR3 deltaVec = bspClusters[c].boundingSpherePos - bspMeshVertexData[v + meshVertexBufferOffset].pos;

							float distance = D3DXVec3Length(&deltaVec);

							if (distance > bspClusters[c].boundingSphereRadius)
							{
								bspClusters[c].boundingSphereRadius = distance;
							}

							bspMeshVertexData[v + meshVertexBufferOffset].texCoord = bspVertices[tmpVertices[v] + face->startVertIndex].texCoord;
							bspMeshVertexData[v + meshVertexBufferOffset].color    = adjustVertexGamma(bspVertices[tmpVertices[v] + face->startVertIndex].color);
						}

						for (int i = 0; i < face->numMeshVerts; i++)
						{
							indexData[i + indexBufferOffset] = tmpIndices[i] + numSurfaceVerts;
						}
						
						delete [] tmpVertices;
						delete [] tmpIndices;

						numSurfaceVerts += face->numVerts;

						meshVertexBufferOffset += face->numVerts;
						indexBufferOffset      += face->numMeshVerts;
					}
				}

				break;
			}

			indexBuffer->Unlock();

			surface->indexBuffer->drop();
			surface->indexBuffer->cache();

			surface->pos = (bbMin + bbMax) / 2.0f;

			surfaceIndex++;

			delete (TBSP_SURFACE*)tmpSurfaces[s];
		}

		if (bspVertexBuffer)
		{
			bspVertexBuffer->Unlock();
			
			bspClusters[c].bspVertexBuffer->drop();
			bspClusters[c].bspVertexBuffer->cache();
		}

		if (meshVertexBuffer)
		{
			meshVertexBuffer->Unlock();

			bspClusters[c].meshVertexBuffer->drop();
			bspClusters[c].meshVertexBuffer->cache();
		}

		//etc
		
		// todo: temp

		if (sharedBSPSurfaces.size())
		{
			for (int s = 0; s < (int)sharedBSPSurfaces.size(); s++)
			{
				bspClusters[c].surfaces[bspClusters[c].numSurfaces + s] = sharedBSPSurfaces[s];
			}

			bspClusters[c].numSurfaces += (int)sharedBSPSurfaces.size();

			sharedBSPSurfaces.clear();
		}
		
		if (sharedMeshSurfaces.size())
		{
			for (int s = 0; s < (int)sharedMeshSurfaces.size(); s++)
			{
				bspClusters[c].surfaces[bspClusters[c].numSurfaces + s] = sharedMeshSurfaces[s];
			}

			bspClusters[c].numSurfaces += (int)sharedMeshSurfaces.size();

			sharedMeshSurfaces.clear();
		}

		bspClusters[c].surfaceInstances = new XSURFACE_INSTANCE[bspClusters[c].numSurfaces];

		for (int s = 0; s < bspClusters[c].numSurfaces; s++)
		{
			bspClusters[c].surfaceInstances[s].surface = bspClusters[c].surfaces[s];

			bspClusters[c].surfaceInstances[s].lastFrameRendered = 0;
			bspClusters[c].surfaces[s]->lastFrameRendered = 0;

			// *** TODO: movers are referenced in the models lump
			// this means that I need to prevent surface merging across multiple models

			bspClusters[c].surfaceInstances[s].hasTransform = false; 
			bspClusters[c].surfaceInstances[s].hasColor	    = false;

			bspClusters[c].surfaceInstances[s].target = NULL;

			bspClusters[c].surfaceInstances[s].numLights = 0;
		}

		tmpSurfaces.clear();
	}

	for (int c = 0; c < numBSPClusters; c++)
	{
		if (bspClusters[c].numVisibleClusters)
		{
			float *clusterDistances = new float[bspClusters[c].numVisibleClusters];

			for (int cluster = 0; cluster < bspClusters[c].numVisibleClusters; cluster++)
			{
				D3DXVECTOR3 deltaVec = bspClusters[c].boundingSpherePos - bspClusters[c].visibleClusters[cluster]->boundingSpherePos;

				clusterDistances[cluster] = D3DXVec3Length(&deltaVec);
			}

			// todo: worst sorting ever alert!

			bool doneSorting;

			do
			{
				doneSorting = true;

				for (int cluster = 0; cluster < bspClusters[c].numVisibleClusters - 1; cluster++)
				{
					if (clusterDistances[cluster] > clusterDistances[cluster + 1])
					{
						float tmpDist = clusterDistances[cluster];
						BSP_CLUSTER *tmpCluster = bspClusters[c].visibleClusters[cluster];

						clusterDistances[cluster] = clusterDistances[cluster + 1];
						bspClusters[c].visibleClusters[cluster] = bspClusters[c].visibleClusters[cluster + 1];
						
						clusterDistances[cluster + 1] = tmpDist;
						bspClusters[c].visibleClusters[cluster + 1] = tmpCluster;

						doneSorting = false;
					}
				}

			} while (!doneSorting);

			delete [] clusterDistances;
		}
	}

	// last, but not least, populate the world with the entities contained in the bsp file

	// *** temp ***, just to get a better idea of what this looks like

	FILE *fp = fopen ("d:\\bspentities.txt", "wb");
	fwrite(buff, 1, bspLumps[TBSP_LUMP_ENTITIES].length, fp);

	fclose(fp);

	// *** /temp ***

	string bspEntities = buff;
	delete [] buff;

	//loadWorldEntities(bspEntities);

	// finally, de-allocate the data structures we aren't going to use or that have been converted

	delete [] faceClusters;
	delete [] faceNumReferences;
	delete [] faceBSPSurfaces;
	delete [] faceMeshSurfaces;

	delete [] bspVertices;
	delete [] bspFaces;
	delete [] bspLightmaps;
	delete [] bspShaders;
	delete [] bspLeafFaces;
	delete [] bspMeshVerts;
	delete [] bspModels; // todo: might wanna save these
	delete [] bspTextures;

	// keep the reference to the loaded bsp

	world = this;

	__super::create(bspPath);
}

int CBSPResource::registerEntity(CPhysicalEntity *entity)
{
	// clear the entity's old residence

	for(list<PHYSICALENTITY_CLUSTER_RESIDENCE_INFO*>::iterator iResidence = entity->clusterResidence.begin();
		iResidence != entity->clusterResidence.end(); iResidence++)
	{	
		PHYSICALENTITY_CLUSTER_RESIDENCE_INFO* residence = (*iResidence);

		bspClusters[residence->cluster].entities.erase(residence->thisEntity);

		delete residence;
	}

	entity->clusterResidence.clear();

	// and find it's new residence

	return registerEntity(0, entity);
}

int CBSPResource::registerEntity(int nodeIndex, CPhysicalEntity *entity)
{
	if (nodeIndex >= 0)
	{
		D3DXVECTOR3 pos;
		entity->getPosition(&pos);

		float radius = entity->getBoundingSphereRadius();

		const TBSP_NODE&  node  = bspNodes[nodeIndex];
		const TBSP_PLANE& plane = bspPlanes[node.plane];

		float distance = D3DXVec3Dot(&plane.normal, &pos) - plane.d;
	 
		if (distance < -radius)
		{
			return registerEntity(node.back, entity);
		}
		else if (distance > radius)
		{
			return registerEntity(node.front, entity); 
		}
		else
		{
			if (distance < 0.0f)
			{
				registerEntity(node.front, entity);
				return registerEntity(node.back, entity);
			}
			else
			{
				registerEntity(node.back, entity);
				return registerEntity(node.front, entity);
			}
		}
	}
	else
	{
		int cluster = bspLeafs[~nodeIndex].cluster;

		if (cluster >=0)
		{
			PHYSICALENTITY_CLUSTER_RESIDENCE_INFO *residence = new PHYSICALENTITY_CLUSTER_RESIDENCE_INFO;

			residence->cluster = cluster;
			
			entity->clusterResidence.push_front(residence);

			bspClusters[cluster].entities.push_front(entity);
			residence->thisEntity = bspClusters[cluster].entities.begin();
		}

		return cluster;
	}
}

	/*
int CBSPResource::findCluster(D3DXVECTOR3 *pos)
{
	int leaf = findLeaf(pos);

	return bspLeafs[leaf].cluster;
}

inline int CBSPResource::findLeaf(D3DXVECTOR3 *pos)
{    
	int nodeIndex = 0;
    float distance = 0.0f;

    while (nodeIndex >= 0)
    {
        const TBSP_NODE&  node  = bspNodes[nodeIndex];
        const TBSP_PLANE& plane = bspPlanes[node.plane];

        distance = D3DXVec3Dot(&plane.normal, pos) - plane.d;
 
        if (distance < 0.0f)   
        {
            nodeIndex = node.back;
        }
        else        
        {
			nodeIndex = node.front;   
        }
    }

    return ~nodeIndex;
}
*/

int CBSPResource::isClusterVisible(int current, int test)
{
    if (!bspVisData.bitsets || current < 0)
	{
		return 1;
	}

    byte visSet = bspVisData.bitsets[(current * bspVisData.bytesPerCluster) + (test / 8)];
    
    return visSet & (1 << ((test) & 7));
}

void CBSPResource::setClusterVisibility(int current, int test, bool visibility)
{
    if (!bspVisData.bitsets || current < 0)
	{
		return;
	}

	byte bitMask = (1 << ((test) & 7));
	int byteOffset = (current * bspVisData.bytesPerCluster) + (test / 8);

    byte visSet = bspVisData.bitsets[byteOffset];

	int oldVisibility = visSet & bitMask;

	if (oldVisibility)
	{
		if (visibility)
		{
			return;
		}
		else
		{
			visSet ^= bitMask;
		}
	}
	else
	{
		if (visibility)
		{
			visSet |= bitMask;
		}
		else
		{
			return;
		}
	}

	bspVisData.bitsets[byteOffset] = visSet;
}

void CBSPResource::render()
{
	currentCamera = XEngine->getCurrentCamera();

	int numClustersRendered    = 0;

	if (!currentCamera)
	{
		XEngine->debugPrint("*** ERROR *** BSP being rendered with no camera!\n");
	}
	else
	{
		int cameraCluster = currentCamera->getCurrentCluster();
	
		if (cameraCluster < 0)
		{
			for (int c = 0; c < numBSPClusters; c++)
			{
				renderCluster(&bspClusters[c]);
			}
		}
		else
		{			
			BSP_CLUSTER *cluster = &bspClusters[cameraCluster];

			renderCluster(cluster);

			for (int c = 0; c < cluster->numVisibleClusters; c++)
			{
				renderCluster(cluster->visibleClusters[c]);
			}
		}
	}
}

void CBSPResource::renderCluster(BSP_CLUSTER *renderCluster)
{
	if (renderCluster->numSurfaces || renderCluster->entities.size())
	{
		if (!currentCamera->sphereInFrustum(renderCluster->boundingSpherePos.x,
										    renderCluster->boundingSpherePos.y,
											renderCluster->boundingSpherePos.z,
											renderCluster->boundingSphereRadius))
		{
			return;
		}

		for (int s = 0; s < renderCluster->numSurfaces; s++)
		{
			if (renderCluster->surfaces[s]->lastFrameRendered < XEngine->getRenderFrame())
			{
				renderCluster->surfaces[s]->lastFrameRendered = XEngine->getRenderFrame();

				XEngine->queueSurface(&renderCluster->surfaceInstances[s]);
			}
		}

		for(list<CPhysicalEntity*>::iterator iPhysicalEntity = renderCluster->entities.begin(); iPhysicalEntity != renderCluster->entities.end(); iPhysicalEntity++)
		{	
			CPhysicalEntity* physicalEntity = (CPhysicalEntity*)(*iPhysicalEntity);

			if (physicalEntity->getRenderEnable())
			{
				if (physicalEntity->getLastFrameRendered() < XEngine->getRenderFrame() && !physicalEntity->isHidden())
				{
					D3DXVECTOR3 boundingSpherePos;
					physicalEntity->getPosition(&boundingSpherePos);

					float boundingSphereRadius = physicalEntity->getBoundingSphereRadius();

#ifdef PHYSICAL_ENTITY_DEBUG_SPHERES

					D3DXMATRIX worldMatrix, tmpMatrix;
					
					D3DXMatrixTranslation(&worldMatrix, boundingSpherePos.x, boundingSpherePos.y, boundingSpherePos.z);
					D3DXMatrixScaling(&tmpMatrix, boundingSphereRadius, boundingSphereRadius, boundingSphereRadius);
					D3DXMatrixMultiply(&worldMatrix, &tmpMatrix, &worldMatrix);

					device->SetTransform(D3DTS_WORLD, &worldMatrix);

					device->SetRenderState(D3DRS_ALPHATESTENABLE, false);
					device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

					device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

					CPhysicalEntity::sphereMesh->DrawSubset(0);

					device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

					D3DXMatrixIdentity(&worldMatrix);

					device->SetTransform(D3DTS_WORLD, &worldMatrix);
#endif

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
}

void CBSPResource::destroy()
{
	// release some memory

	delete [] bspBrushSides;
	delete [] bspBrushes;
	delete [] bspLeafBrushes;
	delete [] bspNodes;
	delete [] bspPlanes;
	delete [] bspLeafs;
	
	for (int m = 0; m < numBSPMaterials; m++)
	{
		bspMaterials[m].footstepSounds->release();
	}

	delete [] bspMaterials;

	if (bspVisData.bitsets)
	{
		delete [] bspVisData.bitsets;
	}

	// release some more memory

	/*
	for (int c = 0; c < numBSPClusters; c++)
	{
		for (int s = 0; s < bspClusters[c].numBSPSurfaces; s++)
		{
			// todo: release dynamic surface vertices/indices

			if (!bspClusters[c].bspSurfaces[s].isDynamic)
			{
				bspClusters[c].bspSurfaces[s].indexBuffer->Release();
			}

			if (bspClusters[c].bspSurfaces[s].shader)
			{
				bspClusters[c].bspSurfaces[s].shader->release();
			}
		}

		bspClusters[c].bspVertexBuffer->Release();

		delete [] bspClusters[c].bspSurfaces;
	}
	*/

	delete [] bspClusters;

	for (int i = 0; i < numBSPLightmaps; i++)
	{
		lightmaps[i]->release();
	}

	delete [] lightmaps;

	world = NULL;

	__super::destroy();
}