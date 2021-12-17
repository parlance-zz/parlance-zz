#include "md3_resource.h"
#include "texture_resource.h"
#include "indexbuffer_resource.h"
#include "vertexbuffer_resource.h"
#include "shader_resource.h"
#include "resource_types.h"
#include "xengine.h"
#include "string_help.h"
#include "math_help.h"

vector<CResource*> CMD3Resource::md3Resources;

int CMD3Resource::getResourceType()
{
	return RT_MD3_RESOURCE;
}

CMD3Resource *CMD3Resource::getMD3(string md3Path)
{
	for (int i = 0; i < (int)CMD3Resource::md3Resources.size(); i++)
	{
		CMD3Resource *md3Resource = (CMD3Resource*)CMD3Resource::md3Resources[i];

		if (md3Resource->getResourceName() == md3Path)
		{
			md3Resource->addRef();

			return md3Resource;
		}
	}

	CMD3Resource *md3Resource = new CMD3Resource;

	md3Resource->create(md3Path);

	return md3Resource;
}

void CMD3Resource::create(string md3Path)
{
	string filePath = XENGINE_RESOURCES_MEDIA_PATH;
	filePath += md3Path;

	FILE *md3File = fopen(filePath.c_str(), "rb");

	if (!md3File)
	{
		string debugString = "*** ERROR *** MD3 file not found '";

		debugString += md3Path;
		debugString += "'!\n";

		XEngine->debugPrint(debugString);
		
		return;
	}

	TMD3_HEADER md3Header;

	fread(&md3Header, 1, sizeof(TMD3_HEADER), md3File);

	if (md3Header.magic != XENGINE_MD3_RESOURCE_MAGIC)
	{
		string debugString = "*** ERROR *** Error loading invalid or corrupt MD3 file '";

		debugString += md3Path;
		debugString += "'!\n";

		XEngine->debugPrint(debugString);
		
		fclose(md3File);

		return;
	}

	if (md3Header.version != XENGINE_MD3_RESOURCE_VERSION)
	{
		string debugString = "*** ERROR *** Incorrect MD3 version found in file '";

		debugString += md3Path;
		debugString += "'!\n";

		XEngine->debugPrint(debugString);
		
		fclose(md3File);

		return;
	}

	fseek(md3File, md3Header.framesLumpPos, SEEK_SET);

	TMD3_FRAME *md3Frames = new TMD3_FRAME[md3Header.numFrames];
	fread(md3Frames, md3Header.numFrames, sizeof(TMD3_FRAME), md3File);

	numFrames = md3Header.numFrames;
	frames    = new MD3_FRAME[numFrames];

	numSurfaces = md3Header.numSurfaces;
	surfaces    = new MD3_SURFACE[numSurfaces];

	for (int f = 0; f < numFrames; f++)
	{
		frames[f].surfaces = new XSURFACE[numSurfaces];

		// todo: these may need adjusting

		frames[f].boundingSphereRadius   = md3Frames[f].radius;
		frames[f].localBoundingSpherePos = md3Frames[f].localOrigin;
	}

	delete [] md3Frames;

	fseek(md3File, md3Header.surfacesLumpPos, SEEK_SET);

	for (int s = 0; s < numSurfaces; s++)
	{
		long surfaceStart = ftell(md3File);

		TMD3_SURFACE surface;
		fread(&surface, 1, sizeof(TMD3_SURFACE), md3File);

		if (surface.magic != XENGINE_MD3_RESOURCE_MAGIC)
		{
			string debugString = "*** ERROR *** Incorrect MD3 surface version found in file '";

			debugString += md3Path;
			debugString += "'!\n";

			XEngine->debugPrint(debugString);
			
			// todo: error out a little more gracefully, at this point we'll just let it crash and burn
		}
		
		fseek(md3File, surfaceStart + surface.shaderLumpOffset, SEEK_SET);

		// we're only interested in the first shader for this surface

		TMD3_SHADER md3Shader;
		fread(&md3Shader, 1, sizeof(TMD3_SHADER), md3File);

		// wtf? carmack has been smoking so many shrooms

		if (md3Shader.name[0] == NULL)
		{
			md3Shader.name[0] = 'm';
		}

		string shaderName = md3Shader.name; shaderName = correctPathSlashes(shaderName);

		surfaces[s].shader = CShaderResource::getShader(shaderName);

		if (!surfaces[s].shader)
		{
			surfaces[s].texture = CTextureResource::getTexture(shaderName);
		}
		else
		{
			surfaces[s].texture = NULL;
		}

		fseek(md3File, surfaceStart + surface.triangleLumpOffset, SEEK_SET);
		
		TMD3_TRIANGLE *md3Triangles = new TMD3_TRIANGLE[surface.numTriangles];
		fread(md3Triangles, surface.numTriangles, sizeof(TMD3_TRIANGLE), md3File);

		surfaces[s].indexBuffer = new CIndexBufferResource;

		string indexBufferName = md3Path;

		indexBufferName += "\\indexbuff";
		indexBufferName += intString(s);

		if (numFrames == 1)
		{	
			surfaces[s].indexBuffer->create(0, surface.numTriangles * 3, false, indexBufferName);
			surfaces[s].indexBuffer->acquire(true);

			IDirect3DIndexBuffer8 *indexBuffer = surfaces[s].indexBuffer->getIndexBuffer();

			BYTE *md3Ibuff;
			unsigned short *md3IndexData;
			
			indexBuffer->Lock(0, 0, &md3Ibuff, NULL);
			md3IndexData = (unsigned short*)md3Ibuff;

			for (int t = 0; t < surface.numTriangles; t++)
			{
				// todo: order may need to be adjusted if md3s are ccw winding by default

				md3IndexData[t * 3 + 0] = md3Triangles[t].v0;
				md3IndexData[t * 3 + 1] = md3Triangles[t].v1;
				md3IndexData[t * 3 + 2] = md3Triangles[t].v2;
			}
			
			indexBuffer->Unlock();

			surfaces[s].indexBuffer->drop();

			surfaces[s].indexBuffer->cache();
		}
		else
		{
			surfaces[s].indexBuffer->create(0, surface.numTriangles * 3, true, indexBufferName);
			surfaces[s].indexBuffer->acquire(true);

			unsigned short *md3IndexData = surfaces[s].indexBuffer->getIndices();
			
			for (int t = 0; t < surface.numTriangles; t++)
			{
				// todo: order may need to be adjusted if md3s are ccw winding by default

				md3IndexData[t * 3 + 0] = md3Triangles[t].v0;
				md3IndexData[t * 3 + 1] = md3Triangles[t].v1;
				md3IndexData[t * 3 + 2] = md3Triangles[t].v2;
			}
			
			surfaces[s].indexBuffer->drop();

			surfaces[s].indexBuffer->cache();
		}

		delete [] md3Triangles;

		fseek(md3File, surfaceStart + surface.texCoordLumpOffset, SEEK_SET);

		TMD3_TEXCOORD *md3TexCoords = new TMD3_TEXCOORD[surface.numVerts];
		fread(md3TexCoords, surface.numVerts, sizeof(TMD3_TEXCOORD), md3File);

		if (numFrames == 1)
		{
			fseek(md3File, surfaceStart + surface.vertexLumpOffset, SEEK_SET);

			TMD3_VERTEX *md3Vertices = new TMD3_VERTEX[surface.numVerts];
			fread(md3Vertices, surface.numVerts, sizeof(TMD3_VERTEX), md3File);

			string vertexBufferName = md3Path;

			vertexBufferName += "\\vbuff";
			vertexBufferName += intString(s);

			frames[0].surfaces[s].vertexBuffer = new CVertexBufferResource;
			frames[0].surfaces[s].vertexBuffer->create(XENGINE_MD3_VERTEX_FVF, surface.numVerts, sizeof(MD3_VERTEX), false, vertexBufferName);

			frames[0].surfaces[s].vertexBuffer->acquire(true);

			IDirect3DVertexBuffer8 *vertexBuffer = frames[0].surfaces[s].vertexBuffer->getVertexBuffer();

			BYTE *md3Vbuff;
			MD3_VERTEX *md3VertexData;

			vertexBuffer->Lock(0, 0, &md3Vbuff, NULL);
			md3VertexData = (MD3_VERTEX*)md3Vbuff;

			for (int v = 0; v < surface.numVerts; v++)
			{
				float tmpPosX = md3Vertices[v].x / XENGINE_MD3_RESOURCE_VERTEX_SCALE;
				float tmpPosY = md3Vertices[v].y / XENGINE_MD3_RESOURCE_VERTEX_SCALE;
				float tmpPosZ = md3Vertices[v].z / XENGINE_MD3_RESOURCE_VERTEX_SCALE;

				md3VertexData[v].pos.x = -tmpPosY;
				md3VertexData[v].pos.y = +tmpPosZ;
				md3VertexData[v].pos.z = +tmpPosX;

				if (!md3Vertices[v].normal)
				{
					md3VertexData[v].normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
				}
				else if (md3Vertices[v].normal == -0x8000)
				{
					md3VertexData[v].normal = D3DXVECTOR3(0.0f, -1.0f, 0.0f);
				}
				else
				{
					float latitude   = (md3Vertices[v].normal & 0xFF) * 2.0f * D3DX_PI / 255.0f;
					float longtitude = ((md3Vertices[v].normal >> 8) & 0xFF) * 2.0f * D3DX_PI / 255.0f;

					float tmpNormalX = float(cos(latitude) * sin(longtitude));
					float tmpNormalY = float(sin(latitude) * sin(longtitude));
					float tmpNormalZ = float(cos(longtitude));

					md3VertexData[v].normal.x = -tmpNormalY;
					md3VertexData[v].normal.y = +tmpNormalZ;
					md3VertexData[v].normal.z = +tmpNormalX;
				}

				md3VertexData[v].texCoord = md3TexCoords[v].texCoord;
			}
			
			vertexBuffer->Unlock();
			
			frames[0].surfaces[s].vertexBuffer->drop();

			frames[0].surfaces[s].vertexBuffer->cache();

			delete [] md3Vertices;

			surfaces[s].texCoords = NULL;
		}
		else
		{
			surfaces[s].texCoords = new CVertexBufferResource;

			string texCoordsBufferName = md3Path;

			texCoordsBufferName += "\\texbuff";
			texCoordsBufferName += intString(s);

			surfaces[s].texCoords->create(XENGINE_MD3_TEXCOORD_FVF, surface.numVerts, sizeof(TMD3_TEXCOORD), true, texCoordsBufferName);

			surfaces[s].texCoords->acquire(true);

			TMD3_TEXCOORD *texCoordsData = (TMD3_TEXCOORD*)surfaces[s].texCoords->getVertices();
			
			memcpy(texCoordsData, md3TexCoords, sizeof(TMD3_TEXCOORD) * surface.numVerts);

			surfaces[s].texCoords->drop();

			surfaces[s].texCoords->cache();
		}

		delete [] md3TexCoords;

		if (numFrames > 1)
		{
			for (int f = 0; f < numFrames; f++)
			{
				fseek(md3File, surfaceStart + surface.vertexLumpOffset + f * surface.numVerts * sizeof(TMD3_VERTEX), SEEK_SET);

				TMD3_VERTEX *md3Vertices = new TMD3_VERTEX[surface.numVerts];
				fread(md3Vertices, surface.numVerts, sizeof(TMD3_VERTEX), md3File);

				string vertexBufferName = md3Path;

				vertexBufferName += "\\vbuff";
				vertexBufferName += intString(f * numSurfaces + s);

				frames[f].surfaces[s].vertexBuffer = new CVertexBufferResource;
				frames[f].surfaces[s].vertexBuffer->create(XENGINE_MD3_DYNAMIC_VERTEX_FVF, surface.numVerts, sizeof(MD3_DYNAMIC_VERTEX), true, vertexBufferName);

				frames[f].surfaces[s].vertexBuffer->acquire(true);

				MD3_DYNAMIC_VERTEX *md3DynamicVertexData = (MD3_DYNAMIC_VERTEX*)frames[f].surfaces[s].vertexBuffer->getVertices();

				for (int v = 0; v < surface.numVerts; v++)
				{
					float tmpPosX = md3Vertices[v].x / XENGINE_MD3_RESOURCE_VERTEX_SCALE;
					float tmpPosY = md3Vertices[v].y / XENGINE_MD3_RESOURCE_VERTEX_SCALE;
					float tmpPosZ = md3Vertices[v].z / XENGINE_MD3_RESOURCE_VERTEX_SCALE;

					md3DynamicVertexData[v].pos.x = -tmpPosY;
					md3DynamicVertexData[v].pos.y = +tmpPosZ;
					md3DynamicVertexData[v].pos.z = +tmpPosX;

					if (!md3Vertices[v].normal)
					{
						md3DynamicVertexData[v].normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
					}
					else if (md3Vertices[v].normal == -0x8000)
					{
						md3DynamicVertexData[v].normal = D3DXVECTOR3(0.0f, -1.0f, 0.0f);
					}
					else
					{
						float latitude   = (md3Vertices[v].normal & 0xFF) * 2.0f * D3DX_PI / 255.0f;
						float longtitude = ((md3Vertices[v].normal >> 8) & 0xFF) * 2.0f * D3DX_PI / 255.0f;

						float tmpNormalX = float(cos(latitude) * sin(longtitude));
						float tmpNormalY = float(sin(latitude) * sin(longtitude));
						float tmpNormalZ = float(cos(longtitude));

						md3DynamicVertexData[v].normal.x = -tmpNormalY;
						md3DynamicVertexData[v].normal.y = +tmpNormalZ;
						md3DynamicVertexData[v].normal.z = +tmpNormalX;
					}
				}

				frames[f].surfaces[s].vertexBuffer->drop();
				
				frames[f].surfaces[s].vertexBuffer->cache();
				
				delete [] md3Vertices;
			}
		}

		for (int f = 0; f < numFrames; f++)
		{
			frames[f].surfaces[s].lightmap	   = NULL;
			frames[f].surfaces[s].lightingType = XLT_VERTEXLIGHTING;

			frames[f].surfaces[s].pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

			frames[f].surfaces[s].indexBuffer	       = surfaces[s].indexBuffer;
			frames[f].surfaces[s].persistantAttributes = surfaces[s].texCoords;
			
			frames[f].surfaces[s].shader  = surfaces[s].shader;
			frames[f].surfaces[s].texture = surfaces[s].texture;
		}

		fseek(md3File, surfaceStart + surface.surfaceSize, SEEK_SET);
	}

	numTags = md3Header.numTags;
	tags    = new MD3_TAG[numTags];

	fseek(md3File, md3Header.tagsLumpPos, SEEK_SET);

	for (int f = 0; f < numFrames; f++)
	{
		frames[f].bones = new MD3_BONE[numTags];

		for (int t = 0; t < numTags; t++)
		{
			TMD3_TAG tag;
			fread(&tag, 1, sizeof(TMD3_TAG), md3File);

			if (!f)
			{
				tags[t].index = t;
				tags[t].name = tag.name;
			}

			frames[f].bones[t].position.x = -tag.origin.y;
			frames[f].bones[t].position.y = +tag.origin.z;
			frames[f].bones[t].position.z = +tag.origin.x;

			quaternionFromMatrix3x3(tag.m3x3, &frames[f].bones[t].rotation);
		}
	}

	md3Resources.push_back(this);
	md3ResourcesListPosition = md3Resources.end();

	__super::create(md3Path);
}

