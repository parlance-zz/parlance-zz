#ifndef _H_PHYSICAL_ENTITY
#define _H_PHYSICAL_ENTITY

#include <xtl.h>
#include <list>
#include <stdio.h>

#include "entity.h"

// physical entity class, provides transform, collision, and culling functionality for entities

// todo: store individual rotation matrices, combined rotation matrix, trans matrix, and final combined matrix
// update them only as nessecary

//#define PHYSICAL_ENTITY_DEBUG_SPHERES // usually not defined

struct PHYSICALENTITY_CLUSTER_RESIDENCE_INFO
{
	int cluster;									// the cluster we are resident in
	list<class CPhysicalEntity*>::iterator thisEntity;	// the position in the list of that cluster that references this entity
};

class CPhysicalEntity : public CEntity
{
public:

	virtual int getEntityType();	// returns the correct entity type

	virtual int isEntityA(int baseType); // returns true if the entity is or is derived from anything that is
										 // derived from the base type.

	virtual void create(bool renderEnable, bool collideEnable);  // todo: decide what parameters to take for this: need basic transform info, although
																// collision info is optional, but if it is there, it needs some basic physics information

	virtual void update();
	virtual void physicalUpdate(); // do the collision, find what clusters we're in, etc.

	virtual void destroy(); // additionally, release the transform/collision/physics info

	virtual void render();	// renderable physical entity types should over-ride this and queue their own
							// surface instances with xengine

	static list<CEntity*> physicalEntities;		   // the main global list of all physical entities

	void getPosition(D3DXVECTOR3 *out);
	void setPosition(D3DXVECTOR3 *position);
	
	void getVelocity(D3DXVECTOR3 *out);
	void setVelocity(D3DXVECTOR3 *velocity);

	void getRotation(D3DXVECTOR3 *out);
	void setRotation(D3DXVECTOR3 *rotation);

	void getDirection(D3DXVECTOR3 *out);
	void setDirection(D3DXVECTOR3 *direction);

	void getWorldDirection(D3DXVECTOR3 *localVector, D3DXVECTOR3 *out);
	
	int	getCurrentCluster();
	
	float getBoundingSphereRadius();
	void  setBoundingSphereRadius(float boundingSphereRadius);

	bool getCollisionEnable();
	void setCollisionEnable(bool collideEnable);

	// called after each successive collision in a trace test, used to allow the entity to respond to individual
	// collisions that may occur in a sliding trace. in addition, this function may be called with traceInfo as NULL
	// when this entity is hit by some other entity in that entity's trace

//	virtual void collide(XENGINE_COLLISION_INFO *collisionInfo, XENGINE_COLLISION_TRACE_INFO *traceInfo);

	bool getRenderEnable();
	void setRenderEnable(bool renderEnable);

	int getLastFrameRendered();

	bool isHidden();
	void setHidden(bool isHidden);

	list<PHYSICALENTITY_CLUSTER_RESIDENCE_INFO*> clusterResidence; // information on the clusters which this entity currently occupies
																   // updated by register entity only if renderCollide is true

#ifdef PHYSICAL_ENTITY_DEBUG_SPHERES

	static ID3DXMesh *sphereMesh;

#endif

private:

	// transform properties

	D3DXVECTOR3 position;
	D3DXVECTOR3 rotation;
	D3DXVECTOR3 direction;

	void updateDirection();
	void updateRotation();

	// culling / rendering properties

	int  lastFrameRendered;
	bool _isHidden;

	bool renderEnable; // if this is false, all collision / rendering of this entity will be disabled

	int currentCluster; // the cluster that the position point actually lies in, or -1 if none or unknown

	float boundingSphereRadius; // defines the maximal distance from the position point the entity's vertices
								// OR collision bounds will occupy.

	// todo: physical & collision properties

	bool collisionEnable;

	D3DXVECTOR3 velocity;

	list<CEntity*>::iterator physicalEntitiesListPosition; // keeps track of where in the physical entity list this entity resides
};

#endif