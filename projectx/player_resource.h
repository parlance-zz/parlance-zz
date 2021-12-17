#ifndef _H_PLAYER_RESOURCE
#define _H_PLAYER_RESOURCE

#include <xtl.h>
#include <vector>
#include <string>
#include <stdio.h>

#include "resource.h"

#include "md3_resource.h"

#define XENGINE_PLAYERRESOURCE_PLAYERMODELS_PATH	"models\\players\\"
#define XENGINE_PLAYERRESOURCE_PLAYERSOUNDS_PATH	"sounds\\player\\"
#define XENGINE_PLAYERRESOURCE_FOOTSTEPSOUNDS_PATH  "sounds\\player\\footsteps\\"

using namespace std;

enum PLAYER_SEX
{
	PS_NONE = 0,
	PS_MALE,
	PS_FEMALE,
};

/*
enum PLAYER_FOOTSTEPS_TYPE
{
	PFT_DEFAULT = 0,
	PFT_MECHANICAL,
	PFT_FLESH,
	PFT_BOOT,
	PFT_ENERGY
};
*/

enum PLAYER_ANIMATION
{
    PA_DEATH1 = 0,	// animation to be played while dying
    PA_DEAD1,		// and while dead
    PA_DEATH2,		// etc.
    PA_DEAD2,
    PA_DEATH3,
    PA_DEAD3,
    PA_TORSO_GESTURE,	// taunt
    PA_TORSO_ATTACK,	// regular firing attack
    PA_TORSO_ATTACK2,	// melee gauntlet attack
    PA_TORSO_DROP,		// crouch
    PA_TORSO_RAISE,		// uncrouch
    PA_TORSO_STAND,		// idle?
    PA_TORSO_STAND2,	// ?
    PA_LEGS_WALKCROUCHED,
    PA_LEGS_WALK,
    PA_LEGS_RUN,
    PA_LEGS_BACKPEDAL,
    PA_LEGS_SWIM,
    PA_LEGS_JUMP,
    PA_LEGS_LAND,
    PA_LEGS_JUMPB,	// not entirely sure what these might be used for
    PA_LEGS_LANDB,	// possibly just to mix it up a little
    PA_LEGS_IDLE,
    PA_LEGS_IDLECROUCHED,
    PA_LEGS_TURN,	// used for when the legs swivel
	PA_MAXANIMATIONS
};

class CPlayerResource : public CResource
{
public:

	int getResourceType();	// returns the resource type

	void create(string playerPath);

	void destroy();

	MD3_ANIMATION_INFO *getAnimationInfo(int animIndex);

	CMD3Resource *getLower();
	CMD3Resource *getUpper();
	CMD3Resource *getHead();

	int getSex();

	void getHeadOffset(D3DXVECTOR3 *headOffset);

	class CFootstepResource *getFootsteps();

	static vector<CResource*> playerResources; // main player resource list

	static CPlayerResource *getPlayer(string playerPath);

private:
	
	int sex; // some member of the PLAYER_SEX enum

	D3DXVECTOR3 headOffset; // not sure what this is all about or if it's even used

	CFootstepResource *footSteps;	// the sounds used for this player's footsteps
	
	CMD3Resource *upper;
	CMD3Resource *lower;
	CMD3Resource *head;

	MD3_ANIMATION_INFO	animations[PA_MAXANIMATIONS];

	vector<CResource*>::iterator playerResourcesListPosition; // keeps track of where in the main resource list this resource resides
};

#endif