bool CMD3Resource::isAnimated()
{
	return (numFrames == 1);
}

// probably temp

int CMD3Resource::getNumFrames()
{
	return numFrames;
}

int CMD3Resource::getNumSurfaces()
{
	return numSurfaces;
}

int CMD3Resource::getNumTags()
{
	return numTags;
}

MD3_FRAME *CMD3Resource::getFrames()
{
	return frames;
}

MD3_SURFACE *CMD3Resource::getSurfaces()
{
	return surfaces;
}

MD3_TAG *CMD3Resource::getTags()
{
	return tags;
}

void CMD3Resource::destroy()
{
	for (int f = 0; f < numFrames; f++)
	{
		for (int s = 0; s < numSurfaces; s++)
		{
			if (!f)
			{
				if (frames[0].surfaces[f].shader)
				{
					frames[0].surfaces[f].shader->release();
				}

				if (frames[0].surfaces[f].texture)
				{
					frames[0].surfaces[f].texture->release();
				}

				frames[0].surfaces[f].indexBuffer->release();

				if (frames[0].surfaces[f].persistantAttributes)
				{
					frames[0].surfaces[f].persistantAttributes->release();
				}
			}

			frames[f].surfaces[f].vertexBuffer->release();
		}
	}

	if (frames)
	{
		delete [] frames;
	}

	if (surfaces)
	{
		delete [] surfaces;
	}

	if (tags)
	{
		delete [] tags;
	}

	md3Resources.erase(md3ResourcesListPosition);

	__super::destroy();
}