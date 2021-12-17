#ifndef _H_SOUND_ENTITY
#define _H_SOUND_ENTITY

#include <xtl.h>
#include <list>
#include <stdio.h>

#include "entity.h"

class CSoundEntity : public CEntity
{
public:

	virtual int getEntityType();	// returns the correct entity type

	virtual int isEntityA(int baseType); // returns true if the entity is or is derived from anything that is
										 // derived from the base type.

	virtual void create(class CSoundResource *soundResource);

	virtual void update();

	virtual void destroy(); // release the idirectsoundbuffer, and such

	void play(bool looping = false);

	static list<CEntity*> soundEntities;	   // the main global list of all sound entities


private:
	
	list<CEntity*>::iterator soundEntitiesListPosition; // keeps track of where in the sound entity list this entity resides

	IDirectSoundBuffer8 *soundBuffer;
};

#endif