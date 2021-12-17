#ifndef _H_PLAYER
#define _H_PLAYER

#include <xtl.h>
#include <list>
#include <stdio.h>

#include "physical_entity.h"

// it would appear as though the player center coordinate is not actually
// in the center of the player bounding box for some reason... which means things will need adjusting

#define PLAYER_BOUNDING_SPHERE_RADIUS	36.0f	// (2 * (player_width / 2) ^ 2 + (player_height / 2) ^ 2) ^ (1 / 2)

#define PLAYER_HEIGHT	56.0f
#define PLAYER_WIDTH	32.0f

#define PLAYER_LOWER_OFFSET		-4.0f
#define PLAYER_CAMERA_OFFSET	25.0f

// todo: these may need a little adjusting

#define PLAYER_COLLISION_ELLIPSE_RADIUS_X	PLAYER_WIDTH  / 2.0f	 
#define PLAYER_COLLISION_ELLIPSE_RADIUS_Y	PLAYER_HEIGHT / 2.0f
#define PLAYER_COLLISION_ELLIPSE_RADIUS_Z	PLAYER_WIDTH  / 2.0f

enum PLAYER_MOVEMENT_STATE 
{
	PLAYER_MOVEMENT_IDLE = 0,
	PLAYER_MOVEMENT_WALKING = 1,
	PLAYER_MOVEMENT_RUNNING = 2,
	PLAYER_MOVEMENT_BACKPEDALING = 3,
	PLAYER_MOVEMENT_TURNING = 4
};

class CPlayerEntity : public CPhysicalEntity
{
public:

	virtual int getEntityType();

	virtual int isEntityA(int baseType);

	virtual void create(class CPlayerResource *playerResource);

	virtual void update();

	virtual void destroy();

	virtual void render();

	virtual void giveWeapon(class CWeapon *weapon);

	static list<class CEntity*> playerEntities;

protected:

	class CCameraEntity *camera;

	CPlayerResource *playerResource;

	class CMeshEntity *lower;
	CMeshEntity *upper;
	CMeshEntity *head;

	CWeapon *currentWeapon;

	int movementState;

private:

	list<CEntity*>::iterator playerEntitiesListPosition;
};

#endif