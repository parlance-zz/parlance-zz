; searches a GPT formatted disk for the SNSOS system partition and loads it into 0x100000, initializes long mode
; sets stack to 0x200000

org 0x10000	; MBR loads us into 0x10000

start:

	; clear segment registers just in case they weren't clear for some reason, they damn well ought to be
	
	mov ax, 0x1000
	mov ds, ax
	mov es, ax
	
	xor ax, ax
	mov ss, ax
	mov sp, 0x8000	; setup some stack space below the program
	
	cld	; clear direction flag
	sti	; and enable interrupts

	; clear screen
	
	mov ah, 0x06	; magics
	mov bh, 0x07
	xor cx, cx
	mov dx, 0x184F
	int 0x10		; bios video function

	; enable the a20 address line
	
	mov ax, 0x2401
	int 0x15
	jc error
	
	; check for long mode x64
	
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb error
    
lbaCheck: ; check for LBA function supported by BIOS
    
	mov ah, 0x41	; int13 ext check
	mov bx, 0x55AA	; test pattern
	mov dl, 0x80	; drive number (80 is first drive, or current drive, or something)
	int 0x13	    ; int13 is the BIOS disk service
	jb error	
	cmp bx, 0xAA55	; test pattern is reversed on success
	jnz error
	test cl, 0x01	; test for extended read/write
	jz  error
	
loadGPTHeader: ; load GPT header

	call loadSectors	; the correct info is already in the lbaPacket
				        ; so this will load the 1 sector GPT header into 0x8000

	lds si,[gptHeader]	; check header for validity
	les di,[es:0x8000]
	call checkGUID
	jne  error
	
	mov eax, [ds:0x8084] ; offset for size of partition entry
	cmp eax, 0x80		 ; has to be 128
	jne error

	lds si, [ds:0x8072]	 ; load the starting partition entry LBA into our LBA packet
	les di, [lbaStartLBALow]
	mov cx, 0x08
	rep movsb
	
	mov ecx, [ds:0x8080] ; load number of partition entries
	
	; todo: check CRC as well, and then go to tail copy of header if it isn't right
	; check CRC of partition entries also
	
scanPartitions: ; scans the partition tables for boot partition

	call loadSectors
	xor ax, ax
		
scanPartition:

	lds si, [gptSystemSignature] ; we will be searching for this signature
	mov di, ax
	shl di, 0x07	; multiply by 128
	add di, 0x8000  ; location for this entry	
	call checkGUID
	je   loadSystemPartition
	dec ecx
	inc ax
	cmp ax, 0x04	   ; (512 / 128)
	jne  scanPartition
	
	add dword [lbaStartLBALow], 0x01
	adc dword [lbaStartLBAHigh], 0x00
	
	test ecx, ecx
	jnz  scanPartitions ; keep going until we find it

notFound: ; we'll only get here after we scan each partition entry
	
	jmp error

loadSystemPartition:

	mov di, ax
	shl di, 0x07	; multiply by 128
	add di, 0x8032  ; location for this entries first LBA
	
	mov si, di
	lodsd
	mov eax, edx
	lodsd
	mov eax, ecx
	lodsd
	mov eax, ebx
	lodsd

	; start low is in edx
	; start high is in ecx
	; end low is in ebx
	; end high is in eax
	
	mov [lbaStartLBALow], edx
	mov [lbaStartLBAHigh], ecx
	
	sub ebx, edx
	sbb eax, ecx
	test eax, eax
	jnz error		; huge kernel partition, over 2TB

