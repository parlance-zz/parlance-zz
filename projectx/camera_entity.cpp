#include <xtl.h>

#include "camera_entity.h"

#include "xengine.h"
#include "entity.h"
#include "entity_types.h"
#include "math_help.h"

#define VERTEX_IN		 1
#define VERTEX_OUT		 2
#define VERTEX_INTERSECT 3

list<CEntity*> CCameraEntity::cameraEntities;

int CCameraEntity::getEntityType()
{
	return ET_CAMERA_ENTITY;
}

int CCameraEntity::isEntityA(int baseType)
{
	if (baseType == ET_CAMERA_ENTITY)
	{
		return true;
	}
	else
	{
		return  __super::isEntityA(baseType);
	}
}

void CCameraEntity::setFOV(float fov)
{
	this->fov = fov;
}

float CCameraEntity::getFOV()
{
	return fov;
}

void CCameraEntity::create()
{
	device = XEngine->getD3DDevice();

	viewport.X = 0;
	viewport.Y = 0;

	viewport.Width  = XENGINE_GRAPHICS_WIDTH;
	viewport.Height = XENGINE_GRAPHICS_HEIGHT;

	viewport.MinZ = 0.0f; // these may need to be adjusted, I'll leave them as is for now
	viewport.MaxZ = 1.0f;

	fov = XENGINE_DEFAULT_CAMERA_FOV;

	updateProjection();

	cameraEntities.push_front(this);
	cameraEntitiesListPosition = cameraEntities.begin();

	__super::create(true, false);
}

void CCameraEntity::updateProjection()
{
	float aspectRatio = float(viewport.Width) / float(viewport.Height);

	setProjection(fov, aspectRatio, XENGINE_DEFAULT_CAMERA_NEARCLIP, XENGINE_DEFAULT_CAMERA_FARCLIP);
}


void CCameraEntity::setProjection(float fov, float aspectRatio, float nearClip, float farClip)
{
	D3DXMatrixPerspectiveFovLH(&projMatrix, fov , aspectRatio, nearClip, farClip);
}


void CCameraEntity::setViewport(int x, int y, int width, int height)
{
	viewport.X = x;
	viewport.Y = y;

	viewport.Width  = width;
	viewport.Height = height;

	updateProjection();
}

void CCameraEntity::activate()
{
	D3DXMATRIX transMatrix;

	D3DXVECTOR3 position;

	getPosition(&position);

	D3DXMatrixTranslation(&transMatrix, -position.x, -position.y, -position.z);

	/*
	D3DXMATRIX rotationXMatrix, rotationYMatrix, rotationZMatrix;

	D3DXVECTOR3 rotation;

	getRotation(&rotation);

	D3DXMatrixRotationX(&rotationXMatrix, D3DXToRadian(-rotation.x));
	D3DXMatrixRotationY(&rotationYMatrix, D3DXToRadian(-rotation.y));
	D3DXMatrixRotationZ(&rotationZMatrix, D3DXToRadian(-rotation.z));
	*/

	D3DXMATRIX rotMatrix;

	D3DXVECTOR3 direction; getDirection(&direction);
	D3DXMatrixLookAtLH(&rotMatrix, &D3DXVECTOR3(0.0f, 0.0f, 0.0f), &direction, &D3DXVECTOR3(0.0f, 1.0f, 0.0f));

	/*
	D3DXMatrixMultiply(&rotMatrix, &rotationZMatrix, &rotationYMatrix);
	D3DXMatrixMultiply(&rotMatrix, &rotMatrix, &rotationXMatrix);
	*/

	D3DXMATRIX worldMatrix;

	D3DXMatrixMultiply(&worldMatrix, &transMatrix, &rotMatrix);

	// todo: setup vertex constants too

	device->SetTransform(D3DTS_VIEW, &worldMatrix);
	device->SetTransform(D3DTS_PROJECTION, &projMatrix);

	device->SetViewport(&viewport);

	D3DXMATRIX worldProj;

	D3DXMatrixMultiply(&worldProj, &worldMatrix, &projMatrix);

	updateFrustum(&worldProj);
}

