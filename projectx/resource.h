#ifndef _H_RESOURCE
#define _H_RESOURCE

#include <xtl.h>
#include <vector>
#include <string>
#include <stdio.h>

using namespace std;

// base resource class, provides generic data resource functionality

class CResource
{
public:

	virtual int getResourceType();	// returns the resource type

	int addRef();			// increases the reference counter
	int release();			// decrements the reference counter, if the reference counter equals zero, this calls destroy

	virtual void create(string myResourceName);  // intializes and loads the resource

	virtual void destroy(); // releases all memory associated with the resource, flags the engine
	                        // for this resource's final destruction.

	string	   getResourceName();	// return the source's name (same as filename in most cases)

	static vector<CResource*> resources; // main resource list

private:
	
	string resourceName;

	int numReferences;			// tracks the number of references this resource currently has, starts initially at one.

	vector<CResource*>::iterator resourcesListPosition; // keeps track of where in the main resource list this resource resides
};

#endif