initUnrealMode:	    ; initialize protected mode but don't reload cs to stay in real mode, reload ds to enable 32 bit addressing
	                ; 'unreal mode'
	cli				; clear interrupts for transition
	
	push ds			; remember our data segment
	push es			; and our extra segment
	push ss			; and our stack segment
	
	lgdt [pgdt]	    ; load the temporary flat 32bit gdt
	
	mov eax, cr0	; enable protected mode
	or  eax, 1
	mov cr0, eax
	
	mov dx, 0x08	; reload ds and es registers to the flat full address space descriptor
	mov ds, dx
	mov es, dx
	mov ss, dx
	
	and al, 0xFE	; and go back to real mode
	mov cr0, eax
	
	pop ds			; my understanding says this shouldn't be here, since the flat descriptor needs to stay cached
					; and reloading ought to reset that
	pop es
	pop ss
	
	sti				; re-enable interrupts again, hopefully the BIOS vectors will work in 'unreal mode'
	
	les edi, [0x100000]
	
loadMoreSectors:

	call loadSectors
	
	lds esi, [ds:0x8000]	; copy 512 bytes at 0x8000 to the next 512 bytes above 0x100000
	mov ecx, 0x200
	rep movsb
	
	add dword [lbaStartLBALow],  1
	adc dword [lbaStartLBAHigh], 0
	
	dec ebx
	jnz loadMoreSectors

getMemoryMap:    ; get memory map (!)
    
    les edi, [es:0x8004] ; table starts at 0x8004
    xor ebx, ebx
    mov esi, ebx
    mov edx, 0x534D4150	  ; magic for memory map function
    
nextMapEntry:    

	mov dword [es:edi + 20], 1
    mov eax, 0xe820
    mov ecx, 24
    int 0x15
    jc error
    inc esi				; esi is just counter the number of entries
    test ebx,ebx
    jz   mapDone
    add  edi, 24
    jmp nextMapEntry	; more memory map entries
	
mapDone:

	mov eax, esi
	mov dword [es:0x8000], eax	; store number of entries at 0x8000
	
initProtectedMode:
	
	cli	  	       ; disable interrupts
	
	; enable protected mode
	
	mov eax, cr0
	or  eax, 1
	mov cr0, eax
		
	jmp 0x08:protectedModeStart

; utility functions

loadSectors:
	
	lds si, [lbaPacket]
	mov ah, 0x42		; ATA ext load LBA function
	mov dl, 0x80		; drive number (starting at 0x80)
	int 0x13		; bios disk service function
	jc error		; if there was any error loading the GPT header
	ret

checkGUID: ; 16 byte scan and compare
	
	mov cx, 0x10
	rep cmpsb		; check the first 16 bytes matches our stored copy
	ret
	
error:  ; print a basic error message
	
	mov ax, 0x1000	; this is because I'm not sure about the implicit segment prefix for BP where our string data is
	mov ds, ax
	mov es, ax
	mov ss, ax
	
	mov ah, 0x13	; write text string
	mov al, 0x01	; moving cursor
	xor bh, bh	    ; 0 page
	mov bl, 0x07    ; 7 = white
	mov cx, [errorMsgLength]
	les bp, [errorMsgString]
	xor dx, dx
	int 0x10
	
spin: jmp spin	; after an error just spin endlessly waiting for ctrl-alt-del

; data

; error messages

errorMsgLength dw 27
errorMsgString db "Error loading system kernel"

; first part of gpt header structure

gptHeader   db 0x45,0x46,0x49,0x20,0x50,0x41,0x52,0x54	; 'EFI PART'
            db 0x00,0x00,0x01,0x00			; version 1.0
            db 0x5C,0x00,0x00,0x00			; header size, should be 92 bytes

		              ; UUID for an SNSOS system partition
gptSystemSignature db 0x01,0x0C,0x4B,0x2F,0x0D,0x7E,0xF3,0x8F,0x1D,0x9C,0x2D,0x7A,0x93,0xA1,0xE2,0xAF

;gptDataSignature   db 0x04,0x37,0x47,0x0C,0xB4,0x86,0xCC,0x2A,0x75,0x69,0x10,0x32,0x14,0x48,0xA5,0x27

; lba packet

lbaPacket   db 0x10   ; size of packet (always 16)
            db 0x00   ; always 0
