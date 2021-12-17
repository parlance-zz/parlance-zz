#ifndef _H_FOOTSTEP_RESOURCE
#define _H_FOOTSTEP_RESOURCE

#include <xtl.h>
#include <vector>
#include <string>
#include <stdio.h>

#include "resource.h"

using namespace std;

#define XENGINE_FOOTSTEPRESOURCE_NUM_FOOTSTEP_SOUNDS	4

class CFootstepResource : public CResource
{
public:

	int getResourceType();

	void create(string footstepPath);

	void destroy();

	static vector<CResource*> footstepResources;

	class CSoundResource *getFootstepSound(); // gets a random footstep noise
	
	static CFootstepResource *getFootstep(string soundPath);

private:
	
	vector<class CSoundResource*> footstepSounds;

	vector<CResource*>::iterator footstepResourcesListPosition;
};

#endif