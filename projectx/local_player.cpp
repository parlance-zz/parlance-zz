#include "local_player.h"

#include "math_help.h"
#include "xengine.h"
#include "entity_types.h"
#include "camera_entity.h"
#include "mesh_entity.h"
#include "player_resource.h"
#include "md3_resource.h"
#include "weapon.h"

list<CEntity*> CLocalPlayer::localPlayerEntities;

int CLocalPlayer::getEntityType()
{
	return ET_LOCALPLAYER_ENTITY;
}

int CLocalPlayer::isEntityA(int baseType)
{
	if (baseType == ET_LOCALPLAYER_ENTITY)
	{
		return true;
	}
	else
	{
		return  __super::isEntityA(baseType);
	}
}

CCameraEntity *CLocalPlayer::getCamera()
{
	return camera;
}

void CLocalPlayer::updateLocalPlayerViewports()
{
	int numLocalPlayers = (int)localPlayerEntities.size();
	int playerIndex = numLocalPlayers;

	for (list<CEntity*>::iterator iLocalPlayer = localPlayerEntities.begin(); iLocalPlayer != localPlayerEntities.end(); iLocalPlayer++)
	{
		CLocalPlayer *localPlayer = (CLocalPlayer*)(*iLocalPlayer);
		CCameraEntity *playerCamera = localPlayer->getCamera();

		switch(numLocalPlayers)
		{
		case 1:

			playerCamera->setViewport(0, 0, XENGINE_GRAPHICS_WIDTH, XENGINE_GRAPHICS_HEIGHT);
			break;

		case 2:

			switch(playerIndex)
			{
			case 1:

				playerCamera->setViewport(0, 0, XENGINE_GRAPHICS_WIDTH, XENGINE_GRAPHICS_HEIGHT / 2);
				break;

			case 2:

				playerCamera->setViewport(0, XENGINE_GRAPHICS_HEIGHT / 2, XENGINE_GRAPHICS_WIDTH, XENGINE_GRAPHICS_HEIGHT / 2);
				break;
			}

			break;

		case 3:

			switch(playerIndex)
			{
			case 1:

				playerCamera->setViewport(0, 0, XENGINE_GRAPHICS_WIDTH, XENGINE_GRAPHICS_HEIGHT / 2);
				break;

			case 2:

				playerCamera->setViewport(0, XENGINE_GRAPHICS_HEIGHT / 2, XENGINE_GRAPHICS_WIDTH / 2, XENGINE_GRAPHICS_HEIGHT / 2);
				break;

			case 3:

				playerCamera->setViewport(XENGINE_GRAPHICS_WIDTH / 2, XENGINE_GRAPHICS_HEIGHT / 2, XENGINE_GRAPHICS_WIDTH / 2, XENGINE_GRAPHICS_HEIGHT / 2);
				break;
			}

			break;

		case 4:

			switch(playerIndex)
			{
			case 1:

				playerCamera->setViewport(0, 0, XENGINE_GRAPHICS_WIDTH / 2, XENGINE_GRAPHICS_HEIGHT / 2);
				break;

			case 2:

				playerCamera->setViewport(XENGINE_GRAPHICS_WIDTH / 2, 0, XENGINE_GRAPHICS_WIDTH / 2, XENGINE_GRAPHICS_HEIGHT / 2);
				break;

			case 3:

				playerCamera->setViewport(0, XENGINE_GRAPHICS_HEIGHT / 2, XENGINE_GRAPHICS_WIDTH / 2, XENGINE_GRAPHICS_HEIGHT / 2);
				break;

			case 4:

				playerCamera->setViewport(XENGINE_GRAPHICS_WIDTH / 2, XENGINE_GRAPHICS_HEIGHT / 2, XENGINE_GRAPHICS_WIDTH / 2, XENGINE_GRAPHICS_HEIGHT / 2);
				break;
			}

			break;
		}
		
		playerIndex--;
	}
}

int CLocalPlayer::getControllerNum()
{
	return controllerNum;
}

bool CLocalPlayer::isControllerInUse(int controllerNum)
{
	for (list<CEntity*>::iterator iLocalPlayer = localPlayerEntities.begin(); iLocalPlayer != localPlayerEntities.end(); iLocalPlayer++)
	{
		CLocalPlayer *localPlayer = (CLocalPlayer*)(*iLocalPlayer);

		if (localPlayer->getControllerNum() == controllerNum)
		{
			return true;
		}
	}

	return false;
}

void CLocalPlayer::create(CPlayerResource *playerResource, int controllerNum)
{
	this->controllerNum = controllerNum;

	if (localPlayerEntities.size() >= GAME_LOCALPLAYER_MAX_LOCALPLAYERS)
	{
		// error

		return;
	}

	localPlayerEntities.push_front(this);
	localPlayerEntitiesListPosition = localPlayerEntities.begin();

	if (controllerNum == 0)
	{
		camera = new CCameraEntity;
		camera->create();
	
		updateLocalPlayerViewports();
	}
	else
	{
		camera = NULL;
	}

	__super::create(playerResource);

	lower->setAnimation(playerResource->getAnimationInfo(PA_LEGS_IDLE));
	upper->setAnimation(playerResource->getAnimationInfo(PA_TORSO_STAND));
}

