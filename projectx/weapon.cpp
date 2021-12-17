#include "weapon.h"

#include "weapon_resource.h"
#include "mesh_entity.h"
#include "math_help.h"
#include "xengine.h"
#include "entity_types.h"

list<CEntity*> CWeapon::weaponEntities;

int CWeapon::getEntityType()
{
	return ET_WEAPON_ENTITY;
}

int CWeapon::isEntityA(int baseType)
{
	if (baseType == ET_WEAPON_ENTITY)
	{
		return true;
	}
	else
	{
		return  __super::isEntityA(baseType);
	}
}

/*
CMeshEntity *CWeapon::getWeaponMesh()
{
	return weapon;
}
*/

CMeshEntity *CWeapon::getHand()
{
	return weapon;//hand;
}

void CWeapon::setOwner(CPlayerEntity *player)
{
	owner = player;

	if (owner)
	{
		setRenderEnable(false);
		setCollisionEnable(false);

		D3DXVECTOR3 weaponRotation = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		hand->setRotation(&weaponRotation);
	}
	else
	{
		hand->unlink();

		setRenderEnable(true);
		setCollisionEnable(true);
	}
}

void CWeapon::create(CWeaponResource *weaponResource)
{
	this->weaponResource = weaponResource;

	hand = new CMeshEntity;
	hand->create(weaponResource->getHand());

	weapon = new CMeshEntity;
	weapon->create(weaponResource->getWeapon());
	weapon->link(hand, "tag_weapon");

	flash = new CMeshEntity;
	flash->create(weaponResource->getFlash());
	flash->link(hand, "tag_flash");
	flash->setHidden(true);

	if (weaponResource->getBarrel())
	{
		barrel = new CMeshEntity;
		barrel->create(weaponResource->getBarrel());
		barrel->link(hand, "tag_barrel");
	}
	else
	{
		barrel = NULL;
	}

	__super::create(true, true);

	setOwner(NULL);

	setBoundingSphereRadius(WEAPON_BOUNDING_SPHERE_RADIUS);

	update();

	// stuff

	weaponEntities.push_front(this);
	weaponEntitiesListPosition = weaponEntities.begin();
}

void CWeapon::update()
{
	if (!owner)
	{
		D3DXVECTOR3 position;
		getPosition(&position);

		D3DXVECTOR3 rotation;
		getRotation(&rotation);

		rotation.y += WEAPON_ROTATE_SPEED * XEngine->getTween();
		setRotation(&rotation);

		D3DXVECTOR3 weaponRotation = D3DXVECTOR3(0.0f, rotation.y, 0.0f);
		hand->setRotation(&weaponRotation);

		D3DXVECTOR3 weaponPosition = D3DXVECTOR3(position.x, position.y + sinf(D3DXToRadian(rotation.y * WEAPON_BOB_SPEED)) * WEAPON_BOB_HEIGHT, position.z);
		hand->setPosition(&weaponPosition);
	}

	if (barrel)
	{
		D3DXVECTOR3 barrelRotation;
		barrel->getRotation(&barrelRotation);

		barrelRotation.z += 10.0f * XEngine->getTween();
		barrel->setRotation(&barrelRotation);
	}

	__super::update();
}

void CWeapon::render()
{
	if (!owner)
	{
		hand->render();		
	}

	__super::render();
}

void CWeapon::destroy()
{
	weaponResource->release();

	hand->unlink();
	hand->release();

	weapon->unlink();
	weapon->release();

	if (barrel)
	{
		barrel->unlink();
		barrel->release();
	}

	flash->unlink();
	flash->release();

	weaponEntities.erase(weaponEntitiesListPosition);
	
	__super::destroy();
}