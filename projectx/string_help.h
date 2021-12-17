#ifndef _H_STRING_HELP
#define _H_STRING_HELP

#include <stdio.h>
#include <string>
#include <vector>

using namespace std;

char *intString(int num);
char *floatString(float num);

string correctPathSlashes(string myString);
string lowerCaseString(string myString);

vector<string> *tokenize(string myString);			// tokenizes the string and returns the number of tokens
vector<string> *tokenize(const char *myCString);	// likewise

#endif