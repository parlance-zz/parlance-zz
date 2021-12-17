#ifndef _H_MESH_ENTITY
#define _H_MESH_ENTITY

#include <xtl.h>
#include <list>
#include <stdio.h>

#include "physical_entity.h"
#include "md3_resource.h"

class CMeshEntity : public CPhysicalEntity
{
public:

	virtual int getEntityType();	// returns the correct entity type

	virtual int isEntityA(int baseType); // returns true if the entity is or is derived from anything that is
										 // derived from the base type.

	virtual void create(CMD3Resource *md3Resource); // creates this mesh as a stand-alone mesh instance that isn't bound
													// to any particular hierarchy.

	void addChild(CMeshEntity *child, int boneIndex);
	void removeChild(int boneIndex);

	void link(CMeshEntity *parent, string tagName);	// links this entity to a parent and bone.

	void unlink(); // unlinks this entity from any parent and makes it once again become free from any
				   // parent hierarchy transform.

	CMD3Resource *getMD3Resource();

	// animation stuff

	void setAnimation(MD3_ANIMATION_INFO *animation);
	void getAnimation(MD3_ANIMATION_INFO *animation);

	// anim speed is a animation speed multiplier that affects the current animation
	// setting the current animation resets the multiplier back to 1.0f

	void  setAnimSpeed(float animSpeed);
	float getAnimSpeed();

	// animTime is measured in frames
	// setting the current animation resets the animTime to 0.0f

	float getAnimTime();
	void  setAnimTime(float animTime);
	
	// entity stuff

	virtual void update(); // updates the mesh's animation and bounding sphere information

	virtual void destroy();

	virtual void render();
	virtual void render(D3DXMATRIX *parentTransform);

	// todo: functions for setting up and playing animations and stuff

	static list<class CEntity*> meshEntities;		   // the main global list of all mesh entities

private:

	void _render(D3DXMATRIX *parentTransform);

	// hierarchy stuff

	CMeshEntity *parent;
	int boneIndex;

	CMD3Resource *md3Resource;	// the mesh resouce this entity is instancing

	// animation stuff

	MD3_ANIMATION_INFO currentAnimation;

	float animTime;
	float animSpeed;

	// md3 stuff

	int numSurfaces;	// this is actually the number of surface instances rather than surfaces. corresponds to the number
						// of surfaces in this entity's associated md3 resource.

	struct XSURFACE_INSTANCE *surfaces;

	int numChildren;
	CMeshEntity **children;

	list<CEntity*>::iterator meshEntitiesListPosition; // keeps track of where in the mesh entity list this entity resides
};

#endif