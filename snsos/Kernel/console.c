#include "stdtypes.h"
#include "console.h"
#include "memory.h"

// platform specific console constants

#define CON_BUFFER 0xB8000
#define CON_WIDTH  80
#define CON_HEIGHT 25

char *Con_buffer = (char*)CON_BUFFER;	// location of text mode framebuffer in memory

// used for tracking printing location

uint8 Con_X = 0;
uint8 Con_Y = 0;

// used to accelerate printing integers

char *Con_writeIntBuffer = "00000000000000000000\0";

// used to accelerate printing hex strings

char *Con_writeHexBuffer = "0x0000000000000000\0";
char *Con_writeHexChars  = "0123456789abcdef";

// these print various sized integer strings

void Con_WriteInt8(int8 ival);
void Con_WriteInt16(int16 ival);
void Con_WriteInt32(int32 ival);
void Con_WriteInt64(int64 ival);

// these print various sized hex strings

void Con_WriteHex8(uint8 val);
void Con_WriteHex16(uint16 val);
void Con_WriteHex32(uint32 val);
void Con_WriteHex64(uint64 val);

void Con_Write(const char *text);			// print a non-escaped null terminated string to the console

// *******************************

void Con_Init()
{
	Con_Write("Initializing console...\n");
}

void Con_WriteInt8(int8 ival)
{
	char *buffer = &Con_writeIntBuffer[20 - 1];
	uint8 val = ival; if (val > 0x80) val >>= 1;
	
	for (uint8 i = 0; i < 3; i++)
	{
		*buffer = (val % 10) + 0x30; val /= 10;
		
		if (val)
		{
			buffer--;
		}
		else
		{
			break;
		}
	}

	if (ival < 0)
	{
		buffer--;
		*buffer = '-';
	}

	Con_Write(buffer);
}

void Con_WriteInt16(int16 ival)
{
	char *buffer = &Con_writeIntBuffer[20 - 1];
	uint16 val = ival; if (val > 0x8000) val >>= 1;
	
	for (uint8 i = 0; i < 5; i++)
	{
		*buffer = (val % 10) + 0x30; val /= 10;
		
		if (val)
		{
			buffer--;
		}
		else
		{
			break;
		}
	}

	if (ival < 0)
	{
		buffer--;
		*buffer = '-';
	}

	Con_Write(buffer);
}

void Con_WriteInt32(int32 ival)
{
	char *buffer = &Con_writeIntBuffer[20 - 1];
	uint32 val = ival; if (val > 0x80000000) val >>= 1;
	
	for (uint8 i = 0; i < 10; i++)
	{
		*buffer = (val % 10) + 0x30; val /= 10;
		
		if (val)
		{
			buffer--;
		}
		else
		{
			break;
		}
	}

	if (ival < 0)
	{
		buffer--;
		*buffer = '-';
	}

	Con_Write(buffer);
}

void Con_WriteInt64(int64 ival)
{
	char *buffer = &Con_writeIntBuffer[20 - 1];
	uint64 val = ival; if (val > 0x8000000000000000) val >>= 1;
	
	for (uint8 i = 0; i < 20; i++)
	{
		*buffer = (val % 10) + 0x30; val /= 10;
		
		if (val)
		{
			buffer--;
		}
		else
		{
			break;
		}
	}

	if (ival < 0)
	{
		buffer--;
		*buffer = '-';
	}

	Con_Write(buffer);
}

void Con_WriteHex8(uint8 val)
{
	Con_writeHexBuffer[2] = Con_writeHexChars[val >> 4  ];
	Con_writeHexBuffer[3] = Con_writeHexChars[val & 0x0F];

	Con_writeHexBuffer[4] = 0;

	Con_Write(Con_writeHexBuffer);
}

void Con_WriteHex16(uint16 val)
{
	Con_writeHexBuffer[2] = Con_writeHexChars[(val >> 12)       ];
	Con_writeHexBuffer[3] = Con_writeHexChars[(val >> 8 ) & 0x0F];
	Con_writeHexBuffer[4] = Con_writeHexChars[(val >> 4 ) & 0x0F];
	Con_writeHexBuffer[5] = Con_writeHexChars[(val      ) & 0x0F];

	Con_writeHexBuffer[6] = 0;

	Con_Write(Con_writeHexBuffer);
}

void Con_WriteHex32(uint32 val)
{
	Con_writeHexBuffer[2] = Con_writeHexChars[(val >> 28)        ];
	Con_writeHexBuffer[3] = Con_writeHexChars[(val >> 24 ) & 0x0F];
	Con_writeHexBuffer[4] = Con_writeHexChars[(val >> 20 ) & 0x0F];
	Con_writeHexBuffer[5] = Con_writeHexChars[(val >> 16 ) & 0x0F];

	Con_writeHexBuffer[6] = Con_writeHexChars[(val >> 12) & 0x0F];
	Con_writeHexBuffer[7] = Con_writeHexChars[(val >> 8 ) & 0x0F];
	Con_writeHexBuffer[8] = Con_writeHexChars[(val >> 4 ) & 0x0F];
	Con_writeHexBuffer[9] = Con_writeHexChars[(val      ) & 0x0F];

	Con_writeHexBuffer[10] = 0;

	Con_Write(Con_writeHexBuffer);
}

