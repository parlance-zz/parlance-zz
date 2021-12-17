#include "string_help.h"

static char buff[32] = {0};

char *intString(int num)
{	
	sprintf(buff, "%i", num);

	return buff;
}

char *floatString(float num)
{
	sprintf(buff, "%f", num);

	return buff;
}

string correctPathSlashes(string myString)
{
	for (int ch = 0; ch < (int)myString.length(); ch++)
	{
		if (myString[ch] == '/')
		{
			myString[ch] = '\\';
		}
	}

	return myString;
}

string lowerCaseString(string myString)
{
	for (int ch = 0; ch < (int)myString.length(); ch++)
	{
		if (myString[ch] >= 65 && myString[ch] <= 90)
		{
			myString[ch] += 32;
		}
	}

	return myString;
}

vector<string> *_tokenize(string myString)
{
	vector<string> *tokens = new vector<string>;

	bool readingWhiteSpace;
	string currentToken = "";

	if (!myString.length())
	{
		return tokens;
	}
	else
	{
		readingWhiteSpace = (myString[0] == '\t' || myString[0] == ' ' || myString[0] == '\n');
	}

	int commentPos = myString.find("//");

	if (commentPos > 0)
	{
		myString = myString.substr(0, commentPos);
	}
	else if (!commentPos)
	{
		myString = "";
	}
	
	for (int c = 0; c < (int)myString.length(); c++)
	{
		char myChar = myString[c];

		if (myChar == '\t' || myChar == ' ' || myChar == '\n')
		{
			if (!readingWhiteSpace)
			{
				tokens->push_back(currentToken);

				currentToken = "";
				readingWhiteSpace = true;
			}
		}
		else
		{
			currentToken += myChar;

			readingWhiteSpace = false;
		}
	}

	if (currentToken.length() > 0)
	{
		tokens->push_back(currentToken);
	}

	return tokens;
}

vector<string> *tokenize(string myString)
{
	return _tokenize(myString);
}

vector<string> *tokenize(const char *myCString)
{
	string myString = myCString;

	return _tokenize(myString);
}