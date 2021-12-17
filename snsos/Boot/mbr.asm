; searches a GPT formatted disk for a BIOS boot partition and loads it into 0x10000
; sets stack to 0x8000

org 0x7C00	; BIOS loads MBR into 0x7C00

start:

	; clear segment registers just in case they weren't clear for some reason, they damn well ought to be
	
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	
	mov sp, 0x7C00	; setup some stack space below the program
	
	cld	; clear direction flag
	sti	; and enable interrupts

	; clear screen
	
	mov ah, 0x06	; buncha magic
	mov bh, 0x07
	xor cx, cx
	mov dx, 0x184F
	int 0x10		; bios video function

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
	les di,[0x8000]
	call checkGUID
	jne  error

	mov eax, [0x8084]	; offset for size of partition entry
	cmp eax, 0x80		; has to be 128
	jne error

	lds si, [0x8072]	; load the starting partition entry LBA into our LBA packet
	les di, [lbaStartLBALow]
	mov cx, 0x08
	rep movsb

	mov ecx, [0x8080] ; load number of partition entries
	
	; todo: check CRC as well, and then go to tail copy of header if it isn't right
	; check CRC of partition entries also
	
scanPartitions: ; scans the partition tables for boot partition

	call loadSectors
	xor ax, ax
	
scanPartition:

	lds si, [gptBootSignature] ; we will be searching for this signature
	mov di, ax
	shl di, 0x07	; multiply by 128
	add di, 0x8000  ; location for this entry
	call checkGUID
	je   loadBootPartition
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

loadBootPartition:

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
	jnz error		; huge boot partition, over 2TB
	
	cmp ebx, 0x100
	jg error		; boot partition max 128kb in size
	
	mov sp, 0x8000	; stack is set to 0x0x00008000
	
	mov ax, 0x1000			; load into 0x10000
	mov [lbaSegment], ax
	xor ax, ax
	mov [lbaOffset], ax
	
	call loadSectors	; load the boot partition into 0x10000
	
	jmp 0x1000:0000		; and transfer control with a far call to the new code segment

loadSectors:

	mov si, lbaPacket	; address of lba packet
	mov ah, 0x42		; ATA ext load LBA function
	mov dl, 0x80		; drive number (starting at 0x80)
	int 0x13		; bios disk service function
	jc error		; if there was any error loading the GPT header
	ret

checkGUID: ; 16 byte scan and compare
	
	mov cx, 0x10
	repz cmpsb		; check the first 16 bytes matches our stored copy
	ret
	
error:  ; print a basic error message
	
	mov ah, 0x13	; write text string
	mov al, 0x01	; moving cursor
	xor bh, bh	    ; 0 page
	mov bl, 0x07    ; 7 = white
	mov cx, [errorMsgLength]
	mov bp, errorMsgString
	xor dx, dx
	int 0x10
	
spin: jmp spin	; after an error just spin endlessly waiting for ctrl-alt-del
	
; data

; error messages

errorMsgLength dw 30
errorMsgString db "Error loading operating system"

; first part of gpt header structure

gptHeader   db 0x45,0x46,0x49,0x20,0x50,0x41,0x52,0x54	; 'EFI PART'
            db 0x00,0x00,0x01,0x00			; version 1.0
            db 0x5C,0x00,0x00,0x00			; header size, should be 92 bytes

		    ; magic BIOS boot signature GUID
gptBootSignature db 0x21,0x68,0x61,0x48,0x64,0x49,0x6E,0x6F,0x74,0x4E,0x65,0x65,0x64,0x45,0x46,0x49

; lba packet

lbaPacket   db 0x10   ; size of packet (always 16)
            db 0x00   ; always 0
lbaSectors  dw 0x01   ; number of sectors to transfer
lbaOffset   dw 0x8000 ; offset to transfer to / from
lbaSegment  dw 0x0000 ; segment to transfer to / from
lbaStartLBALow  dd 0x01   ; first lba index in transfer
lbaStartLBAHigh dd 0x00   ; first lba (high part, only 16 bits used however)  

; sets up the standard MBR partition table for a GPT disk

times 440-($-$$)  db 0	     ; nulls up to the partition table

diskSignature	  dw 0	     ; optional, set to 0
diskReserved      db 0,0     ; unused, set to 0

db 0			    ; disk status (0x80 = bootable)
db 0x00, 0x00, 0x02	; CHS (0, 0, 2) is the first sector in the cover partition (LBA 1, 2nd sector on the disk)
db 0xEE			    ; partition type, GPT standard specifies 0xEE for the cover partition type
db 0xFE, 0xFF, 0xFF	; max CHS value for largest cover possible
dd 0x01			    ; first LBA in partition, 2nd sector on the disk
dd 0xFFFFFFFF	    ; last LBA in partition, max for largest cover possible

times 510-($-$$)  db 0       ; 0 padding to magic signature
                  dw 0x0AA55 ; magic mbr signature 