void Con_WriteHex64(uint64 val)
{
	Con_writeHexBuffer[2] = Con_writeHexChars[(val >> 60)       ];
	Con_writeHexBuffer[3] = Con_writeHexChars[(val >> 56) & 0x0F];
	Con_writeHexBuffer[4] = Con_writeHexChars[(val >> 52) & 0x0F];
	Con_writeHexBuffer[5] = Con_writeHexChars[(val >> 48) & 0x0F];

	Con_writeHexBuffer[6] = Con_writeHexChars[(val >> 44) & 0x0F];
	Con_writeHexBuffer[7] = Con_writeHexChars[(val >> 40) & 0x0F];
	Con_writeHexBuffer[8] = Con_writeHexChars[(val >> 36) & 0x0F];
	Con_writeHexBuffer[9] = Con_writeHexChars[(val >> 32) & 0x0F];

	Con_writeHexBuffer[10] = Con_writeHexChars[(val >> 28)  & 0x0F];
	Con_writeHexBuffer[11] = Con_writeHexChars[(val >> 24 ) & 0x0F];
	Con_writeHexBuffer[12] = Con_writeHexChars[(val >> 20 ) & 0x0F];
	Con_writeHexBuffer[13] = Con_writeHexChars[(val >> 16 ) & 0x0F];

	Con_writeHexBuffer[14] = Con_writeHexChars[(val >> 12) & 0x0F];
	Con_writeHexBuffer[15] = Con_writeHexChars[(val >> 8 ) & 0x0F];
	Con_writeHexBuffer[16] = Con_writeHexChars[(val >> 4 ) & 0x0F];
	Con_writeHexBuffer[17] = Con_writeHexChars[(val      ) & 0x0F];

	// implicit null at end of hex buffer

	Con_Write(Con_writeHexBuffer);
}

void Con_Write(const char *text)
{
	while (*text)	// null terminated string
	{
		if (*text == 0x0A)	// line feed
		{
			Con_Y++;
			Con_X = 0;
		}
		else
		{
			Con_buffer[(Con_Y * CON_WIDTH + Con_X) << 1] = *text;	// console characters are 2 bytes wide

			Con_X++;
		}

		if (Con_X >= CON_WIDTH)	// wrap
		{
			Con_X = 0;
			Con_Y++;
		}

		if (Con_Y >= CON_HEIGHT)	// scroll
		{
			Con_Y = CON_HEIGHT - 1;

			Mem_Copy(Con_buffer, Con_buffer + (CON_WIDTH << 1), ((CON_WIDTH * (CON_HEIGHT - 1)) << 1));
			
			for (uint8 x = 0; x < CON_WIDTH; x++)
			{
				Con_buffer[((CON_HEIGHT - 1) * CON_WIDTH + x) << 1] = 0;
			}
		}

		text++;
	}
}

// *******************************

void Con_Print(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[2] = {0};

	uint8  uval8;
	uint16 uval16;
	uint32 uval32;
	uint64 uval64;

	int8  val8;
	int16 val16;
	int32 val32;
	int64 val64;

	char *string;

	while (*format)
	{
		if ((*format) == 0x25)	// '%' character
		{
			format++;

			switch (*format)
			{
			case 0x00:

				goto printFinish;

			case 0x73: // 's'

				string = va_arg(args, char*);
				Con_Write(string);

				break;

			case 0x68: // 'h'

				format++;

				switch (*format)
				{
				default:
				case 0x00:

					goto printFinish;

				case 0x31: // '1'
				
					uval8 = va_arg(args, int);
					Con_WriteHex8(uval8);

					break;

				case 0x32: // '2'

					uval16 = va_arg(args, int);
					Con_WriteHex16(uval16);

					break;

				case 0x34: // '4'

					uval32 = va_arg(args, int);
					Con_WriteHex32(uval32);

					break;
	
				case 0x38: // '8'

					uval64 = va_arg(args, int);
					Con_WriteHex64(uval64);
				}

				format++;

				break;

			case 0x69: // 'i'

				format++;

				switch (*format)
				{
				default:
				case 0x00:

					goto printFinish;

				case 0x31: // '1'
				
					val8 = va_arg(args, int);
					Con_WriteInt8(val8);

					break;

				case 0x32: // '2'

					val16 = va_arg(args, int);
					Con_WriteInt16(val16);

					break;

				case 0x34: // '4'

					val32 = va_arg(args, int);
					Con_WriteInt32(val32);

					break;
				
				case 0x38: // '8'

					val64 = va_arg(args, int);
					Con_WriteInt64(val64);
				}

				format++;
			}
		}
		else
		{
			buffer[0] = *format;
			Con_Write(buffer);

			format++;
		}
	}
	
printFinish:	

	va_end(args);
}
