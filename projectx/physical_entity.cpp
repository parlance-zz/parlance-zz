#include "physical_entity.h"

#include "xengine.h"
#include "bsp_resource.h"
#include "entity_types.h"

list<CEntity*> CPhysicalEntity::physicalEntities;

#ifdef PHYSICAL_ENTITY_DEBUG_SPHERES

ID3DXMesh *CPhysicalEntity::sphereMesh = NULL;

#endif

int CPhysicalEntity::getEntityType()
{
	return ET_PHYSICAL_ENTITY;
}

int CPhysicalEntity::isEntityA(int baseType)
{
	if (baseType == ET_PHYSICAL_ENTITY)
	{
		return true;
	}
	else
	{
		return  __super::isEntityA(baseType);
	}
}

int CPhysicalEntity::getLastFrameRendered()
{
	return lastFrameRendered;
}

bool CPhysicalEntity::isHidden()
{
	return _isHidden;
}

void CPhysicalEntity::setHidden(bool isHidden)
{
	_isHidden = isHidden;
}

void CPhysicalEntity::create(bool renderEnable, bool collisionEnable)
{

#ifdef PHYSICAL_ENTITY_DEBUG_SPHERES

	if (!sphereMesh)
	{
		D3DXCreateSphere(XEngine->getD3DDevice(), 1.0f, 24, 24, &sphereMesh, NULL);
	}

#endif

	// transform stuff

	position.x = 0.0f;
	position.y = 0.0f;
	position.z = 0.0f;

	rotation.x = 0.0f;
	rotation.y = 0.0f;
	rotation.z = 0.0f;

	direction.x = 0.0f;
	direction.y = 0.0f;
	direction.z = 1.0;

	// culling / rendering stuff

	this->renderEnable  = renderEnable;

	boundingSphereRadius = 0.0f;

	currentCluster = -1;

	lastFrameRendered = 0;
	_isHidden = false;

	// physical stuff

	this->collisionEnable = collisionEnable;

	velocity.x = 0.0f;
	velocity.y = 0.0f;
	velocity.z = 0.0f;

	physicalUpdate();

	// and the rest of stuff
	
	physicalEntities.push_front(this);
	physicalEntitiesListPosition = physicalEntities.begin();

	__super::create();
}

void CPhysicalEntity::update()
{
}

void CPhysicalEntity::render()
{
	lastFrameRendered = XEngine->getRenderFrame();
}

void CPhysicalEntity::physicalUpdate()
{
	// todo: perform collision here, if any is nessecary

	position += velocity;

	// todo: update transforms here

	if (CBSPResource::world)
	{
		if (renderEnable || collisionEnable)
		{
			currentCluster = CBSPResource::world->registerEntity(this);
		}
		else
		{
			currentCluster = -1;
		}
	}
	else
	{
		currentCluster = -1;
	}
}

void CPhysicalEntity::getPosition(D3DXVECTOR3 *out)
{
	(*out) = position;
}

void CPhysicalEntity::setPosition(D3DXVECTOR3 *position)
{
	this->position = (*position);
}

void CPhysicalEntity::getVelocity(D3DXVECTOR3 *out)
{
	(*out) = velocity;
}

void CPhysicalEntity::setVelocity(D3DXVECTOR3 *velocity)
{
	this->velocity = (*velocity);
}

void CPhysicalEntity::getRotation(D3DXVECTOR3 *out)
{
	(*out) = rotation;
}

void CPhysicalEntity::setRotation(D3DXVECTOR3 *rotation)
{
	this->rotation = (*rotation);

	updateDirection();
}

void CPhysicalEntity::getDirection(D3DXVECTOR3 *out)
{
	(*out) = direction;
}

void CPhysicalEntity::setDirection(D3DXVECTOR3 *direction)
{
	this->direction = (*direction);

	updateRotation();
}


void CPhysicalEntity::updateDirection()
{
	D3DXVECTOR3 in = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	getWorldDirection(&in, &direction);
}

void CPhysicalEntity::updateRotation()
{
	// todo: this will be significantly harder, probably involving atan2
}

void CPhysicalEntity::getWorldDirection(D3DXVECTOR3 *localVector, D3DXVECTOR3 *out)
{
	D3DXMATRIX rotationXMatrix, rotationYMatrix, rotationZMatrix;

	D3DXMatrixRotationX(&rotationXMatrix, D3DXToRadian(rotation.x));
	D3DXMatrixRotationY(&rotationYMatrix, D3DXToRadian(rotation.y));
	D3DXMatrixRotationZ(&rotationZMatrix, D3DXToRadian(rotation.z));

	D3DXMATRIX rotMatrix;

	D3DXMatrixMultiply(&rotMatrix, &rotationZMatrix, &rotationYMatrix);
	D3DXMatrixMultiply(&rotMatrix, &rotMatrix, &rotationXMatrix);

	D3DXVECTOR4 out4;

	D3DXVec3Transform(&out4, localVector, &rotMatrix);

	out->x = out4.x;
	out->y = out4.y;
	out->z = out4.z;
}

int	CPhysicalEntity::getCurrentCluster()
{
	return currentCluster;
}

float CPhysicalEntity::getBoundingSphereRadius()
{
	return boundingSphereRadius;
}

void CPhysicalEntity::setBoundingSphereRadius(float boundingSphereRadius)
{
	this->boundingSphereRadius = boundingSphereRadius;
}

bool CPhysicalEntity::getRenderEnable()
{
	return renderEnable;
}

void CPhysicalEntity::setRenderEnable(bool renderEnable)
{
	this->renderEnable = renderEnable;
}

bool CPhysicalEntity::getCollisionEnable()
{
	return collisionEnable;
}

void CPhysicalEntity::setCollisionEnable(bool collisionEnable)
{
	this->collisionEnable = collisionEnable;
}

//void CPhysicalEntity::collide(XENGINE_COLLISION_INFO *collisionInfo, XENGINE_COLLISION_TRACE_INFO *traceInfo)
//{
//}

void CPhysicalEntity::destroy()
{
	//todo: release transform/collision/physics info

	physicalEntities.erase(physicalEntitiesListPosition);
	
	__super::destroy();
}