// include kernel modules

#include "stdtypes.h"
#include "kernel.h"
#include "console.h"
#include "memory.h"

// entry point

void _main()
{
	Krn_Init();
}

// initialize the kernel

void Krn_Init()
{
	// put some text so we know we actually got into the kernel properly

	Con_Print("Loaded kernel.bin @ 0x100000, x64 enabled, initializing kernel...\n");
	
	// initialize kernel modules

	Con_Init();
	Mem_Init();

	Krn_Panic("End of code");
}

void Krn_Panic(const char *error)
{
	Con_Print(error);

	while (1) {}
}