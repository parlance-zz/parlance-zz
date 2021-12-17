#include <xtl.h>

#include "mesh_entity.h"
#include "md3_resource.h"
#include "math_help.h"
#include "string_help.h"
#include "xengine.h"
#include "entity_types.h"

list<CEntity*> CMeshEntity::meshEntities;

int CMeshEntity::getEntityType()
{
	return ET_MESH_ENTITY;
}

int CMeshEntity::isEntityA(int baseType)
{
	if (baseType == ET_MESH_ENTITY)
	{
		return true;
	}
	else
	{
		return  __super::isEntityA(baseType);
	}
}

void CMeshEntity::create(CMD3Resource *md3Resource)
{
	this->md3Resource = md3Resource;

	parent = NULL;
	boneIndex = 0;

	ZeroMemory(&currentAnimation, sizeof(MD3_ANIMATION_INFO));

	animTime  = 0.0f;
	animSpeed = 1.0f;

	__super::create(false, false);

	//update();

	numSurfaces = md3Resource->getNumSurfaces();
	surfaces = new XSURFACE_INSTANCE[numSurfaces];

	numChildren = md3Resource->getNumTags();
	children = new CMeshEntity*[numChildren];

	ZeroMemory(children, numChildren * sizeof(CMeshEntity*));

	meshEntities.push_front(this);
	meshEntitiesListPosition = meshEntities.begin();
}

void CMeshEntity::addChild(CMeshEntity *child, int boneIndex)
{
	if (children[boneIndex])
	{
		children[boneIndex]->release();
	}

	children[boneIndex] = child;
	children[boneIndex]->addRef();
}

void CMeshEntity::removeChild(int boneIndex)
{
	if (children[boneIndex])
	{
		children[boneIndex]->release();
		children[boneIndex] = NULL;
	}
}

void CMeshEntity::link(CMeshEntity *parent, string tagName)
{
	CMD3Resource *md3Resource = parent->getMD3Resource();

	int numTags = md3Resource->getNumTags();
	MD3_TAG *tags = md3Resource->getTags();

	for (int t = 0; t < numTags; t++)
	{
		if (tags[t].name == tagName)
		{
			parent->addRef();

			boneIndex = tags[t].index;

			this->parent = parent;
			parent->addChild(this, boneIndex);

			return;
		}
	}

	// if we get here, this ought to be an error
}

void CMeshEntity::unlink()
{
	if (parent)
	{
		parent->removeChild(boneIndex);

		parent->release();

		parent	  = NULL;
		boneIndex = 0;
	}
	else
	{
		// error
	}
}

CMD3Resource *CMeshEntity::getMD3Resource()
{
	return md3Resource;
}

void CMeshEntity::update()
{
	// todo: fix animations that have a non 0 and non full count of looping frames

	int numFrames	  = md3Resource->getNumFrames();
	MD3_FRAME *frames = md3Resource->getFrames();

	if (numFrames)
	{
		int numAnimFrames = currentAnimation.endFrame - currentAnimation.startFrame;

		if (numAnimFrames)
		{
			if (currentAnimation.numLoopingFrames)
			{
				animTime = fMod(animTime + XEngine->getTween() * (1.0f / 60.0f) * currentAnimation.framesPerSecond * animSpeed, float(numAnimFrames));
			}
			else
			{
				animTime += XEngine->getTween() * (1.0f / 60.0f) * currentAnimation.framesPerSecond * animSpeed;

				if (animTime >= numAnimFrames - 1)
				{
					animTime = float(numAnimFrames - 1);
				}
			}
		}
		else
		{
			animTime = 0.0f;
		}

		/*
		int currentFrame = (int(floorf(animTime)) + currentAnimation.startFrame) % numFrames;
		int nextFrame    = (int(ceilf(animTime)) + currentAnimation.startFrame)  % numFrames;
		
		float t = animTime - floorf(animTime);
		*/
	}
	else
	{
		/*
		if (!parent)
		{
			setBoundingSphereRadius(frames[0].boundingSphereRadius);
		}
		*/
	}

	__super::update();
}

void CMeshEntity::setAnimation(MD3_ANIMATION_INFO *animation)
{
	animTime  = 0.0f;
	animSpeed = 1.0f;

	currentAnimation.startFrame = animation->startFrame;
	currentAnimation.endFrame   = animation->endFrame;

	currentAnimation.numLoopingFrames = animation->numLoopingFrames;

	currentAnimation.framesPerSecond = animation->framesPerSecond;
}

void CMeshEntity::getAnimation(MD3_ANIMATION_INFO *animation)
{
	animation->startFrame = currentAnimation.startFrame;
	animation->endFrame   = currentAnimation.endFrame;

	animation->numLoopingFrames = currentAnimation.numLoopingFrames;

	animation->framesPerSecond = currentAnimation.framesPerSecond;
}

float CMeshEntity::getAnimTime()
{
	return animTime;
}

void CMeshEntity::setAnimTime(float animTime)
{
	this->animTime = animTime;
}

void CMeshEntity::setAnimSpeed(float animSpeed)
{
	this->animSpeed = animSpeed;
}

float CMeshEntity::getAnimSpeed()
{
	return animSpeed;
}

