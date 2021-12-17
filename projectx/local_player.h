#ifndef _H_LOCALPLAYER_ENTITY
#define _H_LOCALPLAYER_ENTITY

#include <xtl.h>
#include <list>
#include <stdio.h>

#include "player.h"

#define GAME_LOCALPLAYER_MAX_LOCALPLAYERS	4

class CLocalPlayer : public CPlayerEntity
{
public:

	virtual int getEntityType();

	virtual int isEntityA(int baseType);

	virtual void create(class CPlayerResource *playerResource, int controllerNum);

	virtual void update();

	virtual void destroy();

	static list<class CEntity*> localPlayerEntities;

	class CCameraEntity *getCamera();

	int getControllerNum();

	static void updateLocalPlayerViewports();
	static bool isControllerInUse(int controllerNum);

private:

	int localPlayerNum;
	int controllerNum;

	list<CEntity*>::iterator localPlayerEntitiesListPosition;
};

#endif