void CCameraEntity::updateFrustum(D3DXMATRIX *worldProj)
{
	float *clip = (float*)worldProj;

    frustum[CAMERA_FS_RIGHT][0] = clip[ 3] - clip[ 0];
    frustum[CAMERA_FS_RIGHT][1] = clip[ 7] - clip[ 4];
    frustum[CAMERA_FS_RIGHT][2] = clip[11] - clip[ 8];
    frustum[CAMERA_FS_RIGHT][3] = clip[15] - clip[12];

	float invLength = 1.0f / sqrtf(frustum[CAMERA_FS_RIGHT][0] * frustum[CAMERA_FS_RIGHT][0] +
						           frustum[CAMERA_FS_RIGHT][1] * frustum[CAMERA_FS_RIGHT][1] +
						           frustum[CAMERA_FS_RIGHT][2] * frustum[CAMERA_FS_RIGHT][2]);

    frustum[CAMERA_FS_RIGHT][0] *= invLength;
    frustum[CAMERA_FS_RIGHT][1] *= invLength;
    frustum[CAMERA_FS_RIGHT][2] *= invLength;
    frustum[CAMERA_FS_RIGHT][3] *= invLength;

    frustum[CAMERA_FS_LEFT][0] = clip[ 3] + clip[ 0];
    frustum[CAMERA_FS_LEFT][1] = clip[ 7] + clip[ 4];
    frustum[CAMERA_FS_LEFT][2] = clip[11] + clip[ 8];
    frustum[CAMERA_FS_LEFT][3] = clip[15] + clip[12];

	invLength = 1.0f / sqrtf(frustum[CAMERA_FS_LEFT][0] * frustum[CAMERA_FS_LEFT][0] +
						     frustum[CAMERA_FS_LEFT][1] * frustum[CAMERA_FS_LEFT][1] +
						     frustum[CAMERA_FS_LEFT][2] * frustum[CAMERA_FS_LEFT][2]);

    frustum[CAMERA_FS_LEFT][0] *= invLength;
    frustum[CAMERA_FS_LEFT][1] *= invLength;
    frustum[CAMERA_FS_LEFT][2] *= invLength;
    frustum[CAMERA_FS_LEFT][3] *= invLength;

    frustum[CAMERA_FS_BOTTOM][0] = clip[ 3] + clip[ 1];
    frustum[CAMERA_FS_BOTTOM][1] = clip[ 7] + clip[ 5];
    frustum[CAMERA_FS_BOTTOM][2] = clip[11] + clip[ 9];
    frustum[CAMERA_FS_BOTTOM][3] = clip[15] + clip[13];

	invLength = 1.0f / sqrtf(frustum[CAMERA_FS_BOTTOM][0] * frustum[CAMERA_FS_BOTTOM][0] +
						     frustum[CAMERA_FS_BOTTOM][1] * frustum[CAMERA_FS_BOTTOM][1] +
						     frustum[CAMERA_FS_BOTTOM][2] * frustum[CAMERA_FS_BOTTOM][2]);

    frustum[CAMERA_FS_BOTTOM][0] *= invLength;
    frustum[CAMERA_FS_BOTTOM][1] *= invLength;
    frustum[CAMERA_FS_BOTTOM][2] *= invLength;
    frustum[CAMERA_FS_BOTTOM][3] *= invLength;

    frustum[CAMERA_FS_TOP][0] = clip[ 3] - clip[ 1];
    frustum[CAMERA_FS_TOP][1] = clip[ 7] - clip[ 5];
    frustum[CAMERA_FS_TOP][2] = clip[11] - clip[ 9];
    frustum[CAMERA_FS_TOP][3] = clip[15] - clip[13];

	invLength = 1.0f / sqrtf(frustum[CAMERA_FS_TOP][0] * frustum[CAMERA_FS_TOP][0] +
						     frustum[CAMERA_FS_TOP][1] * frustum[CAMERA_FS_TOP][1] +
						     frustum[CAMERA_FS_TOP][2] * frustum[CAMERA_FS_TOP][2]);

    frustum[CAMERA_FS_TOP][0] *= invLength;
    frustum[CAMERA_FS_TOP][1] *= invLength;
    frustum[CAMERA_FS_TOP][2] *= invLength;
    frustum[CAMERA_FS_TOP][3] *= invLength;

    frustum[CAMERA_FS_BACK][0] = clip[ 3] - clip[ 2];
    frustum[CAMERA_FS_BACK][1] = clip[ 7] - clip[ 6];
    frustum[CAMERA_FS_BACK][2] = clip[11] - clip[10];
    frustum[CAMERA_FS_BACK][3] = clip[15] - clip[14];

	invLength = 1.0f / sqrtf(frustum[CAMERA_FS_BACK][0] * frustum[CAMERA_FS_BACK][0] +
						     frustum[CAMERA_FS_BACK][1] * frustum[CAMERA_FS_BACK][1] +
						     frustum[CAMERA_FS_BACK][2] * frustum[CAMERA_FS_BACK][2]);

    frustum[CAMERA_FS_BACK][0] *= invLength;
    frustum[CAMERA_FS_BACK][1] *= invLength;
    frustum[CAMERA_FS_BACK][2] *= invLength;
    frustum[CAMERA_FS_BACK][3] *= invLength;

    frustum[CAMERA_FS_FRONT][0] = clip[ 3] + clip[ 2];
    frustum[CAMERA_FS_FRONT][1] = clip[ 7] + clip[ 6];
    frustum[CAMERA_FS_FRONT][2] = clip[11] + clip[10];
    frustum[CAMERA_FS_FRONT][3] = clip[15] + clip[14];

	invLength = 1.0f / sqrtf(frustum[CAMERA_FS_FRONT][0] * frustum[CAMERA_FS_FRONT][0] +
						     frustum[CAMERA_FS_FRONT][1] * frustum[CAMERA_FS_FRONT][1] +
						     frustum[CAMERA_FS_FRONT][2] * frustum[CAMERA_FS_FRONT][2]);

    frustum[CAMERA_FS_FRONT][0] *= invLength;
    frustum[CAMERA_FS_FRONT][1] *= invLength;
    frustum[CAMERA_FS_FRONT][2] *= invLength;
    frustum[CAMERA_FS_FRONT][3] *= invLength;
}

