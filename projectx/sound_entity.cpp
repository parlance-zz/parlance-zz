#include "sound_entity.h"
#include "entity_types.h"
#include "sound_resource.h"
#include "xengine.h"

list<CEntity*> CSoundEntity::soundEntities;

int CSoundEntity::getEntityType()
{
	return ET_SOUND_ENTITY;
}

int CSoundEntity::isEntityA(int baseType)
{
	if (baseType == ET_SOUND_ENTITY)
	{
		return true;
	}
	else
	{
		return  __super::isEntityA(baseType);
	}
}

void CSoundEntity::create(CSoundResource *soundResource)
{
	WAVEFORMATEXTENSIBLE *format = soundResource->getFormat();

    DSBUFFERDESC bufferDesc;

    bufferDesc.dwSize = sizeof(DSBUFFERDESC);

    bufferDesc.dwFlags = DSBCAPS_LOCDEFER;
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

	soundEntities.push_front(this);
	soundEntitiesListPosition = soundEntities.begin();
	
	__super::create();
}

void CSoundEntity::update()
{
	// todo: perhaps we should self-destruct when we're finished playing
}

void CSoundEntity::play(bool looping)
{
	if (looping)
	{
		soundBuffer->Play(0, 0, DSBPLAY_FROMSTART | DSBPLAY_LOOPING);
	}
	else
	{
		soundBuffer->Play(0, 0, DSBPLAY_FROMSTART);
	}
}

void CSoundEntity::destroy()
{
	soundBuffer->Release();

	soundEntities.erase(soundEntitiesListPosition);
	
	__super::destroy();
}