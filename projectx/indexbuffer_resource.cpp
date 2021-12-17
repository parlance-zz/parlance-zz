#include <xtl.h>

#include "indexbuffer_resource.h"
#include "resource_types.h"
#include "xengine.h"

vector<CResource*> CIndexBufferResource::indexBufferResources;

int CIndexBufferResource::getResourceType()
{
	return RT_INDEXBUFFER_RESOURCE;
}

int CIndexBufferResource::getNumIndices()
{
	return numIndices;
}

int CIndexBufferResource::getNumTriangles()
{
	return numTriangles;
}

int CIndexBufferResource::getVertexBufferOffset()
{
	return vertexBufferOffset;
}

bool CIndexBufferResource::isDynamic()
{
	return _isDynamic;
}

IDirect3DIndexBuffer8 *CIndexBufferResource::getIndexBuffer()
{
	return indexBuffer;
}

unsigned short *CIndexBufferResource::getIndices()
{
	return indices;
}

void CIndexBufferResource::create(int vertexBufferOffset, int numIndices, bool isDynamic, string resourceName)
{
	XEngine->reserveCacheResourceMemory();

	_isDynamic = isDynamic;

	this->vertexBufferOffset = vertexBufferOffset;

	this->numIndices    = numIndices;
	this->numTriangles  = numIndices / 3;

	if (isDynamic)
	{
		if (!(indices = new unsigned short[numIndices]))
		{
			string debugString = "*** ERROR *** Error creating index buffer resource '";

			debugString += resourceName;
			debugString += "'\n";

			XEngine->debugPrint(debugString);

			return;
		}
		
		indexBuffer = NULL;
	}
	else
	{
		if (XEngine->getD3DDevice()->CreateIndexBuffer(sizeof(unsigned short) * numIndices, NULL, D3DFMT_INDEX16, NULL, &indexBuffer) != D3D_OK)
		{
			string debugString = "*** ERROR *** Error creating index buffer resource '";

			debugString += resourceName;
			debugString += "'\n";

			XEngine->debugPrint(debugString);

			return;
		}
		
		indices = NULL;
	}

	indexBufferResources.push_back(this);
	indexBufferResourcesListPosition = indexBufferResources.end();
	
	__super::create(resourceName);
}

void CIndexBufferResource::destroy()
{
	if (_isDynamic)
	{
		delete [] indices;
	}
	else
	{
		indexBuffer->Release();
	}

	indexBufferResources.erase(indexBufferResourcesListPosition);

	__super::destroy();
}

void CIndexBufferResource::unload()
{
	if (_isDynamic)
	{
		delete [] indices;
	}
	else
	{
		indexBuffer->Release();
	}

	__super::unload();
}

void CIndexBufferResource::recover(bool blockOnRecover)
{
	// todo: don't ignore blockOnRecover

	string cachePath = XENGINE_RESOURCES_CACHE_PATH;
	cachePath += getResourceName();

	HANDLE cacheFile;
	
	if ((cacheFile = CreateFile(cachePath.c_str(), GENERIC_READ, 0, NULL, OPEN_ALWAYS,
								  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		string debugString = "*** ERROR *** Failed recovering resource '";

		debugString += getResourceName();
		debugString += "'!\n";

		XEngine->debugPrint(debugString);

		return;
	}

	BYTE *buffer;

	if (_isDynamic)
	{
		indices = new unsigned short[numIndices];

		buffer = (BYTE*)indices;
	}
	else
	{
		XEngine->getD3DDevice()->CreateIndexBuffer(sizeof(unsigned short) * numIndices, NULL, D3DFMT_INDEX16, NULL, &indexBuffer);

		indexBuffer->Lock(0, sizeof(unsigned short) * numIndices, &buffer, NULL);
	}

	DWORD bytesRead;

	if (!ReadFile(cacheFile, buffer, sizeof(unsigned short) * numIndices, &bytesRead, NULL))
	{
		string debugString = "*** ERROR *** Failed recovering resource '";

		debugString += getResourceName();
		debugString += "'!\n";

		XEngine->debugPrint(debugString);

		CloseHandle(cacheFile);
	}
	else
	{
		CloseHandle(cacheFile);

		if (!_isDynamic)
		{
			indexBuffer->Unlock();
		}

		__super::recover(blockOnRecover);
	}
}

void CIndexBufferResource::cache()
{
	string cachePath = XENGINE_RESOURCES_CACHE_PATH;
	cachePath += getResourceName();

	XEngine->validatePath(cachePath);

	HANDLE cacheFile;
	
	if ((cacheFile = CreateFile(cachePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
								  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE)
	{
		string debugString = "*** ERROR *** Failed caching resource '";

		debugString += getResourceName();
		debugString += "'!\n";

		XEngine->debugPrint(debugString);

		return;
	}

	BYTE *buffer;

	if (_isDynamic)
	{
		buffer = (BYTE*)indices;
	}
	else
	{
		indexBuffer->Lock(0, sizeof(unsigned short) * numIndices, &buffer, NULL);
	}

	DWORD bytesWritten;

	if (!WriteFile(cacheFile, buffer, sizeof(unsigned short) * numIndices, &bytesWritten, NULL))
	{
		string debugString = "*** ERROR *** Failed caching resource '";

		debugString += getResourceName();
		debugString += "'!\n";

		XEngine->debugPrint(debugString);

		CloseHandle(cacheFile);
	}
	else
	{
		CloseHandle(cacheFile);

		if (!_isDynamic)
		{
			indexBuffer->Unlock();
		}

		__super::cache();
	}
}