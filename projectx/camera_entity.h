#ifndef _H_CAMERA_ENTITY
#define _H_CAMERA_ENTITY

#include <xtl.h>
#include <list>
#include <stdio.h>

#include "physical_entity.h"
#include "xengine.h"

#define XENGINE_DEFAULT_CAMERA_FOV				D3DX_PI / 3.0f // 2.0f
#define XENGINE_DEFAULT_CAMERA_NEARCLIP			1.0f
#define XENGINE_DEFAULT_CAMERA_FARCLIP			10000.0f

enum CAMERA_FRUSTUMTEST_RESULT
{
	CAMERA_FTR_COMPLETE_OUT = 0,
	CAMERA_FTR_INTERSECT,
	CAMERA_FTR_COMPLETE_IN
};

enum CAMERA_FRUSTUMSIDE
{
    CAMERA_FS_RIGHT = 0,
    CAMERA_FS_LEFT,
    CAMERA_FS_BOTTOM,
    CAMERA_FS_TOP,
    CAMERA_FS_BACK,
    CAMERA_FS_FRONT
}; 

class CCameraEntity : public CPhysicalEntity
{
public:

	virtual int getEntityType();	// returns the correct entity type

	virtual int isEntityA(int baseType); // returns true if the entity is or is derived from anything that is
										 // derived from the base type.

	virtual void create();  // todo:

	virtual void destroy(); // todo:

	void activate(); 
	
	void setFOV(float fov);
	float getFOV();

	// visibility testing methods

	int pointInFrustum(float x, float y, float z);
	int sphereInFrustum(float x, float y, float z, float radius);
	int boxInFrustum(float x, float y, float z, float x2, float y2, float z2);

	void setProjection(float fov, float aspectRatio, float nearClip, float farClip);
	void setViewport(int x, int y, int width, int height);

	static list<class CEntity*> cameraEntities;		   // the main global list of all camera entities

private:

	float fov;

	void updateFrustum(D3DXMATRIX *worldProj);
	void updateProjection();

	float frustum[6][4];

	IDirect3DDevice8 *device;

	D3DVIEWPORT8 viewport;
	D3DXMATRIX projMatrix;
	
	list<CEntity*>::iterator cameraEntitiesListPosition; // keeps track of where in the camera entity list this entity resides
};

#endif