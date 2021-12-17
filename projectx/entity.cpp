#include "entity.h"

#include "entity_types.h"
#include "engine_events.h"
#include "xengine.h"

// basic entity functionality
// todo: consider entity messaging system with queues

list<CEntity*> CEntity::entities;

int CEntity::getEntityType()
{
	return ET_ENTITY;
}

int CEntity::isEntityA(int baseType)
{
	if (baseType == ET_ENTITY)
	{
		return true;
	}
	else
	{
		return false;
	}
}

int CEntity::addRef()
{
	return ++numReferences;
}

int CEntity::release()
{
	if (!(--numReferences))
	{
		destroy();
	}
	
	return numReferences;
}

// virtual entity handling logic functions

void CEntity::create()
{
	numReferences = 1;

	entities.push_front(this);
	entitiesListPosition = entities.begin();
}

void CEntity::update()
{
}

void CEntity::destroy()
{
	ENGINE_EVENT *engineEvent = new ENGINE_EVENT;
	EE_ENTITY_TERMINATION_STRUCT *entityTerminationInfo = new EE_ENTITY_TERMINATION_STRUCT;

	entityTerminationInfo->terminatingEntity = this;

	engineEvent->eventType = EE_ENTITY_TERMINATION;
	engineEvent->eventData = (void*)entityTerminationInfo;

	XEngine->queueEngineEvent(engineEvent);

	entities.erase(entitiesListPosition);
}