void CLocalPlayer::update()
{
	D3DXVECTOR2 leftJoy;
	D3DXVECTOR2 rightJoy;
	
	XEngine->getAxisState(controllerNum, XENGINE_INPUT_AXIS_LEFT_JOYSTICK, &leftJoy);
	XEngine->getAxisState(controllerNum, XENGINE_INPUT_AXIS_RIGHT_JOYSTICK, &rightJoy);

	D3DXVECTOR3 rotation;

	getRotation(&rotation);

	if (D3DXVec2Length(&rightJoy) > 0.1f)
	{	
		rotation.y = D3DXToDegree(-atan2(rightJoy.y, rightJoy.x)) + 135.0f;
	}

	setRotation(&rotation);

	D3DXVECTOR3 direction;
	getDirection(&direction);

	D3DXVECTOR2 movement;

	float len = D3DXVec2Length(&leftJoy);
	float ang = atan2(leftJoy.x, leftJoy.y);

	leftJoy.x = sin(ang + D3DX_PI / 4.0f) * len;
	leftJoy.y = cos(ang + D3DX_PI / 4.0f) * len;
	if (len < 1.0f) len = 1.0f;

	leftJoy /= len;

	movement = leftJoy * 3.0f * XEngine->getTween();
	float movementSpeed = D3DXVec2Length(&movement);

	D3DXVECTOR2 movementDirection = D3DXVECTOR2(movement.x, movement.y);
	D3DXVec2Normalize(&movementDirection, &movementDirection);

	D3DXVECTOR2 playerDirection = D3DXVECTOR2(direction.x, direction.y);
	D3DXVec2Normalize(&playerDirection, &playerDirection);

	if (movementSpeed < 0.1f)
	{
		D3DXVECTOR3 legsRotation;
		lower->getRotation(&legsRotation);

		if (movementState == PLAYER_MOVEMENT_BACKPEDALING)
		{
			//legsRotation.y -= 180.0f;
			//lower->setRotation(&legsRotation);
		}
		
		float deltaAngle = fmod(rotation.y - legsRotation.y + 180.0f, 360.0f) - 180.0f;

		if (movementState == PLAYER_MOVEMENT_TURNING && (deltaAngle < -4.0f || deltaAngle > 4.0f))
		{
			legsRotation.y += deltaAngle / fabs(deltaAngle) * 2.0f;
			lower->setRotation(&legsRotation);
		}
		else if (deltaAngle < -90.0f || deltaAngle > 90.0f)
		{
			if (movementState != PLAYER_MOVEMENT_TURNING)
			{
				lower->setAnimation(playerResource->getAnimationInfo(PA_LEGS_TURN));
				lower->setAnimSpeed(1.25f);

				movementState = PLAYER_MOVEMENT_TURNING;
			}
		} 
		else if (movementState != PLAYER_MOVEMENT_IDLE)
		{
			lower->setAnimation(playerResource->getAnimationInfo(PA_LEGS_IDLE));
			lower->setAnimSpeed(1.0f);

			movementState = PLAYER_MOVEMENT_IDLE;
		}

		movement *= 0.0f;
	}
	else if (D3DXVec2Dot(&movementDirection, &playerDirection) < -0.1f)
	{
		//player is backpedaling
		if (movementState != PLAYER_MOVEMENT_BACKPEDALING)
		{
			movementState = PLAYER_MOVEMENT_BACKPEDALING;
			lower->setAnimation(playerResource->getAnimationInfo(PA_LEGS_BACKPEDAL));
		}

		lower->setAnimSpeed(movementSpeed);
	}
	else if (movementSpeed < 0.75f)
	{
		if (movementState != PLAYER_MOVEMENT_WALKING)
		{
			movementState = PLAYER_MOVEMENT_WALKING;
			lower->setAnimation(playerResource->getAnimationInfo(PA_LEGS_WALK));
		}

		lower->setAnimSpeed(movementSpeed * 1.25f);
	}
	else
	{
		if (movementState != PLAYER_MOVEMENT_RUNNING)
		{
			movementState = PLAYER_MOVEMENT_RUNNING;
			lower->setAnimation(playerResource->getAnimationInfo(PA_LEGS_RUN));
		}

		lower->setAnimSpeed(movementSpeed / 1.2f);
	}
	
	if (movementSpeed >= 0.1f)
	{
		D3DXVECTOR3 legsRotation = D3DXVECTOR3(0.0f, D3DXToDegree(atan2(movementDirection.x, movementDirection.y)), 0.0f);
	
		if (movementState == PLAYER_MOVEMENT_BACKPEDALING)
		{
			legsRotation.y -= 180.0f;
		}

		lower->setRotation(&legsRotation);
	}

	if (XEngine->getButtonPressed(controllerNum, XENGINE_INPUT_BUTTON_RTRIGGER))
	{
		upper->setAnimation(playerResource->getAnimationInfo(PA_TORSO_ATTACK2));
	}

	//upMovement.y = (XEngine->getButtonState(controllerNum, XENGINE_INPUT_BUTTON_RTRIGGER) / 255.0f - 
	//			    XEngine->getButtonState(controllerNum, XENGINE_INPUT_BUTTON_LTRIGGER) / 255.0f) * 4.0f  * XEngine->getTween();

	D3DXVECTOR3 velocity;

	velocity = D3DXVECTOR3(movement.x, 0.0f, movement.y);

	setVelocity(&velocity);

	__super::update();
}

void CLocalPlayer::destroy()
{
	localPlayerEntities.erase(localPlayerEntitiesListPosition);
	
	__super::destroy();
}