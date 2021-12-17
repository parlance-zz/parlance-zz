#ifndef _H_ENTITY
#define _H_ENTITY

#include <stdio.h>
#include <list>
#include <vector>
#include <string>

using namespace std;

/*
struct ENTITY_FIELD
{
	string property;
	string value;
};
*/

// base entity class, provides basic entity functionality

class CEntity
{
public:

	virtual int getEntityType();	// returns the correct entity type

	virtual int isEntityA(int baseType); // returns true if the entity is or is derived from anything that is
										 // derived from the base type.

	int addRef();			// increases the reference counter
	int release();			// decrements the reference counter, if the reference counter equals zero, this calls destroy

	virtual void create();  // intializes and binds all resources and other entities associated with this entity. sets up
							// entity member data and possibly takes some relevant parameters.

	virtual void update();  // update anything the entity needs to, called once per entity per frame

	virtual void destroy(); // actually releases all memory associated with the entity, flags the engine
	                        // for this entity's final destruction.

	static list<CEntity*> entities;	   // global list that keeps track of all entities	

private:
	
	int numReferences;			// tracks the number of references this entity currently has, starts initially at one.

	list<CEntity*>::iterator entitiesListPosition; // keeps track of where in the main entity list this entity resides

};

#endif