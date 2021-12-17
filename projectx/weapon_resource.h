#ifndef _H_WEAPON_RESOURCE
#define _H_WEAPON_RESOURCE

#include <xtl.h>
#include <vector>
#include <string>
#include <stdio.h>

#include "resource.h"

using namespace std;

class CWeaponResource : public CResource
{
public:

	int getResourceType();	// returns the resource type

	void create(string weaponPath);

	void destroy();

	static CWeaponResource *getWeapon(string weaponPath);

	static vector<CResource*> weaponResources; // main player resource list

	class CMD3Resource *getWeapon();
	CMD3Resource	   *getBarrel(); // can be NULL if no barrel is present in this particular weapon
	CMD3Resource	   *getFlash();
	CMD3Resource	   *getHand();

private:
	
	CMD3Resource *weapon;
	CMD3Resource *barrel;	// can be NULL
	CMD3Resource *flash;
	CMD3Resource *hand;

	vector<CResource*>::iterator weaponResourcesListPosition; // keeps track of where in the main resource list this resource resides
};

#endif