int CCameraEntity::pointInFrustum(float x, float y, float z)
{
    for (int i = 0; i < 6; i++)
    {
        if (frustum[i][0] * x + frustum[i][1] * y + frustum[i][2] * z + frustum[i][3] <= 0.0f)
        {
            return false;
        }
    }

    return true;
}

// if the sphere is out of any plane, it is immediately out
// otherwise, it is in

int CCameraEntity::sphereInFrustum(float x, float y, float z, float radius)
{
    if (frustum[CAMERA_FS_FRONT][0] * x + frustum[CAMERA_FS_FRONT][1] * y + frustum[CAMERA_FS_FRONT][2] * z + frustum[CAMERA_FS_FRONT][3] <= -radius)
    {
        return false;
	}

    if (frustum[CAMERA_FS_LEFT][0] * x + frustum[CAMERA_FS_LEFT][1] * y + frustum[CAMERA_FS_LEFT][2] * z + frustum[CAMERA_FS_LEFT][3] <= -radius)
    {
        return false;
	}

    if (frustum[CAMERA_FS_RIGHT][0] * x + frustum[CAMERA_FS_RIGHT][1] * y + frustum[CAMERA_FS_RIGHT][2] * z + frustum[CAMERA_FS_RIGHT][3] <= -radius)
    {
        return false;
	}

    if (frustum[CAMERA_FS_BOTTOM][0] * x + frustum[CAMERA_FS_BOTTOM][1] * y + frustum[CAMERA_FS_BOTTOM][2] * z + frustum[CAMERA_FS_BOTTOM][3] <= -radius)
    {
        return false;
	}

    if (frustum[CAMERA_FS_TOP][0] * x + frustum[CAMERA_FS_TOP][1] * y + frustum[CAMERA_FS_TOP][2] * z + frustum[CAMERA_FS_TOP][3] <= -radius)
    {
        return false;
	}

    if (frustum[CAMERA_FS_BACK][0] * x + frustum[CAMERA_FS_BACK][1] * y + frustum[CAMERA_FS_BACK][2] * z + frustum[CAMERA_FS_BACK][3] <= -radius)
    {
        return false;
	}

    return true;
}

