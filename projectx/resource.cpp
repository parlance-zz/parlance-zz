#include "resource.h"
#include "resource_types.h"
#include "engine_events.h"
#include "xengine.h"

vector<CResource*> CResource::resources;

int CResource::getResourceType()
{
	return RT_RESOURCE;
}

int CResource::addRef()
{
	return ++numReferences;
}

int CResource::release()
{
	if (!--numReferences)
	{
		destroy();
	}

	return numReferences;
}

void CResource::create(string myResourceName)
{
	resourceName = myResourceName;

	numReferences = 1;

	resources.push_back(this);
	resourcesListPosition = resources.end();
}

void CResource::destroy()
{
	ENGINE_EVENT *engineEvent = new ENGINE_EVENT;
	EE_RESOURCE_TERMINATION_STRUCT *resourceTerminationInfo = new EE_RESOURCE_TERMINATION_STRUCT;

	resourceTerminationInfo->terminatingResource = this;

	engineEvent->eventType = EE_RESOURCE_TERMINATION;
	engineEvent->eventData = (void*)resourceTerminationInfo;

	XEngine->queueEngineEvent(engineEvent);

	resources.erase(resourcesListPosition);
}

string CResource::getResourceName()
{
	return resourceName;
}