void CMeshEntity::render()
{
	D3DXMATRIX identity;
	D3DXMatrixIdentity(&identity);

	_render(&identity);
}

void CMeshEntity::render(D3DXMATRIX *parentTransform)
{
	_render(parentTransform);
}

void CMeshEntity::_render(D3DXMATRIX *parentTransform)
{
	int numFrames	  = md3Resource->getNumFrames();
	MD3_FRAME *frames = md3Resource->getFrames();

	int currentFrame;
	int nextFrame;
	
	float t; 

	if (numFrames > 1)
	{
		int numAnimFrames = currentAnimation.endFrame - currentAnimation.startFrame;

		if (numAnimFrames)
		{
			currentFrame = int(floorf(animTime)) % numAnimFrames;
			nextFrame    = (currentFrame + 1) % numAnimFrames + currentAnimation.startFrame;

			currentFrame += currentAnimation.startFrame;
		}
		else
		{
			currentFrame = 0;
			nextFrame	 = 0;
		}

		t = animTime - floorf(animTime);
	}

	D3DXMATRIX localTransform;

	D3DXVECTOR3 position;
	getPosition(&position);

	D3DXMATRIX localTranslation;
	D3DXMatrixTranslation(&localTranslation, position.x, position.y, position.z);

	D3DXVECTOR3 rotation;
	getRotation(&rotation);

	D3DXMATRIX rotationX, rotationY, rotationZ;
	D3DXMATRIX localRotation;

	D3DXMatrixRotationX(&rotationX, D3DXToRadian(rotation.x));
	D3DXMatrixRotationY(&rotationY, D3DXToRadian(rotation.y));
	D3DXMatrixRotationZ(&rotationZ, D3DXToRadian(rotation.z));

	D3DXMatrixMultiply(&localRotation, &rotationX, &rotationY);
	D3DXMatrixMultiply(&localRotation, &localRotation, &rotationZ);

	/*
	D3DXMatrixRotationX(&rotationX, D3DXToRadian(rotation.x));
	D3DXMatrixRotationY(&rotationY, D3DXToRadian(rotation.y));
	D3DXMatrixRotationZ(&localRotation, D3DXToRadian(rotation.z));

	D3DXMatrixMultiply(&localRotation, &localRotation, &rotationY);
	D3DXMatrixMultiply(&localRotation, &localRotation, &rotationX);
	*/

	D3DXMatrixMultiply(&localTransform, &localRotation, &localTranslation);

	D3DXMATRIX transform;

	D3DXMatrixMultiply(&transform, &localTransform, parentTransform);

	for (int c = 0; c < numChildren; c++)
	{
		if (children[c])
		{
			if (!children[c]->isHidden())
			{
				if (numFrames > 1)
				{
					D3DXVECTOR3 bonePosition;

					bonePosition = frames[currentFrame].bones[c].position * (1.0f - t) + 
								frames[nextFrame].bones[c].position * t;

					D3DXMATRIX boneTranslation;

					D3DXMatrixTranslation(&boneTranslation, bonePosition.x, bonePosition.y, bonePosition.z);

					D3DXQUATERNION boneQuaternion;

					D3DXQuaternionSlerp(&boneQuaternion, &frames[currentFrame].bones[c].rotation,
											    		&frames[nextFrame].bones[c].rotation, t);

					D3DXMATRIX boneRotation;
					D3DXMatrixRotationQuaternion(&boneRotation, &boneQuaternion);

					D3DXMATRIX boneTransform;
					D3DXMatrixMultiply(&boneTransform, &boneRotation, &boneTranslation);

					D3DXMatrixMultiply(&boneTransform, &boneTransform, &transform);

					children[c]->render(&boneTransform);
				}
				else
				{
					D3DXMATRIX boneTranslation;

					D3DXMatrixTranslation(&boneTranslation, frames[0].bones[c].position.x,
															frames[0].bones[c].position.y,
															frames[0].bones[c].position.z);

					D3DXMATRIX boneRotation;
					D3DXQuaternionRotationMatrix(&frames[0].bones[c].rotation, &boneRotation);

					D3DXMATRIX boneTransform;
					D3DXMatrixMultiply(&boneTransform, &boneTranslation, &boneRotation);

					D3DXMatrixMultiply(&boneTransform, &boneTransform, &transform);

					children[c]->render(&boneTransform);
				}
			}
		}
	}

	for (int s = 0; s < numSurfaces; s++)
	{
		surfaces[s].numLights = 0;
		surfaces[s].hasColor  = false;

		// todo: setup instance transform

		surfaces[s].hasTransform = true;

		memcpy(&surfaces[s].world, &transform, sizeof(D3DXMATRIX));

		if (numFrames > 1)
		{
			surfaces[s].surface = &frames[currentFrame].surfaces[s];
			surfaces[s].target  = frames[nextFrame].surfaces[s].vertexBuffer;

			surfaces[s].time = t;
		}
		else
		{
			surfaces[s].surface = &frames[0].surfaces[s];
		}

		XEngine->queueSurface(&surfaces[s]);
	}

	__super::render();
}

void CMeshEntity::destroy()
{
	// todo:

	meshEntities.erase(meshEntitiesListPosition);
	
	__super::destroy();
}