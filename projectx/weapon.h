#ifndef _H_WEAPON
#define _H_WEAPON

#include <xtl.h>
#include <list>
#include <stdio.h>

#include "physical_entity.h"

// the bounding sphere for a weapon when it is loose

#define WEAPON_BOUNDING_SPHERE_RADIUS	16.0f // may need adjusting, seems a little conservative

// used for picking up a weapon off the ground

#define WEAPON_COLLISION_ELLIPSE_RADIUS_X	12.0f
#define WEAPON_COLLISION_ELLIPSE_RADIUS_Y	6.0f
#define WEAPON_COLLISION_ELLIPSE_RADIUS_Z	12.0f

// these control how the weapon appears spinning on the map

#define WEAPON_ROTATE_SPEED	4.0f
#define WEAPON_BOB_HEIGHT	4.0f
#define WEAPON_BOB_SPEED	1.6f

// these control the location of the animation frames in the hand md3 for firing / switching weapon animations

#define WEAPON_FIRING_START_FRAME	0
#define WEAPON_FIRING_END_FRAME		(8 - 1)
#define WEAPON_FIRING_FPS			20.0f

class CWeapon : public CPhysicalEntity
{
public:

	virtual int getEntityType();

	virtual int isEntityA(int baseType);

	virtual void create(class CWeaponResource *weaponResource);

	virtual void update();

	virtual void destroy();

	void render();

	static list<class CEntity*> weaponEntities;

	void setOwner(class CPlayerEntity *player); // can be called with NULL to release it back into the wild

	//class CMeshEntity *getWeaponMesh();
	class CMeshEntity *getHand();

private:

	CPlayerEntity *owner;

	CWeaponResource *weaponResource;

	CMeshEntity *weapon;
	CMeshEntity *barrel;
	CMeshEntity *flash;
	CMeshEntity *hand;

	list<CEntity*>::iterator weaponEntitiesListPosition;
};

#endif