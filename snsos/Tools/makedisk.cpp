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
	if (argc != 4)
	{
		printf("Usage: makedisk {-bin | -vhd} disksize filename\n");

		return 0;
	}
	
	unsigned long long diskSize = atoi(argv[2]);
	char *outputFileName = argv[3];

	FILE *output = fopen(outputFileName, "wb");
	
	char buffer[512] = {0};
	
	for (unsigned long long i = 0; i < diskSize; i += 512)
	{
		fwrite(buffer, 1, 512, output);
	}

	string format = string(argv[1]);

	if (format == string("-bin"))
	{
		fclose(output);

		return 0;
	}
	else if (format == string("-vhd"))
	{
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

		header.originalSize = diskSize;
		header.currentSize  = diskSize;
		header.originalSize = ReverseEndian(header.originalSize);
		header.currentSize  = ReverseEndian(header.currentSize);

		unsigned long long totalSectors = diskSize / 512;
		if (diskSize % 512 > 0) totalSectors++;

		unsigned int sectorsPerTrack;
		unsigned int heads;
		unsigned int cylinderTimesHeads;
		unsigned int cylinders;

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

		fwrite(&header, 1, sizeof(VHD_HEADER), output);		
		fclose(output);

		return 0;
	}
	else
	{
		printf("Error: Unknown argument '%s'\n", format.c_str());

		return 1;
	}
}