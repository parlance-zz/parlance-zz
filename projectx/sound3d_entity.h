#ifndef _H_SOUND3D_ENTITY
#define _H_SOUND3D_ENTITY

#include <xtl.h>
#include <list>
#include <stdio.h>

#include "physical_entity.h"

class CSound3DEntity : public CPhysicalEntity
{
public:

	virtual int getEntityType();	// returns the correct entity type

	virtual int isEntityA(int baseType); // returns true if the entity is or is derived from anything that is
										 // derived from the base type.

	virtual void create(class CSoundResource *soundResource, float maxDistance = 1000.0f);

	virtual void update();

	virtual void destroy(); // release the idirectsoundbuffer, and such

	void play(bool looping = false);

	void playOnce();
	
    void setPaused(bool isPaused);
	void setVolume(float volume);

	void  setMaxDistance(float maxDistance);
	float getMaxDistance();

	void activate();
	void deactivate();

	bool isActivated();

	static list<CEntity*> sound3DEntities;	   // the main global list of all sound entities

private:
	
	list<CEntity*>::iterator sound3DEntitiesListPosition; // keeps track of where in the sound entity list this entity resides

	bool terminateOnEnd;

	bool  isPaused;
	LONG  volume;

	bool activated;

	int isSoundAcquired;

	CSoundResource *soundResource;
	IDirectSoundBuffer8 *soundBuffer;
};

#endif