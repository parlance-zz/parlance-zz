#ifndef _H_ENGINE_EVENTS
#define _H_ENGINE_EVENTS

// engine event type enumeration stores all the different kinds of engine event types

enum ENGINE_EVENT_TYPE
{
	EE_ENTITY_TERMINATION = 0,
	EE_RESOURCE_TERMINATION,
	EE_APPLICATION_TERMINATION,
	EE_CONTROLLER_CONNECTION,
	EE_CONTROLLER_DISCONNECTION,
};

// engine event structure is used to store global game events received
// from entities or resources in a queue to be handled when a frame begins

struct ENGINE_EVENT
{
	int eventType;
	void *eventData;
};

// todo: insert structures for all engine event datas here

struct EE_ENTITY_TERMINATION_STRUCT
{
	class CEntity *terminatingEntity;
};

struct EE_RESOURCE_TERMINATION_STRUCT
{
	class CResource *terminatingResource;
};

struct EE_CONTROLLER_CONNECTION_STRUCT
{
	int portNum; // todo: any other info that would be relevant here?
};

struct EE_CONTROLLER_DISCONNECTION_STRUCT
{
	int portNum; // todo: ditto
};

enum APPLICATION_TERMINATION_CODE
{
	APPTERM_SUCCESS = 0,
	APPTERM_ERROR,
};

struct EE_APPLICATION_TERMINATION_STRUCT
{
	int terminationCode;  // a member of the APPLICATION_TERMINATION_CODE enumeration
	string detailedError; // if there was an error, the reason will be explained here
};

#endif