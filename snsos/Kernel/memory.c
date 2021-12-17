#include "stdtypes.h"
#include "memory.h"
#include "console.h"
#include "kernel.h"

#define MEM_MIN_REQ_MEMORY	0x2000000	// 16mb minimum free memory required

// the memory layout is as follows:

// 0x000000000000 - 0x800000000000 lower 512gb [kernel memory space, 1:1 mapping with physical memory]
// 0x800000000000 - 0xFFFFFFFFFFFF upper 512gb [user mode memory space, mapping to physical addresses may not be continuous]

// 0x000000 - 0x000500 - used by BIOS and real mode IVT
// 0x000500 - 0x007C00 - free but currently unused, stack set to 0x7C00 in MBR
// 0x007c00 - 0x007e00 - MBR boot loader
// 0x007e00 - 0x008000 - free, but currently unused, stack in boot loader is set to 0x8000
// 0x008000 - 0x008004 - number of entries in memory region tables initialized by int 0x15
// 0x008004 - 0x00a000 - memory region tables initialized by int 0x15
// 0x00a000 - 0x00b000 - temporary initial PML4 setup in boot loader (used to map lowest 2MB to loader kernel)
// 0x00b000 - 0x00c000 - temporary initial PDP setup in boot loader  (used to map lowest 2MB to loader kernel)
// 0x00c000 - 0x00d000 - temporary initial PDE setup in boot loader  (used to map lowest 2MB to loader kernel)
// 0x00d000 - 0x010000 - free but currently unused
// 0x010000 - 0x020000 - boot loader
// 0x020000 - 0x028000 - PDE presence tables, each bit represents a 2MB region from 0 to 512gb and whether or not it exists
// 0x028000 - 0x030000 - PDE free tables, each bit represents a 2MB region from 0 to 512gb and whether or not it is free
// 0x030000 - 0x080000 - free but currently unused
// 0x080000 - 0x081000 - permanent PML4 will be setup in the kernel (entry at 0x80000 points to kernel PDP, 0x80008 will point to whatever PDP is used to map user space in the current process)
// 0x081000 - 0x082000 - permanent physical / kernel space PDP (lower 512gb address space)
// 0x082000 - 0x09fc00 - free, but currently unused
// 0x09fc00 - 0x0a0000 - extended bios data area, unusable
// 0x0a0000 - 0x100000 - mapped video memory, 0xb8000 is framebuffer for text mode used by console
// 0x100000 - 0x180000 - kernel.bin, currently 512kb
// 0x180000 - 0x200000 - kernel mode stack descending from 0x200000, currently 512kb
// 0x200000 - 0x400000 - permanent kernel mode PDE tables mapping physical memory 1:1

// page table flags

// page table entry format is basically PTF flags combined with the physical address of either
// another page of table entries (if PAGESIZE is unset) or the physical address of the page mapped (if pagesize is set, can only be set in the PDE or PTE levels)
// if pagesize IS set the physical address needs to be aligned to an address of the corresponding size for that level (2MB for PDE, 4kb for PTE) (because the remaining bits will be used for flags or reserved)
// the size of an entry is 64 bits and there are 512 entries in each mapped entry / level

// sizes mapped by each level:

// PML4 - 512 GB / entry (512 entries)
// PDP  - 1 GB   / entry (512 entries)
// PDE  - 2 MB   / entry (512 entries)
// PTE  - 4 kb   / entry (512 entries)

#define MEM_PTF_PRESENT	      0x01	// 1 in any valid mapped entry, else all should be 0
#define MEM_PTF_WRITEABLE	  0x02  // enable writes to page, no idea if higher levels supercede lower levels or what
#define MEM_PTF_USERACCESS    0x04  // enable CPL3 (user mode) access to the pages mapped by the entry, no idea if this supercedes either
#define MEM_PTF_WRITETHROUGH  0x08  // if set, page has writethrough cache policy, else, writeback, no idea what this does really
#define MEM_PTF_CACHEDISABLE  0x10  // disable any and all caching of reads / writes to this page
#define MEM_PTF_ACCESSED	  0x20  // CPU will set this when page has been read from
#define MEM_PTF_DIRTY		  0x40	// only used in lowest level of page table only (lowest level is where PAGESIZE is set), CPU sets when page has been written to
#define MEM_PTF_PAGESIZE	  0x80	// set in lowest level of page table only, else unset
#define MEM_PTF_GLOBALPAGE	  0x100 // set if this pages entries in the TLB cache should not be cleared when CR3 (PML4 pointer) is reset
#define MEM_PTF_NOEXECUTE     0x8000000000000000 // disables code execution in pages mapped by the entry