lbaSectors  dw 0x01   ; number of sectors to transfer
lbaOffset   dw 0x8000 ; offset to transfer to / from
lbaSegment  dw 0x1000 ; segment to transfer to / from
lbaStartLBALow  dd 0x01   ; first lba index in transfer
lbaStartLBAHigh dd 0x00   ; first lba (high part, only 16 bits used however)  

; global descriptor table for protected mode memory map

pgdt	  dw pgdtEnd - pgdtStart - 1 ; size
          dd pgdtStart               ; address

pgdtStart dd 0  ; null descriptor
          dd 0

		  ; code descriptor
		  
		  dw 0xFFFF ; limit 0:15
		  dw 0      ; base 0:15
		  db 0      ; base 16:23
		  db 0x9A   ; access byte
		  db 0xCF   ; flags, limit 16:19 0CFh
		  db 0      ; base 24:31
		  
		  ; data descriptor
		  
		  dw 0xFFFF ; limit 0:15
		  dw 0      ; base 0:15
		  db 0      ; base 16:23
		  db 0x92   ; access byte
		  db 0xCF   ; flags, limit 16:19 0CFh
		  db 0      ; base 24:31
		 
pgdtEnd   db 0

; begin 32-bit protected mode code

[bits 32]
protectedModeStart:
	
	; setup all non-code segments to use descriptor 10h
		
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov ss, ax
	
	; init long mode
	
	; build page tables
	; the page tables will look like this:
	; PML4:
	; dq 0x000000000000b00b = 00000000 00000000 00000000 00000000 00000000 00000000 10010000 00001111
	; times 511 dq 0x0000000000000000

	; PDP:
	; dq 0x000000000000c00b = 00000000 00000000 00000000 00000000 00000000 00000000 10100000 00001111
	; times 511 dq 0x0000000000000000

	; PD:
	; dq 0x000000000000018b = 00000000 00000000 00000000 00000000 00000000 00000000 00000001 10001111
	; times 511 dq 0x0000000000000000

	; this defines one 2MB page at the start of memory, so we can access the first 2MBs as if paging was disabled

	cld
	mov edi, 0xa000

	mov ax, 0xb00b
	stosw

	xor ax, ax
	mov ecx, 0x07ff
	rep stosw

	mov ax,0xc00b
	stosw

	xor ax,ax
	mov ecx,0x07ff
	rep stosw

	mov ax,0x018b
	stosw

	xor ax,ax
	mov ecx,0x07ff
	rep stosw

	; switch to long mode
	
	sti								; interrupts have to be enabled to enter x64 for some reason
	
	mov eax,10100000b				; set PAE and PGE
	mov cr4,eax

	mov edx, 0x0000a000				; point CR3 at PML4
	mov cr3,edx

	mov ecx,0xC0000080				; specify EFER MSR

	rdmsr						; enable Long Mode
	or eax,0x00000100
	wrmsr
	
	mov ebx,cr0					; activate long mode
	or ebx,0x80000000			; by enabling paging
	mov cr0,ebx
	
	lgdt [gdt.pointer]			; load new 80-bit gdt.pointer below

	jmp 0x08:longModeStart  	; load CS with 64 bit segment and flush the instruction cache

; begin 64-bit long mode code

; global descriptor table for long mode (flat, no length specified)

gdt:
dq 0x0000000000000000				; null Descriptor

.code equ $ - gdt
dq 0x0020980000000000                   

.data equ $ - gdt
dq 0x0000900000000000                   

.pointer:
dw $-gdt-1					; 16-bit structure size (limit)
dq gdt						; 64-bit structure address

[bits 64]
longModeStart:
	
	cli					; clear interrupts because we're fucked until we setup an IDT if one occurs
	
	mov rsp, 0x200000	; setup stack, todo: find better address
	
	jmp 0x100000	     ; transfer control to the kernel

times 65536-($-$$)  db 0       ; 0 padding to 64kb
