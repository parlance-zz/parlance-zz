#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

struct VHD_HEADER
{
	char          cookie[8];
	int           features;
	int           fileVersion;
	long long int dataOffset;
	int           timeStamp;
	char          creatorApplication[4];
	int           creatorVersion;
	int           creatorHostOS;
	unsigned long long int originalSize;
	unsigned long long int currentSize;
	unsigned short cylinders;
	unsigned char  heads;
	unsigned char  sectorsPerTrack;
	int           diskType;
	unsigned int  checkSum;
	char          uniqueID[16];
	char          savedState;
	char          reserved[427];
};

struct PARTITION_RECORD
{
	char status;
	char fhead;
	char fsector;
	char fcylinder;
	char type;
	char lhead;
	char lsector;
	char lcylinder;
	unsigned int flba;
	unsigned int nblocks;
};

unsigned short ReverseEndian(unsigned short val)
{
	return ((val & 0xFF) << 8) | ((val & 0xFF00) >> 8);
}

unsigned long long int ReverseEndian(unsigned long long int val)
{
	unsigned long long int newVal = 0;

	char *sptr = (char *)&val;
	char *dptr = (char *)&newVal;
	
	for (int i = 0; i < 8; i++)
	{
		dptr[i] = sptr[8 - i - 1];
	}

	return newVal;
}

unsigned int ReverseEndian(unsigned int val)
{
	unsigned int newVal = 0;

	char *sptr = (char *)&val;
	char *dptr = (char *)&newVal;
	
	for (int i = 0; i < 4; i++)
	{
		dptr[i] = sptr[4 - i - 1];
	}

	return newVal;
}

int main(int argc, char *argv[])
{
	if (argc != 8)
	{
		printf("Usage: virtualinstall {-bin | -vhd} mbr.bin boot.bin kernel.bin \n");

		return 0;
	}

	string inputFileName  = argv[1];
	string outputFileName = string(argv[1], strlen(argv[1]) - 4) + string(".vhd");

	FILE *input = fopen(inputFileName.c_str(), "rb");

	fseek(input, 0, SEEK_END);
	int   inputSize = ftell(input);
	char *inputData = new char[inputSize];

	fseek(input, 0, SEEK_SET);
	fread(inputData, 1, inputSize, input);

	fclose(input);

	FILE *output = fopen(outputFileName.c_str(), "wb");
	
	VHD_HEADER header;

	char cookie[] = {'c','o','n','e','c','t','i','x'};
	memcpy(header.cookie, cookie, 8);

	header.features = 0x02000000; // reserved
	header.fileVersion = 0x00000100;
	header.dataOffset = 0xFFFFFFFFFFFFFFFF;
	header.timeStamp  = 0x3C895413;

	char creatorApplication[] = {'v','p','c',' '};
	memcpy(header.creatorApplication, creatorApplication, 4);

	header.creatorVersion = 0x03000500;
	header.creatorHostOS = 0x6B326957;

	header.originalSize = inputSize;
	header.currentSize  = inputSize;
	header.originalSize = ReverseEndian(header.originalSize);
	header.currentSize  = ReverseEndian(header.currentSize);

	int totalSectors = inputSize / 512;
	if (inputSize % 512 > 0) totalSectors++;

	int sectorsPerTrack;
	int heads;
	int cylinderTimesHeads;
	int cylinders;

	if (totalSectors > 65535 * 16 * 255)
	{
		totalSectors = 65535 * 16 * 255;
	}

	if (totalSectors >= 65535 * 16 * 63)
	{
		sectorsPerTrack = 255;
		heads = 16;
		cylinderTimesHeads = totalSectors / sectorsPerTrack;
	}
	else
	{
		sectorsPerTrack = 17; 
		cylinderTimesHeads = totalSectors / sectorsPerTrack;

		heads = (cylinderTimesHeads + 1023) / 1024;
	      
		if (heads < 4)
		{
			heads = 4;
		}

		if (cylinderTimesHeads >= (heads * 1024) || heads > 16)
		{
			sectorsPerTrack = 31;
			heads = 16;
			cylinderTimesHeads = totalSectors / sectorsPerTrack;	
		}

		if (cylinderTimesHeads >= (heads * 1024))
		{
			sectorsPerTrack = 63;
			heads = 16;
			cylinderTimesHeads = totalSectors / sectorsPerTrack;
		}
	}

	cylinders = cylinderTimesHeads / heads;

	header.cylinders = cylinders;
	header.cylinders = ReverseEndian(header.cylinders);

	header.heads = heads;
	header.sectorsPerTrack = sectorsPerTrack;

	header.diskType = 0x02000000; // fixed disk
	header.checkSum = 0;

	char uniqueID[] = {0xD3,0xDD,0x50,0x69,0x45,0xAB,0x11,0xDF,0x83,0x53,0xAD,0x78,0xCD,0x91,0x01,0xE3};
	memcpy(header.uniqueID, uniqueID, 16);

	header.savedState = 0;

	memset(header.reserved, 0, 427);

	unsigned int checkSum = 0;
	char *footer = (char *)&header;
	
	for (int i = 0; i < sizeof(VHD_HEADER); i++)
	{
		checkSum += footer[i];
	}

	header.checkSum = ~checkSum;
	header.checkSum = ReverseEndian(header.checkSum);

	fwrite(inputData, 1, inputSize, output);
	fwrite(&header, 1, sizeof(VHD_HEADER), output);
	
	fclose(output);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 7)
	{
		printf("Usage: makembr sourcembr.bin destmbr.bin status type flba nblocks\n");

		return 0;
	}

	string inputFileName  = argv[1];
	string outputFileName = argv[2];

	FILE *input = fopen(inputFileName.c_str(), "rb");

	int   inputSize = 512;
	char *inputData = new char[inputSize];

	fread(inputData, 1, inputSize, input);
	fclose(input);

	GPT_RECORD *gpt = (GPT_RECORD*)&inputData[446];

	gpt->status    = atoi(argv[3]);
	gpt->type      = atoi(argv[4]);
	gpt->flba	   = atoi(argv[5]); // may need to be adjusted as apparently blocks start at 1?
	gpt->nblocks   = atoi(argv[6]);

	// needs fixing, quick hack for now

	gpt->fhead     = 0;
	gpt->fsector   = gpt->flba + 1;
	gpt->fcylinder = 0;

	gpt->lhead     = 0;
	gpt->lsector   = gpt->flba + gpt->nblocks;
	gpt->lcylinder = 0;


	FILE *output = fopen(outputFileName.c_str(), "wb");
	fwrite(inputData, 1, inputSize, output);
	fclose(output);

	return 0;
}