#define MEM_PML4_ADDRESS	   0x80000 // location of the one and only PML4 table
#define MEM_KERNEL_PDP_ADDRESS 0x81000 // lower 512gb page table, 1:1 mapping used for kernel space

#define MEM_KERNEL_PDE_PRESENCE_ADDRESS 0x010000 // PDE presence tables, each bit represents a 2MB region from 0 to 512gb and whether or not it is exists physically
#define MEM_KERNEL_PDE_FREE_ADDRESS     0x018000 // PDE free tables, each bit represents a 2MB region from 0 to 512gb and whether or not it is free

struct Mem_PDEPresenceTable *Mem_KernelPDEPresenceTable  = (struct Mem_PDEPresenceTable*)MEM_KERNEL_PDE_PRESENCE_ADDRESS;
struct Mem_PDEPresenceTable *Mem_KernelPDEFreeTable      = (struct Mem_PDEPresenceTable*)MEM_KERNEL_PDE_FREE_ADDRESS; 

struct Mem_PDEPresenceTable	// the structure used for the PDE presence and free tables
{
	uint64 entries[4096];
};

#define MEM_PML4_KERNEL_PDP_INDEX	0x00 // entry 0 of PML4 maps the 1:1 physical / kernel address space
#define MEM_PML4_USER_PDP_INDEX	    0x01 // entry 1 is set to the PDP for the current user process working set mapping the upper 512gb

struct Mem_PageTable
{
	uint64 entries[512];
};

#define MEM_TEMP_PDE_ADDRESS	0x00C000	// location of temporary pde table initialized by boot loader
#define MEM_KERNEL_PDE_ADDRESS  0x200000	// location of the permanent pde tables initialized by kernel

struct Mem_PageTable *Mem_RootPML4  = (struct Mem_PageTable*)MEM_PML4_ADDRESS;       // the one and only permanent PML4

struct Mem_PageTable *Mem_KernelPDP = (struct Mem_PageTable*)MEM_KERNEL_PDP_ADDRESS; // lower 512gb page table, 1:1 mapping used for kernel space
struct Mem_PageTable *Mem_KernelPDE = (struct Mem_PageTable*)MEM_KERNEL_PDE_ADDRESS;

struct Mem_PageTable *Mem_UserPDP   = (struct Mem_PageTable*)NULL;                   // points to the current user process PDP which maps the upper 512gb
																					 // of the 3, this is the only page table will change dynamically

void Mem_SetPML4(const struct Mem_PageTable *pml4);  // reset page tables by specifying the new address for the PML4 level page table

// structure of int 0x15 memory region table entry loaded in boot loader, starts with 4 bytes for number of entries
// followed by tables describing regions of physical memory available or reserved

#define MEM_REGIONTABLE_ADDRESS 0x8000

struct Mem_RegionTableEntry
{
	uint64 baseAddress;
	uint64 regionLength;
	uint32 regionType;
	uint32 extAttributes;
};

uint32 Mem_numRegionEntries                  = 0;												 
struct Mem_RegionTableEntry *Mem_regionTable = (struct Mem_RegionTableEntry*)(MEM_REGIONTABLE_ADDRESS + 0x04);

// track memory usage consumption / capacity, these will always be in multiples of 2MB because that is our smallest page allocation

uint64 Mem_totalPhysicalMemory = 0;
uint64 Mem_freePhysicalMemory  = 0;

// *******************************

