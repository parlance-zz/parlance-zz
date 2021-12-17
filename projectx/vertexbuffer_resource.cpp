#include <xtl.h>

#include "vertexbuffer_resource.h"
#include "resource_types.h"
#include "xengine.h"

vector<CResource*> CVertexBufferResource::vertexBufferResources;

int CVertexBufferResource::getResourceType()
{
	return RT_VERTEXBUFFER_RESOURCE;
}

bool CVertexBufferResource::isDynamic()
{
	return _isDynamic;
}
int CVertexBufferResource::getNumVertices()
{
	return numVertices;
}

int CVertexBufferResource::getVertexSize()
{
	return vertexSize;
}

IDirect3DVertexBuffer8 *CVertexBufferResource::getVertexBuffer()
{
	return vertexBuffer;
}

BYTE *CVertexBufferResource::getVertices()
{
	return vertices;
}

DWORD CVertexBufferResource::getFVF()
{
	return fvf;
}

void CVertexBufferResource::create(DWORD fvf, int numVertices, int vertexSize, bool isDynamic, string resourceName)
{
	XEngine->reserveCacheResourceMemory();

	_isDynamic = isDynamic;

	this->numVertices = numVertices;
	this->vertexSize  = vertexSize;

	this->fvf = fvf;

	if (isDynamic)
	{
		if (!(vertices = new BYTE[numVertices * vertexSize]))
		{
			string debugString = "*** ERROR *** Error creating vertex buffer resource '";

			debugString += resourceName;
			debugString += "'\n";

			XEngine->debugPrint(debugString);

			return;
		}
		
		vertexBuffer = NULL;
	}
	else
	{
		if (XEngine->getD3DDevice()->CreateVertexBuffer(numVertices * vertexSize, NULL, NULL, NULL, &vertexBuffer) != D3D_OK)
		{
			string debugString = "*** ERROR *** Error creating vertex buffer resource '";

			debugString += resourceName;
			debugString += "'\n";

			XEngine->debugPrint(debugString);

			return;
		}
		
		vertices = NULL;
	}

	vertexBufferResources.push_back(this);
	vertexBufferResourcesListPosition = vertexBufferResources.end();
	
	__super::create(resourceName);
}

void CVertexBufferResource::destroy()
{
	if (_isDynamic)
	{
		delete [] vertices;
	}
	else
	{
		vertexBuffer->Release();
	}

	vertexBufferResources.erase(vertexBufferResourcesListPosition);

	__super::destroy();
}

void CVertexBufferResource::unload()
{
	if (_isDynamic)
	{
		delete [] vertices;
	}
	else
	{
		vertexBuffer->Release();
	}

	__super::unload();
}

void CVertexBufferResource::recover(bool blockOnRecover)
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
		buffer = vertices = new BYTE[numVertices * vertexSize];
	}
	else
	{
		XEngine->getD3DDevice()->CreateVertexBuffer(numVertices * vertexSize, NULL, NULL, NULL, &vertexBuffer);

		vertexBuffer->Lock(0, numVertices * vertexSize, &buffer, NULL);
	}

	DWORD bytesRead;

	if (!ReadFile(cacheFile, buffer, numVertices * vertexSize, &bytesRead, NULL))
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
			vertexBuffer->Unlock();
		}

		__super::recover(blockOnRecover);
	}
}

void CVertexBufferResource::cache()
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
		buffer = vertices;
	}
	else
	{
		vertexBuffer->Lock(0, numVertices * vertexSize, &buffer, D3DLOCK_READONLY | D3DLOCK_NOFLUSH);
	}

	DWORD bytesWritten;

	if (!WriteFile(cacheFile, buffer, numVertices * vertexSize, &bytesWritten, NULL))
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
			vertexBuffer->Unlock();
		}

		__super::cache();
	}
}