#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		printf("Usage: padbin source.bin dest.bin size\n");

		return 0;
	}

	string inputFileName  = argv[1];
	string outputFileName = argv[2];
	
	unsigned int outputSize = atoi(argv[3]);

	FILE *input = fopen(inputFileName.c_str(), "rb");

	fseek(input, 0, SEEK_END);
	unsigned int inputSize = ftell(input);
	fseek(input, 0, SEEK_SET);

	if (outputSize < inputSize)
	{
		printf("Error: Input size for %s %i is larger than output size %i\n", inputFileName.c_str(), inputSize, outputSize);
		
		fclose(input);

		return 1;
	}

	char *inputData = (char*)malloc(inputSize);

	fread(inputData, 1, inputSize, input);
	fclose(input);

	FILE *output = fopen(outputFileName.c_str(), "wb");
	fwrite(inputData, 1, inputSize, output);

	char zero = 0;

	for (unsigned int i = inputSize; i < outputSize; i++)
	{
		fwrite(&zero, 1, 1, output);
	}

	fclose(output);

	return 0;
}