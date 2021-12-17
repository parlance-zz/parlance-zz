#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string>
#include <windows.h>
#include <winnt.h>

using namespace std;

int GetFileSize(FILE *fp)
{
	int p = ftell(fp);

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);

	fseek(fp, p, SEEK_SET);

	return size;
}

unsigned char *RVAtoPtr(DWORD rva, IMAGE_FILE_HEADER *imageHeader, IMAGE_SECTION_HEADER *sectionHeaders, unsigned char *imageData)
{
	for (int i = 0; i < imageHeader->NumberOfSections; i++)
	{
		if ((rva >= sectionHeaders[i].VirtualAddress) && (rva < (sectionHeaders[i].VirtualAddress + sectionHeaders[i].SizeOfRawData)))
		{
			DWORD rvaSectionOffset = rva - sectionHeaders[i].VirtualAddress;

			return (imageData + rvaSectionOffset + sectionHeaders[i].PointerToRawData);
		}
	}

	return NULL;
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		printf("\nUsage: injector.exe source.exe destination.exe\n\n");
		return 0;
	}

	string sourceFileName  = argv[1];
	string destFileName    = argv[2];

	string outputFileName = string(argv[2], strlen(argv[2]) - 4) + string(".injected.exe");

	// ****************************************************

	FILE *sourceFile = fopen(sourceFileName.c_str(), "rb");

	if (!sourceFile)
	{
		printf("Error - Unable to open source file '%s'\n", sourceFileName.c_str());
		return 1;
	}

	FILE *destFile = fopen(destFileName.c_str(), "rb");

	if (!destFile)
	{
		printf("Error - Unable to open destination file '%s'\n", destFileName.c_str());
		return 1;
	}

	FILE *outputFile = fopen(outputFileName.c_str(), "wb");

	if (!outputFile)
	{
		printf("Error - Unable to open output file '%s'\n", outputFileName.c_str());
		return 1;
	}

	FILE *stubFile = fopen("stub.bin", "rb");

	if (!stubFile)
	{
		printf("Error - Unable to open stub file 'stub.bin'\n");
		return 1;
	}

	// ****************************************************

	int sourceDataSize = GetFileSize(sourceFile);
	unsigned char *sourceData = new unsigned char[sourceDataSize];
	fread(sourceData, 1, sourceDataSize, sourceFile);
	fclose(sourceFile);

	int destDataSize = GetFileSize(destFile);
	unsigned char *destData = new unsigned char[destDataSize];
	fread(destData, 1, destDataSize, destFile);
	fclose(destFile);

	int stubDataSize = GetFileSize(stubFile);
	unsigned char *stubData = new unsigned char[stubDataSize];
	fread(stubData, 1, stubDataSize, stubFile);
	fclose(stubFile);

	// ****************************************************

	if ((destData[0] != 0x4D) || (destData[1] != 0x5A))
	{
		printf("Error - DOS signature not found\n");
		return 1;
	}

	DWORD startOfPE = 0;

	for (int i = 2; i < 0x400; i++) // apparently you can have whatever length dos stub you want
	{
		DWORD *ptr = (DWORD*)&destData[i];

		if (*ptr == 0x00004550)
		{
			startOfPE = i + 4;

			break;
		}
	}

	if (!startOfPE)
	{
		printf("Error - PE signature not found\n");
		return 1;
	}

	IMAGE_FILE_HEADER *imageHeader = (IMAGE_FILE_HEADER*)(destData + startOfPE);

	if (imageHeader->Machine != IMAGE_FILE_MACHINE_I386)
	{
		printf("Error - Machine type is not i386\n");
		return 1;
	}

	if (!(imageHeader->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
	{
		printf("Error - PE file is not executable type\n");
		return 1;
	}
	
	if (imageHeader->NumberOfSections >= 12)
	{
		printf("Error - Too many existing sections, no new room for new section\n");
		return 1;
	}

	if (imageHeader->SizeOfOptionalHeader != 0xE0)
	{
		printf("Error - No optional header found\n");
		return 1;
	}

	IMAGE_OPTIONAL_HEADER32 *optHeader = (IMAGE_OPTIONAL_HEADER32*)((char*)imageHeader + sizeof(IMAGE_FILE_HEADER));

	if (optHeader->Magic != 0x010B)
	{
		printf("Error - Optional header signature not PE32\n");
		return 1;
	}
	if (optHeader->SizeOfHeaders < 0x400)
	{
		printf("Error - Size of headers too small\n");
		return 1;
	}

	if (optHeader->ImageBase < 0x200000)
	{
		printf("Error - Image base too low\n");
		return 1;
	}

	if (optHeader->NumberOfRvaAndSizes != 16)
	{
		printf("Error - Number of data directories unexpected\n");
		return 1;
	}

	// ****************************************************

	IMAGE_SECTION_HEADER *sectionHeaders = (IMAGE_SECTION_HEADER*)((char*)optHeader + sizeof(IMAGE_OPTIONAL_HEADER32));

	DWORD importTableRVA        = optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	DWORD importTableNumEntries = (optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size / sizeof(IMAGE_IMPORT_DESCRIPTOR)) - 1;

	IMAGE_IMPORT_DESCRIPTOR *importDescriptors = (IMAGE_IMPORT_DESCRIPTOR*)RVAtoPtr(importTableRVA, imageHeader, sectionHeaders, destData);

	if (!importDescriptors)
	{
		printf("Unable to locate import table in any section\n");
		return 1;
	}

	DWORD loadLibraryRVA    = 0;
	DWORD getProcAddressRVA = 0;

	for (int d = 0; d < importTableNumEntries; d++)
	{
		char *szImportDLLName = (char*)RVAtoPtr(importDescriptors[d].Name, imageHeader, sectionHeaders, destData);
		
		if (szImportDLLName)
		{
			string dllName = string(szImportDLLName);

			for (int i = 0; i < dllName.length(); i++)
			{
				dllName[i] = tolower(dllName[i]);
			}

			if (dllName == string("kernel32.dll"))
			{
				DWORD *thunks = (DWORD*)RVAtoPtr(importDescriptors[d].OriginalFirstThunk, imageHeader, sectionHeaders, destData);

				if (!thunks)
				{
					thunks = (DWORD*)RVAtoPtr(importDescriptors[d].FirstThunk, imageHeader, sectionHeaders, destData);

					if (!thunks)
					{
						printf("Unable to locate thunk table in file for KERNEL32.dll import descriptor\n");
						return 1;
					}
				}

				DWORD thunkIndex = 0;

				while (*thunks)
				{
					IMAGE_IMPORT_BY_NAME *functionImport = (IMAGE_IMPORT_BY_NAME*)RVAtoPtr(*thunks, imageHeader, sectionHeaders, destData);

					if (functionImport)
					{
						string functionName = string((char*)functionImport->Name);

						if ((functionName == string("LoadLibraryA")) || (functionName == string("GetModuleHandleA")))
						{
							if ((!loadLibraryRVA) || (functionName == string("GetModuleHandleA")))
							{
								loadLibraryRVA = importDescriptors[d].FirstThunk + thunkIndex * 4;
								if (getProcAddressRVA && loadLibraryRVA) break;
							}
						}
						else if (functionName == string("GetProcAddress"))
						{
							if (!getProcAddressRVA)
							{
								getProcAddressRVA = importDescriptors[d].FirstThunk + thunkIndex * 4;
								if (getProcAddressRVA && loadLibraryRVA) break;
							}
						}
					}

					thunks++;

					thunkIndex++;
				}

				if (getProcAddressRVA && loadLibraryRVA) break;
			}
		}
	}

	if ((!getProcAddressRVA) || (!loadLibraryRVA))
	{
		printf("Unable to locate import for GetProcAddress or LoadLibraryA\n");
		return 1;
	}

	// ****************************************************

	DWORD vAlignMaskSub = optHeader->SectionAlignment - 1;
	DWORD vAlignMask    = 0xFFFFFFFF ^ vAlignMaskSub;

	DWORD fAlignMaskSub = optHeader->FileAlignment - 1;
	DWORD fAlignMask    = 0xFFFFFFFF ^ fAlignMaskSub;

	DWORD virtualSizeOfLastSection = ((DWORD*)&sectionHeaders[imageHeader->NumberOfSections - 1])[2];
	if (virtualSizeOfLastSection & vAlignMaskSub) virtualSizeOfLastSection = (virtualSizeOfLastSection & vAlignMask) + optHeader->SectionAlignment;

	DWORD newSectionRVA         = sectionHeaders[imageHeader->NumberOfSections - 1].VirtualAddress + virtualSizeOfLastSection;

											   // this should've been fine, but apparently some faggots just put shit at the end of their exes for fun
	DWORD newSectionFileAddress = destDataSize;//sectionHeaders[imageHeader->NumberOfSections - 1].PointerToRawData + sectionHeaders[imageHeader->NumberOfSections - 1].SizeOfRawData;
	if (newSectionFileAddress & fAlignMaskSub) newSectionFileAddress = (newSectionFileAddress & fAlignMask) + optHeader->FileAlignment;

	DWORD realSizeOfNewSection = stubDataSize + sourceDataSize;

	DWORD rawSizeOfNewSection = (realSizeOfNewSection);
	if (rawSizeOfNewSection & fAlignMaskSub) rawSizeOfNewSection = (rawSizeOfNewSection & fAlignMask) + optHeader->FileAlignment;

	DWORD vSizeOfNewSection = (realSizeOfNewSection);
	if (vSizeOfNewSection & vAlignMaskSub) vSizeOfNewSection = (vSizeOfNewSection & vAlignMask) + optHeader->SectionAlignment;

	IMAGE_SECTION_HEADER *newSection = &sectionHeaders[imageHeader->NumberOfSections];

	newSection->Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
	DWORD *tmp = (DWORD*)newSection; tmp[2] = realSizeOfNewSection;
	char sectionName[] = {'.','e','x','t','r','s','r','c'}; memcpy(newSection->Name, sectionName, 8);
	newSection->NumberOfLinenumbers = 0;
	newSection->NumberOfRelocations = 0;
	newSection->PointerToLinenumbers = 0;
	newSection->PointerToRawData = newSectionFileAddress;
	newSection->PointerToRelocations = 0;
	newSection->SizeOfRawData = rawSizeOfNewSection;
	newSection->VirtualAddress = newSectionRVA;

	imageHeader->NumberOfSections++;
	
	DWORD originalEntryPoint = optHeader->AddressOfEntryPoint;

	optHeader->AddressOfEntryPoint = newSectionRVA + 23;
	optHeader->DllCharacteristics &= 0xFFFFFFBF;
	//optHeader->SizeOfCode  += rawSizeOfNewSection;
	//optHeader->SizeOfInitializedData += rawSizeOfNewSection;
	optHeader->SizeOfImage += vSizeOfNewSection;
	optHeader->CheckSum = 0; // possibly try computing it

	// patch VAs for functions into stubdata

	DWORD *patchData = (DWORD*)stubData;

	patchData[0] = getProcAddressRVA + optHeader->ImageBase;
	patchData[1] = loadLibraryRVA + optHeader->ImageBase;
	patchData[2] = newSectionRVA + optHeader->ImageBase + stubDataSize;
	patchData[3] = sourceDataSize;
	patchData[4] = originalEntryPoint + optHeader->ImageBase;
	patchData[6] = newSectionRVA + optHeader->ImageBase;

	fwrite(destData, 1, destDataSize, outputFile);

	DWORD padSize = newSectionFileAddress - destDataSize;

	for (int i = 0; i < padSize; i++)
	{
		char tmpnull = 0;

		fwrite(&tmpnull, 1, 1, outputFile);
	}
	
	fwrite(stubData, 1, stubDataSize, outputFile);
	fwrite(sourceData, 1, sourceDataSize, outputFile);

	padSize = rawSizeOfNewSection - realSizeOfNewSection;

	for (int i = 0; i < padSize; i++)
	{
		char tmpnull = 0;

		fwrite(&tmpnull, 1, 1, outputFile);
	}

	fclose(outputFile);

	printf("Successfully injected into '%s'\n\n", outputFileName.c_str());

	return 0;
}