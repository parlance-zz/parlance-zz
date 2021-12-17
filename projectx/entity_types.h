#ifndef _H_ENTITY_TYPES
#define _H_ENTITY_TYPES

enum ENTITY_TYPE
{
	ET_ENTITY = 0,
	ET_PHYSICAL_ENTITY,
	ET_CAMERA_ENTITY,
	ET_SOUND_ENTITY,
	ET_SOUND3D_ENTITY,
	ET_MESH_ENTITY,
	ET_PLAYER_ENTITY,
	ET_WEAPON_ENTITY,
	ET_LOCALPLAYER_ENTITY,

	//todo, include all other entity types in here
};

/*
class CEntityFactory
{
public:

	class CEntity *createEntity(string className, string *args, int numArgs);

private:

	// todo:
};
*/

#endif