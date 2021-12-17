#include "player_resource.h"

#include "resource_types.h"
#include "md3_resource.h"
#include "string_help.h"
#include "xengine.h"
#include "footstep_resource.h"

vector<CResource*> CPlayerResource::playerResources;

int CPlayerResource::getResourceType()
{
	return RT_PLAYER_RESOURCE;
}

CPlayerResource *CPlayerResource::getPlayer(string playerPath)
{
	for (int i = 0; i < (int)CPlayerResource::playerResources.size(); i++)
	{
		CPlayerResource *playerResource = (CPlayerResource*)CPlayerResource::playerResources[i];

		if (playerResource->getResourceName() == playerPath)
		{
			playerResource->addRef();

			return playerResource;
		}
	}

	CPlayerResource *playerResource = new CPlayerResource;

	playerResource->create(playerPath);

	return playerResource;
}

MD3_ANIMATION_INFO *CPlayerResource::getAnimationInfo(int animIndex)
{
	return &animations[animIndex];
}

CMD3Resource *CPlayerResource::getLower()
{
	return lower;
}

CMD3Resource *CPlayerResource::getUpper()
{
	return upper;
}

CMD3Resource *CPlayerResource::getHead()
{
	return head;
}

int CPlayerResource::getSex()
{
	return sex;
}

void CPlayerResource::getHeadOffset(D3DXVECTOR3 *headOffset)
{
	(*headOffset).x = this->headOffset.x;
	(*headOffset).y = this->headOffset.y;
	(*headOffset).z = this->headOffset.z;
}

CFootstepResource *CPlayerResource::getFootsteps()
{
	return footSteps;
}

void CPlayerResource::create(string playerPath)
{
	string explicitPath = XENGINE_RESOURCES_MEDIA_PATH;

	explicitPath += playerPath;
	explicitPath += "\\";

	// set some defaults

	sex = PS_NONE;

	headOffset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

	footSteps = NULL;

	// go after the animations and a few other pieces of player information

	string animationsPath = explicitPath;
	animationsPath += "animation.cfg";

	ifstream animationsFile(animationsPath.c_str());

	int animNum = 0;
	int frameOffset;

	if (!animationsFile.fail())
	{
		while (!animationsFile.eof())
		{
			string line;

			if (getline(animationsFile, line, '\n'))
			{
				line = lowerCaseString(line);

				vector<string> *tmp = tokenize(line);
				vector<string> tokens = (*tmp);

				delete tmp;

				if (tokens.size())
				{			
					if (tokens[0] == "sex")
					{
						if (tokens.size() != 2)
						{
							// error
						}
						else
						{
							if (tokens[1] == "m")
							{
								sex = PS_MALE;
							}
							else if (tokens[1] == "f")
							{
								sex = PS_FEMALE;
							}
							else if (tokens[1] == "n")
							{
								sex = PS_NONE;
							}
							else
							{
								// error
							}
						}
					}
					else if (tokens[0] == "headoffset")
					{
						if (tokens.size() != 4)
						{
							// error
						}
						else
						{
							headOffset.x = (float)atof(tokens[1].c_str());
							headOffset.y = (float)atof(tokens[2].c_str());
							headOffset.z = (float)atof(tokens[3].c_str());
						}
					}
					else if (tokens[0] == "footsteps")
					{
						if (tokens.size() != 2)
						{
							// error
						}
						else
						{
							if (tokens[1] == "boot" ||
								tokens[1] == "mech" ||
								tokens[1] == "energy" ||
								tokens[1] == "flesh")
							{
								string footstepPath = XENGINE_PLAYERRESOURCE_FOOTSTEPSOUNDS_PATH;
								footstepPath += tokens[1];

								footSteps = CFootstepResource::getFootstep(footstepPath);
							}
							else
							{
								// error
							}
						}
					}
					else
					{
						if (tokens.size() >= 4)
						{
							string firstToken = tokens[0];

							// we're going to go ahead and say that this is good enough to assume
							// that this line has an animation in it

							if (isdigit(firstToken[0]))
							{
								if (animNum < PA_MAXANIMATIONS)
								{
									animations[animNum].startFrame		 = atoi(tokens[0].c_str());
									animations[animNum].endFrame		 = atoi(tokens[1].c_str()) + animations[animNum].startFrame;
									animations[animNum].numLoopingFrames = atoi(tokens[2].c_str());

									animations[animNum].framesPerSecond  = (float)atof(tokens[3].c_str());

									// woo, john carmack is a fucking smack addict

									if (animNum >= PA_LEGS_WALKCROUCHED)
									{
										if (animNum == PA_LEGS_WALKCROUCHED)
										{										
											frameOffset = animations[PA_LEGS_WALKCROUCHED].startFrame - 
								              		      animations[PA_TORSO_GESTURE].startFrame;
										}

										animations[animNum].startFrame -= frameOffset;
										animations[animNum].endFrame   -= frameOffset;
									}

									animNum++;
								}
								else
								{
									// error
								}
							}
						}
					}
				}
			}
		}

		animationsFile.close();
	}
	else
	{
		string debugString = "*** ERROR *** Error loading player animation configuration '";

		debugString += animationsPath;
		debugString += "'\n";

		XEngine->debugPrint(debugString);
	}
	

	if (!footSteps)
	{
		string footstepPath = XENGINE_PLAYERRESOURCE_FOOTSTEPSOUNDS_PATH;
		footstepPath += "step";

		footSteps = CFootstepResource::getFootstep(footstepPath);
	}

	// todo: error checking and skin loading / selection

	// lower body (legs)

	string lowerPath = playerPath;
	lowerPath += "\\lower.md3";
	
	lower = CMD3Resource::getMD3(lowerPath);

	// upper body (torso and arms)

	string upperPath = playerPath;
	upperPath += "\\upper.md3";
	
	upper = CMD3Resource::getMD3(upperPath);

	// and the head

	string headPath = playerPath;
	headPath += "\\head.md3";
	
	head = CMD3Resource::getMD3(headPath);

	playerResources.push_back(this);
	playerResourcesListPosition = playerResources.end();

	__super::create(playerPath);
}

void CPlayerResource::destroy()
{
	if (footSteps)
	{
		footSteps->release();
	}

	lower->release();
	upper->release();
	head->release();

	playerResources.erase(playerResourcesListPosition);

	__super::destroy();
}