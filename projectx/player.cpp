#include <xtl.h>

#include "player.h"

#include "player_resource.h"
#include "mesh_entity.h"
#include "math_help.h"
#include "xengine.h"
#include "entity_types.h"
#include "weapon.h"
#include "camera_entity.h"

list<CEntity*> CPlayerEntity::playerEntities;

int CPlayerEntity::getEntityType()
{
	return ET_PLAYER_ENTITY;
}

int CPlayerEntity::isEntityA(int baseType)
{
	if (baseType == ET_PLAYER_ENTITY)
	{
		return true;
	}
	else
	{
		return  __super::isEntityA(baseType);
	}
}

void CPlayerEntity::giveWeapon(CWeapon *weapon)
{
	currentWeapon = weapon;

	weapon->setOwner(this);

	weapon->getHand()->link(upper, "tag_weapon");
}

void CPlayerEntity::create(CPlayerResource *playerResource)
{
	this->playerResource = playerResource;

	lower = new CMeshEntity;
	upper = new CMeshEntity;
	head  = new CMeshEntity;

	lower->create(playerResource->getLower());
	upper->create(playerResource->getUpper());
	head->create(playerResource->getHead());

	lower->setRenderEnable(false);
	upper->setRenderEnable(false);
	head->setRenderEnable(false);

	lower->setCollisionEnable(false);
	upper->setCollisionEnable(false);
	head->setCollisionEnable(false);

	upper->link(lower, "tag_torso");
	head->link(upper, "tag_head");

	currentWeapon = NULL;

	movementState = PLAYER_MOVEMENT_IDLE;

	__super::create(true, true);

	setBoundingSphereRadius(PLAYER_BOUNDING_SPHERE_RADIUS);
	
	// todo: setup collision

	update();

	// stuff

	playerEntities.push_front(this);
	playerEntitiesListPosition = playerEntities.begin();
}

void CPlayerEntity::update()
{
	// todo: actual..stuff

	D3DXVECTOR3 playerPosition;
	getPosition(&playerPosition);

	D3DXVECTOR3 playerRotation;
	getRotation(&playerRotation);

	// setup lower

	D3DXVECTOR3 lowerPosition = D3DXVECTOR3(playerPosition.x, playerPosition.y + PLAYER_LOWER_OFFSET, playerPosition.z);
	lower->setPosition(&lowerPosition);

	//D3DXVECTOR3 legsRotation = D3DXVECTOR3(0.0f, playerRotation.y, 0.0f);
	//lower->setRotation(&legsRotation);

	D3DXVECTOR3 legsRotation; lower->getRotation(&legsRotation);
	
	// setup upper

	D3DXVECTOR3 upperRotation = D3DXVECTOR3(playerRotation.x * 0.8f, playerRotation.y - legsRotation.y, 0.0f);
	upper->setRotation(&upperRotation);

	// setup head

	D3DXVECTOR3 headRotation = D3DXVECTOR3(playerRotation.x * 0.3f, 0.0f, 0.0f);
	head->setRotation(&headRotation);

	if (camera)
	{
		// some other local update stuff
		// make the camera bob and stuff
		// and if there's a weapon, make it bob appropriately too

		D3DXVECTOR3 cameraPosition = D3DXVECTOR3(playerPosition.x - 80.0f, playerPosition.y + 200.0f, playerPosition.z - 80.0f);

			//D3DXVECTOR3(playerPosition.x, playerPosition.y + PLAYER_CAMERA_OFFSET, playerPosition.z);

		camera->setPosition(&cameraPosition);

		D3DXVECTOR3 cameraDirection = playerPosition - cameraPosition;
		camera->setDirection(&cameraDirection);
		//camera->setRotation(&playerRotation);
	}

	__super::update();
}

void CPlayerEntity::render()
{
	lower->render();

	__super::render();
}

void CPlayerEntity::destroy()
{
	lower->release();
	upper->release();
	head->release();

	if (camera)
	{
		camera->release();
	}

	playerResource->release();

	playerEntities.erase(playerEntitiesListPosition);
	
	__super::destroy();
}