#ifndef _H_KERNEL_
#define _H_KERNEL_

// pre-declaration of entry point to ensure it is loaded at 0x100000

void _main();

// kernel API

void Krn_Panic(const char *error);

// initialize the kernel

void Krn_Init();

#endif