#include "footstep_resource.h"

#include "resource_types.h"
#include "sound_resource.h"
#include "string_help.h"
#include "math_help.h"

vector<CResource*> CFootstepResource::footstepResources;

int CFootstepResource::getResourceType()
{
	return RT_FOOTSTEP_RESOURCE;
}

CFootstepResource *CFootstepResource::getFootstep(string soundPath)
{
	for (int i = 0; i < (int)CFootstepResource::footstepResources.size(); i++)
	{
		CFootstepResource *footstepResource = (CFootstepResource*)CFootstepResource::footstepResources[i];

		if (footstepResource->getResourceName() == soundPath)
		{
			footstepResource->addRef();

			return footstepResource;
		}
	}

	CFootstepResource *footstepResource = new CFootstepResource;

	footstepResource->create(soundPath);

	return footstepResource;
}

void CFootstepResource::create(string footstepPath)
{
	for (int s = 0; s < XENGINE_FOOTSTEPRESOURCE_NUM_FOOTSTEP_SOUNDS; s++)
	{
		string footstepSoundPath = footstepPath;
		footstepSoundPath += intString(s);

		CSoundResource *footstepSound = CSoundResource::getSound(footstepSoundPath);

		if (footstepSound)
		{
			footstepSounds.push_back(footstepSound);
		}
	}

	footstepResources.push_back(this);
	footstepResourcesListPosition = footstepResources.end();

	__super::create(footstepPath);
}

CSoundResource *CFootstepResource::getFootstepSound()
{
	if (footstepSounds.size() > 1)
	{
		return footstepSounds[rand() % int(footstepSounds.size())];
	}
	else if (footstepSounds.size() == 1)
	{
		return footstepSounds[0];
	}
	else
	{
		return NULL;
	}
}

void CFootstepResource::destroy()
{
	footstepResources.erase(footstepResourcesListPosition);

	__super::destroy();
}