int CCameraEntity::boxInFrustum(float x, float y, float z, float x2, float y2, float z2)
{
	static BYTE mode = 0;

	for (int i = 0; i < 6; i++)
	{
		mode &= VERTEX_OUT;

		if (frustum[i][0] * x + frustum[i][1] * y + frustum[i][2] * z + frustum[i][3] >= 0.0f)
		{
			mode |= VERTEX_IN;
		}
		else
		{
			mode |= VERTEX_OUT;
		}

		if (mode == VERTEX_INTERSECT)
		{
			continue;
		}
		
		if (frustum[i][0] * x2 + frustum[i][1] * y + frustum[i][2] * z + frustum[i][3] >= 0.0f)
		{
			mode |= VERTEX_IN;
		}
		else
		{
			mode |= VERTEX_OUT;
		}

		if (mode == VERTEX_INTERSECT)
		{
			continue;
		}

		if (frustum[i][0] * x + frustum[i][1] * y2 + frustum[i][2] * z + frustum[i][3] >= 0.0f)
		{
			mode |= VERTEX_IN;
		}
		else
		{
			mode |= VERTEX_OUT;
		}

		if (mode == VERTEX_INTERSECT)
		{
			continue;
		}

		if (frustum[i][0] * x2 + frustum[i][1] * y2 + frustum[i][2] * z + frustum[i][3] >= 0.0f)
		{
			mode |= VERTEX_IN;
		}
		else
		{
			mode |= VERTEX_OUT;
		}

		if (mode == VERTEX_INTERSECT)
		{
			continue;
		}

		if (frustum[i][0] * x + frustum[i][1] * y + frustum[i][2] * z2 + frustum[i][3] >= 0.0f)
		{
			mode |= VERTEX_IN;
		}
		else
		{
			mode |= VERTEX_OUT;
		}

		if (mode == VERTEX_INTERSECT)
		{
			continue;
		}

		if (frustum[i][0] * x2 + frustum[i][1] * y + frustum[i][2] * z2 + frustum[i][3] >= 0.0f)
		{
			mode |= VERTEX_IN;
		}
		else
		{
			mode |= VERTEX_OUT;
		}

		if (mode == VERTEX_INTERSECT)
		{
			continue;
		}

		if (frustum[i][0] * x + frustum[i][1] * y2 + frustum[i][2] * z2 + frustum[i][3] >= 0.0f)
		{
			mode |= VERTEX_IN;
		}
		else
		{
			mode |= VERTEX_OUT;
		}

		if (mode == VERTEX_INTERSECT)
		{
			continue;
		}

		if (frustum[i][0] * x2 + frustum[i][1] * y2 + frustum[i][2] * z2 + frustum[i][3] >= 0.0f)
		{
			mode |= VERTEX_IN;
		}
		else
		{
			mode |= VERTEX_OUT;
		}

		if (mode == VERTEX_INTERSECT || mode == VERTEX_IN)
		{
			continue;
		}

		return CAMERA_FTR_COMPLETE_OUT;
	}

	if (mode == VERTEX_INTERSECT)
	{
		return CAMERA_FTR_INTERSECT;
	}
	else
	{
		return CAMERA_FTR_COMPLETE_IN;
	}
}

void CCameraEntity::destroy()
{
	// todo:

	cameraEntities.erase(cameraEntitiesListPosition);
	
	__super::destroy();
}