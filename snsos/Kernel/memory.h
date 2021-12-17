#ifndef _H_MEMORY_
#define _H_MEMORY_

void Mem_Init();	// initialize memory subsystem - physical memory map -> page tables

#define MEM_ALLOC_WRITEABLE	    0x02  // enable writes to page, no idea if higher levels supercede lower levels or what
#define MEM_ALLOC_USERACCESS    0x04  // enable CPL3 (user mode) access to the pages mapped by the entry, no idea if this supercedes either
#define MEM_ALLOC_WRITETHROUGH  0x08  // if set, page has writethrough cache policy, else, writeback, no idea what this does really
#define MEM_ALLOC_CACHEDISABLE  0x10  // disable any and all caching of reads / writes to this page

// memory interaction kernel API

uint64 Mem_GetPhysicalMemorySize();
uint64 Mem_GetAvailableMemorySize();

void  *Mem_PhysAlloc(void *baseAddress, uint64 size, uint8 flags);  // allocate physical memory, base address can be null in which case
																    // physical memory will be searched for a suitable location.
																	// if none can be found or the specified range is occupied
																	// null will be returned, else a pointer to the memory allocated will be returned
																	// flags is a set of MEM_ALLOC_* flags specifying the memory protection and performance attributes

uint64 Mem_PhysFree(void *baseAddress, uint64 size);				// de-allocate physical memory, returns amount freed, 0 on error

void Mem_Copy(void *dest, const void *source, uint64 num); // copy a given number of bytes from source to destination buffer
void Mem_Set(void *dest, uint8 val, uint64 num);           // set a memory buffer to a specific byte value

#endif