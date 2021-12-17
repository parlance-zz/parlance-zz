#ifndef _H_INDEXBUFFER_RESOURCE
#define _H_INDEXBUFFER_RESOURCE

#include <xtl.h>
#include <string>
#include <stdio.h>
#include <vector>

#include "cache_resource.h"

using namespace std;

class CIndexBufferResource : public CCacheResource
{
public:

	int getResourceType();

	void create(int vertexBufferOffset, int numIndices, bool isDynamic, string resourceName);

	void destroy(); // additionally, this releases the index buffers / mem

	IDirect3DIndexBuffer8 *getIndexBuffer();
	unsigned short		  *getIndices();

	bool isDynamic();

	int getVertexBufferOffset();
	int getNumIndices();
	int getNumTriangles();

	static vector<CResource*> indexBufferResources;	// main index buffer resource list

	// caching mechanisms

	void unload();
	void cache();

protected:

	void recover(bool blockOnRecover);
	
private:

	vector<CResource*>::iterator indexBufferResourcesListPosition; // keeps track of where in the indexbuffer resource list this resource resides

	IDirect3DIndexBuffer8 *indexBuffer;
	unsigned short		  *indices;

	int vertexBufferOffset;
	int numIndices;
	int numTriangles;

	bool _isDynamic;
};

#endif