void Mem_Init()
{
	Con_Print("Initializing memory subsystem...\n");

	// setup 2MB @ 0x200000 - where we will keep our physical pde tables, it'd be nice to just do this in the boot loader
	// but I'm basically out of code space in that 512 bytes :<

	uint64 *tempPde = (uint64*)MEM_TEMP_PDE_ADDRESS; 

	tempPde[1] = MEM_KERNEL_PDE_ADDRESS | MEM_PTF_PRESENT | MEM_PTF_WRITEABLE | MEM_PTF_WRITETHROUGH |
		                                                    MEM_PTF_GLOBALPAGE | MEM_PTF_PAGESIZE;// |  MEM_PTF_NOEXECUTE;

	// setup presence / free bit fields

	Mem_Set(Mem_KernelPDEPresenceTable, 0, sizeof(struct Mem_PDEPresenceTable));
	Mem_Set(Mem_KernelPDEFreeTable,     0, sizeof(struct Mem_PDEPresenceTable));

	// grab full memory map initialized by the boot loader

	uint32 *regionEntriesPtr = (uint32*)MEM_REGIONTABLE_ADDRESS;
	Mem_numRegionEntries = *regionEntriesPtr;

	Con_Print("INT 0x15 Memory Region Table @ 0x8000: %i4 entries\n", Mem_numRegionEntries);

	for (uint32 i = 0; i < Mem_numRegionEntries; i++)
	{
		Con_Print("Base: %h8 Length: %h8 Type: %h4\n", Mem_regionTable[i].baseAddress, Mem_regionTable[i].regionLength, Mem_regionTable[i].regionType);

		if (Mem_regionTable[i].baseAddress & 0x1FFFFF) // does not start on 2MB boundary
		{
			uint64 baseAddress = ((Mem_regionTable[i].baseAddress >> 21) << 21); // round down to nearest 2MB

			if (Mem_regionTable[i].regionType == 1)	// free 
			{
				baseAddress  += 0x200000;	// if the memory is a 'free' region we need to be careful and round up instead
			}

			uint64 regionLength = (Mem_regionTable[i].baseAddress + Mem_regionTable[i].regionLength) - baseAddress; // region length changed

			Mem_regionTable[i].baseAddress  = baseAddress;
			Mem_regionTable[i].regionLength = regionLength;
		}
		
		if (Mem_regionTable[i].regionLength & 0x1FFFFF) // does not end on 2MB boundary
		{
			Mem_regionTable[i].regionLength = (Mem_regionTable[i].regionLength >> 21) << 21; // round length down to nearest 2MB

			if (Mem_regionTable[i].regionType != 1) // reserved / completely unavailable
			{
				Mem_regionTable[i].regionLength += 0x200000;	// be careful and round up if region is reserved
			}
		}

		if ((Mem_regionTable[i].baseAddress + Mem_regionTable[i].regionLength) > 0x8000000000)
		{
			Mem_regionTable[i].regionLength = 0x8000000000 - Mem_regionTable[i].baseAddress; // 512gb max supported memory, chop region off if it exceeds 512gb
		}

		if ((Mem_regionTable[i].baseAddress + Mem_regionTable[i].regionLength) < 0x400000) // the lowest 4MB is reserved and manually mapped anyway, ignore this region
		{
			Mem_regionTable[i].regionType   = 0;

			continue;	
		}
		
		if (Mem_regionTable[i].baseAddress >= 0x8000000000) // 512gb max supported memory
		{
			Mem_regionTable[i].regionType   = 0;

			continue;
		}
		
		if (!Mem_regionTable[i].regionLength)	// it's possible due to boundaries the region now has 0 length
		{
			Mem_regionTable[i].regionType   = 0;

			continue;
		}

		Con_Print("MAPPING Base: %h8 Length: %h8 Type: %h4\n", Mem_regionTable[i].baseAddress, Mem_regionTable[i].regionLength, Mem_regionTable[i].regionType);

		for (uint64 page = (Mem_regionTable[i].baseAddress >> 21); page < ((Mem_regionTable[i].baseAddress + Mem_regionTable[i].regionLength) >> 21); page++)
		{
			if ((Mem_regionTable[i].regionType == 1) || (Mem_regionTable[i].regionType == 3))	// free / reclaimable
			{
				Mem_KernelPDEPresenceTable->entries[(page >> 6)] |= ((uint64)1 << (page & 0x3F));

				if (Mem_regionTable[i].regionType == 1)	// entirely free
				{
					Mem_KernelPDEFreeTable->entries[(page >> 6)] |= ((uint64)1 << (page & 0x3F));
				}
			}
		}
	}

	// this loop allows reserved regions to overlap free regions and reserved regions to take precedence

	for (uint32 i = 0; i < Mem_numRegionEntries; i++)
	{
		if (Mem_regionTable[i].regionType)	// we set invalid regions to 0 in the last loop
		{
			for (uint64 page = (Mem_regionTable[i].baseAddress >> 21); page < ((Mem_regionTable[i].baseAddress + Mem_regionTable[i].regionLength) >> 21); page++)
			{
				if ((Mem_regionTable[i].regionType != 1) && (Mem_regionTable[i].regionType != 3)) // reserved / completely unavailable
				{
					if (Mem_KernelPDEPresenceTable->entries[(page >> 6)] & ((uint64)1 << (page & 0x3F))) // apparently overlapped with a region that marked this as usable
					{
						Mem_KernelPDEPresenceTable->entries[(page >> 6)] ^= ((uint64)1 << (page & 0x3F)); // mark it as not usable
					}
				}

				if (Mem_regionTable[i].regionType == 3)	// reclaimable but currently in use
				{
					if (Mem_KernelPDEFreeTable->entries[(page >> 6)] & ((uint64)1 << (page & 0x3F)))	// was marked as entirely free
					{
						Mem_KernelPDEFreeTable->entries[(page >> 6)] ^= ((uint64)1 << (page & 0x3F));	// mark it as not free
					}
				}
			}
		}
	}
	
	Mem_KernelPDEPresenceTable->entries[0] |= 0x0000000000000003; // the lowest 4MB must exist to even get this far
	Mem_KernelPDEFreeTable->entries[0]     &= 0xFFFFFFFFFFFFFFFC; // lowest 4MB is not available

	Con_Print("MAPPING Base: 0x0000000000000000 Length: 0x0000000000400000 Type: 3\n");

	// setup pml4 and our physical / kernel and user PDPs in our page table space

	Mem_Set(Mem_RootPML4, 0, sizeof(struct Mem_PageTable));
	
	Mem_RootPML4->entries[MEM_PML4_KERNEL_PDP_INDEX] = (uint64)Mem_KernelPDP | MEM_PTF_PRESENT | MEM_PTF_WRITEABLE | MEM_PTF_WRITETHROUGH;
	Mem_RootPML4->entries[MEM_PML4_USER_PDP_INDEX]   = NULL;	// initially the kernel doesn't have a user address space working set

	for (uint64 i = 0; i < 512; i++)
	{
		Mem_KernelPDP->entries[i] = (MEM_KERNEL_PDE_ADDRESS + (i << 12)) | MEM_PTF_PRESENT | MEM_PTF_WRITEABLE | MEM_PTF_WRITETHROUGH;
	}

	Mem_Set(Mem_KernelPDE, 0, sizeof(struct Mem_PageTable) * 512);

	// now that the presence / free tables are complete tally up the memory and map the actual pages

	for (uint64 page = 0; page < (512 * 512); page++)
	{
		if (Mem_KernelPDEPresenceTable->entries[(page >> 6)] & ((uint64)1 << (page & 0x3F))) // 2MB page exists
		{
			Mem_totalPhysicalMemory += 0x200000;
		}

		if (Mem_KernelPDEFreeTable->entries[(page >> 6)] & ((uint64)1 << (page & 0x3F)))     // 2MB page is free
		{
			Mem_freePhysicalMemory += 0x200000;
		}
	}
	
	// manually map lowest 4MB for kernel stuff

	Mem_KernelPDE[0].entries[0] = 0x000000 | MEM_PTF_PRESENT | MEM_PTF_WRITEABLE | MEM_PTF_WRITETHROUGH | MEM_PTF_PAGESIZE;
	Mem_KernelPDE[0].entries[1] = 0x200000 | MEM_PTF_PRESENT | MEM_PTF_WRITEABLE | MEM_PTF_WRITETHROUGH | MEM_PTF_PAGESIZE;

	Mem_SetPML4(Mem_RootPML4); // and enable our new memory environment, whoopee!

	Con_Print("Total usable physical memory: %i8MB\n", Mem_totalPhysicalMemory >> 20);
	Con_Print("Available physical memory:    %i8MB\n", Mem_freePhysicalMemory >> 20);

	if (Mem_freePhysicalMemory < MEM_MIN_REQ_MEMORY)
	{
		Krn_Panic("Error: Not enough memory, 32MB required");
	}
}

