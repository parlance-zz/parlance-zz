#include "sound3d_entity.h"
#include "entity_types.h"
#include "sound_resource.h"
#include "xengine.h"
#include "bsp_resource.h"
#include "math_help.h"

list<CEntity*> CSound3DEntity::sound3DEntities;

int CSound3DEntity::getEntityType()
{
	return ET_SOUND3D_ENTITY;
}

int CSound3DEntity::isEntityA(int baseType)
{
	if (baseType == ET_SOUND3D_ENTITY)
	{
		return true;
	}
	else
	{
		return  __super::isEntityA(baseType);
	}
}

void CSound3DEntity::setPaused(bool isPaused)
{
	if (isPaused)
	{
		soundBuffer->Pause(DSSTREAMPAUSE_PAUSE);
	}
	else
	{
		soundBuffer->Pause(DSSTREAMPAUSE_RESUME);
	}
}

void CSound3DEntity::setVolume(float volume)
{
	if (volume < 0.0f)
	{
		volume = 0.0f;
	}
	else if (volume > 1.0f)
	{
		volume = 1.0f;
	}

	this->volume = -6400 + LONG(volume * 6400.0f); // yay for dsound being silly
}

void CSound3DEntity::setMaxDistance(float maxDistance)
{
	setBoundingSphereRadius(maxDistance);

	soundBuffer->SetMaxDistance(maxDistance, DS3D_DEFERRED);
}

float CSound3DEntity::getMaxDistance()
{
	return getBoundingSphereRadius();
}

void CSound3DEntity::activate()
{
	activated = true;

	soundBuffer->SetVolume(volume);
}

void CSound3DEntity::deactivate()
{
	activated = false;

	soundBuffer->SetVolume(DSBVOLUME_MIN);
}

bool CSound3DEntity::isActivated()
{
	return activated;
}

void CSound3DEntity::create(CSoundResource *soundResource, float maxDistance)
{
	this->soundResource = soundResource;

	soundResource->addRef();

	WAVEFORMATEXTENSIBLE *format = soundResource->getFormat();

    DSBUFFERDESC bufferDesc;

    bufferDesc.dwSize = sizeof(DSBUFFERDESC);

    bufferDesc.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_LOCDEFER;// | DSBCAPS_MUTE3DATMAXDISTANCE;
    bufferDesc.dwBufferBytes = 0;
	bufferDesc.lpwfxFormat = (WAVEFORMATEX*)format;

	bufferDesc.lpMixBins   = NULL;
	bufferDesc.dwInputMixBin = 0;

	XEngine->getDirectSound()->CreateSoundBuffer(&bufferDesc, &soundBuffer, NULL); // todo: liable to fail

	int bufferSize = soundResource->getBufferSize();

	DWORD loopStart;
	DWORD loopLength;

	int loopStartOffset;
	int loopSize;

	if (soundResource->getLoopRegion(&loopStart, &loopLength))
	{
		if (format->Format.wFormatTag == WAVE_FORMAT_XBOX_ADPCM ||
		   (format->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE && format->SubFormat == KSDATAFORMAT_SUBTYPE_XBOX_ADPCM))
		{
			loopStartOffset = loopStart  / 64 * format->Format.nBlockAlign;
			loopSize        = loopLength / 64 * format->Format.nBlockAlign;
		}
		else
		{
			int bytesPerSample = format->Format.nChannels * format->Format.wBitsPerSample / 8;

			loopStartOffset = loopStart  * bytesPerSample;
			loopSize        = loopLength * bytesPerSample;
		}
	}
	else
	{
		loopStartOffset = 0;
		loopSize = bufferSize;
	}

    soundBuffer->SetBufferData(soundResource->getBuffer(), bufferSize);

    soundBuffer->SetLoopRegion(loopStartOffset, loopSize);
    soundBuffer->SetCurrentPosition(0);

	isSoundAcquired = false;

	volume = 0;

	activated = true;

	terminateOnEnd = false;

	setMaxDistance(maxDistance);

	sound3DEntities.push_front(this);
	sound3DEntitiesListPosition = sound3DEntities.begin();

	__super::create(false, false);

	update();
}

void CSound3DEntity::update()
{
	DWORD soundStatus;

	soundBuffer->GetStatus(&soundStatus);

	if (soundStatus == DSBSTATUS_PAUSED)
	{
		soundResource->drop();

		isSoundAcquired = false;

		if (terminateOnEnd)
		{
			destroy();

			return;
		}
	}

	if (activated)
	{
		D3DXVECTOR3 position;
		D3DXVECTOR3 velocity;

		getPosition(&position);
		getVelocity(&velocity);

		soundBuffer->SetPosition(position.x, position.y, position.z, DS3D_DEFERRED);
		soundBuffer->SetVelocity(velocity.x, velocity.y, velocity.z, DS3D_DEFERRED);
	}
}

void CSound3DEntity::play(bool looping)
{
	if (!isSoundAcquired)
	{
		if (!(isSoundAcquired = soundResource->acquire(true)))
		{
			return;
		}
	}

	if (looping)
	{
		soundBuffer->Play(0, 0, DSBPLAY_FROMSTART | DSBPLAY_LOOPING);
	}
	else
	{
		soundBuffer->Play(0, 0, DSBPLAY_FROMSTART);
	}
}

void CSound3DEntity::playOnce()
{
	if (!isSoundAcquired)
	{
		if (!(isSoundAcquired = soundResource->acquire(true)))
		{
			return;
		}
	}

	soundBuffer->Play(0, 0, DSBPLAY_FROMSTART);
	
	terminateOnEnd = true;
}

void CSound3DEntity::destroy()
{
	soundResource->release();
	soundBuffer->Release();

	sound3DEntities.erase(sound3DEntitiesListPosition);
	
	__super::destroy();
}