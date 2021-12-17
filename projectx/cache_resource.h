#ifndef _H_CACHE_RESOURCE
#define _H_CACHE_RESOURCE

#include <stdio.h>
#include <string>
#include <vector>

#include "resource.h"

enum CACHERESOURCE_ACQUISITION_STATE
{
	CRAS_UNACQUIRED = 0,
	CRAS_ACQUIRING,
	CRAS_ACQUIRED
};

class CCacheResource : public CResource
{
public:

	virtual int getResourceType();

	virtual void create(string resourceName); // sets up our data members

	virtual void destroy(); // destroys the cache file for this resource
	
	DWORD getLastReferenceTime(); // pretty obvious
	
	int acquire(bool blockOnRecover); // first, of all, updates lastReferenceTime. checks if the resource is loaded, if not, check if the resource is cached.
		    		// if the resource is not loaded and not cached, we're fucked. big error. otherwise,
				    // call XEngine->reserveCacheResourceMemory(). this function will check if we're under our given threshold
					// for memory usage and if we aren't it will eject the most stale cache-able resource from memory
					// and continue until we're below our threshold. then we go ahead and get the data back from the file.
					// if blockOnRecover is set, this will happen synchronously and execution will block until we get our
					// resource back. if blockonrecover is not set, this happens asychronously. if we go synchronously,
					// simply set the acquisition state to acquired, 

	void drop();    // decrements the acquisitionReferences to 0

	virtual void unload();
	virtual void cache();

	bool isUnloadable();

	static vector<CResource*> cacheResources;	// main cache resource list

protected:

	virtual void recover(bool blockOnRecover);

private:

	// this is a list rather than a vector for speed reasons, for now

	vector<CResource*>::iterator cacheResourcesListPosition; // keeps track of where in the cache resource list this resource resides
	
	int numAcquisitionReferences;
	int	acquisitionState; // some member of the CACHERESOURCE_ACQUISITION_STATE enum
	
	bool isCached;   // if this is true the resource has a cache file, otherwise it has not yet been built
	bool isLoaded; 
	 
	DWORD lastReferenceTime;
};

#endif