void Mem_SetPML4(const struct Mem_PageTable *pml4)
{
	__asm__("mov rdx, %0  \n"
		    "mov cr3, rdx\n"
			:
			: "g"(pml4)
			: "rdx"        );
}


// *******************************

void *Mem_PhysAlloc(void *baseAddress, uint64 size, uint8 flags)
{
	uint64 ptfFlags = MEM_PTF_PRESENT | MEM_PTF_GLOBALPAGE | MEM_PTF_PAGESIZE | (uint64)flags;

	if (!baseAddress)
	{
		// search for region of sufficient size
		//Mem_KernelPDEFreeTable

		// if none found return null
	}
	else
	{
		// map specific page range, if pages are not present or are already in use return null

		//Mem_KernelPDE[page >> 9].entries[page & 0x1FF] = (page << 21) | ptfFlags;
	}
}

uint64 Mem_PhysFree(void *baseAddress, uint64 size)
{

}
uint64 Mem_GetPhysicalMemorySize()
{
	return Mem_totalPhysicalMemory;
}

uint64 Mem_GetAvailableMemorySize()
{
	return Mem_freePhysicalMemory;
}

void Mem_Copy(void *dest, const void *source, uint64 num)
{
	uint8 *destPtr = (uint8*)dest;
	uint8 *srcPtr  = (uint8*)source;

	while (num)
	{
		*destPtr = *srcPtr;

		srcPtr++;
		destPtr++;

		num--;
	}
}


void Mem_Set(void *dest, uint8 val, uint64 num) 
{
	uint8 *destPtr = (uint8*)dest;

	while (num)
	{
		*destPtr = val;

		destPtr++;
		num--;
	}
}