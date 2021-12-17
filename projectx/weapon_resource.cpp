#include "weapon_resource.h"

#include "resource_types.h"
#include "md3_resource.h"
#include "xengine.h"

vector<CResource*> CWeaponResource::weaponResources;

int CWeaponResource::getResourceType()
{
	return RT_WEAPON_RESOURCE;
}

CMD3Resource *CWeaponResource::getWeapon()
{
	return weapon;
}

CMD3Resource *CWeaponResource::getBarrel()
{
	return barrel;
}

CMD3Resource *CWeaponResource::getFlash()
{
	return flash;
}

CMD3Resource *CWeaponResource::getHand()
{
	return hand;
}

CWeaponResource *CWeaponResource::getWeapon(string weaponPath)
{
	for (int i = 0; i < (int)CWeaponResource::weaponResources.size(); i++)
	{
		CWeaponResource *weaponResource = (CWeaponResource*)CWeaponResource::weaponResources[i];

		if (weaponResource->getResourceName() == weaponPath)
		{
			weaponResource->addRef();

			return weaponResource;
		}
	}

	CWeaponResource *weaponResource = new CWeaponResource;

	weaponResource->create(weaponPath);

	return weaponResource;
}

void CWeaponResource::create(string weaponPath)
{
	string explicitPath = XENGINE_RESOURCES_MEDIA_PATH;
	explicitPath += weaponPath;

	string explicitWeaponPath = weaponPath + ".md3";
	weapon = CMD3Resource::getMD3(explicitWeaponPath);

	string flashPath = weaponPath + "_flash.md3";
	flash = CMD3Resource::getMD3(flashPath);

	string handPath = weaponPath + "_hand.md3";
	hand = CMD3Resource::getMD3(handPath);

	string checkBarrelPath = explicitPath + "_barrel.md3";

	FILE *fp = fopen(checkBarrelPath.c_str(), "rb");

	if (fp)
	{
		fclose(fp);

		string barrelPath = weaponPath + "_barrel.md3";
		barrel = CMD3Resource::getMD3(barrelPath);
	}
	else
	{
		barrel = NULL;
	}

	weaponResources.push_back(this);
	weaponResourcesListPosition = weaponResources.end();

	__super::create(weaponPath);
}

void CWeaponResource::destroy()
{
	weaponResources.erase(weaponResourcesListPosition);

	__super::destroy();
}