#include <xtl.h>

#include "cache_resource.h"
#include "resource_types.h"
#include "xengine.h"

vector<CResource*> CCacheResource::cacheResources;

int CCacheResource::getResourceType()
{
	return RT_CACHE_RESOURCE;
}

void CCacheResource::create(string resourceName)
{
	numAcquisitionReferences = 0;

	lastReferenceTime = XEngine->getMillisecs();

	isCached = false;
	isLoaded = true;

	acquisitionState = CRAS_UNACQUIRED;

	cacheResources.push_back(this);
	cacheResourcesListPosition = cacheResources.end();

	__super::create(resourceName);
}

void CCacheResource::destroy()
{
	if (isCached)
	{
		string cachePath = XENGINE_RESOURCES_CACHE_PATH;
		cachePath += getResourceName();

		if (!DeleteFile(cachePath.c_str()))
		{
			string debugString = "*** ERROR *** Error deleting cache file for resource '";

			debugString += getResourceName();
			debugString += "'!\n";

			XEngine->debugPrint(debugString);
		}
	}

	cacheResources.erase(cacheResourcesListPosition);

	__super::destroy();
}
	
DWORD CCacheResource::getLastReferenceTime()
{
	return lastReferenceTime;
}
	
int CCacheResource::acquire(bool blockOnRecover)
{
	lastReferenceTime = XEngine->getMillisecs();

	switch (acquisitionState)
	{
	case CRAS_ACQUIRED:

		numAcquisitionReferences++;
		return true;

	case CRAS_ACQUIRING:

		return false;
	}

	if (isLoaded)
	{
		acquisitionState = CRAS_ACQUIRED;
		numAcquisitionReferences++;

		return true;
	}
	else
	{
		if (isCached)
		{
			// todo: asynchronous recover unsupported for now, all recovers will be synchronous

			//if (blockOnRecover)
			//{
				XEngine->reserveCacheResourceMemory();

				recover(true);

				acquisitionState = CRAS_ACQUIRED;
				numAcquisitionReferences = 1;

				return true;
			//}
			//else
			//{
			//	XEngine->reserveCacheResourceMemory();

			//	recover(false);

			//	return false;
			//}
		}
		else
		{
			string debugString = "*** ERROR *** Attempted acquisition of uncached, unloaded cache resource '";

			debugString += getResourceName();
			debugString += "'!\n";

			XEngine->debugPrint(debugString);

			DebugBreak(); // todo: temp

			return false;
		}
	}
}

void CCacheResource::drop()
{
	if (numAcquisitionReferences)
	{
		numAcquisitionReferences--;

		if (!numAcquisitionReferences)
		{
			acquisitionState = CRAS_UNACQUIRED;
		}
	}
	else
	{
		string debugString = "*** ERROR *** Acquire/drop count mismatch for resource '";

		debugString += getResourceName();
		debugString += "'!\n";

		XEngine->debugPrint(debugString);
	}
}

// any derived cache resource class needs to call these after they finish their recover/cache functions

bool CCacheResource::isUnloadable()
{
	if (acquisitionState != CRAS_ACQUIRING && !numAcquisitionReferences && isCached && isLoaded)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CCacheResource::recover(bool blockOnRecover)
{
	isLoaded = true;
}

void CCacheResource::cache()
{
	isCached = true;
}

void CCacheResource::unload()
{
	isLoaded = false;
}