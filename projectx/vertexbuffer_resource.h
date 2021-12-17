#ifndef _H_VERTEXBUFFER_RESOURCE
#define _H_VERTEXBUFFER_RESOURCE

#include <xtl.h>
#include <string>
#include <stdio.h>
#include <vector>

#include "cache_resource.h"

using namespace std;

class CVertexBufferResource : public CCacheResource
{
public:

	int getResourceType();

	void create(DWORD fvf, int numVertices, int vertexSize, bool isDynamic, string resourceName);

	void destroy(); // additionally, this releases the vertex buffers / mem

	bool isDynamic();

	int getNumVertices();
	int getVertexSize();

	IDirect3DVertexBuffer8 *getVertexBuffer();
	BYTE				   *getVertices();

	DWORD getFVF();

	static vector<CResource*> vertexBufferResources;	// main vertex buffer resource list

	// caching mechanisms

	void unload();
	void cache();

protected:

	void recover(bool blockOnRecover);

private:

	vector<CResource*>::iterator vertexBufferResourcesListPosition; // keeps track of where in the vertexbuffer resource list this resource resides

	IDirect3DVertexBuffer8 *vertexBuffer;
	BYTE				   *vertices;

	DWORD fvf;

	int numVertices;
	int vertexSize;

	bool _isDynamic;
};

#endif