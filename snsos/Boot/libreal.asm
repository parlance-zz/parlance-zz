GetChar proc   ; char returned in al

	mov ah, 00h
	mov al, 00h
	int 16h
	
	ret
	
GetChar endp

GetString proc ; string written to es:di
	
	cld
	
GetStringStart:

	call GetChar
	cmp al, 13
	je GetStringFinish
	push ax
	call WriteChar
	pop ax
	stosb		
	jmp GetStringStart
	
GetStringFinish:

	mov al, 13
	stosb
	xor al, al
	stosb
	
	ret
	
GetString endp

WriteHex proc ; val in al

	push ax
	shr al, 04h
	
	cmp al, 10
	jge WriteHexLetter1
	add al, 30h
	call WriteChar
	jmp WriteHexLetter2
	
WriteHexLetter1:

	add al, 37h
	call WriteChar
	
WriteHexLetter2:

	pop ax
	and al, 0Fh
	
	cmp al, 10
	jge WriteHexLetter3
	add al, 30h
	call WriteChar
	ret
	
WriteHexLetter3:

	add al, 37h
	call WriteChar
	
	ret

WriteHex endp

WriteString proc ; string is in ds:si

	cld
	
WriteStringStart:

	lodsb
	test al, al
	jz WriteStringFinish
	call WriteChar
	jmp WriteStringStart
	
WriteStringFinish:

	ret

WriteString endp

WriteChar proc ; char passed in al

	cmp al, 13  ; carriage return ascii
	je WriteCharNewLine
		
	mov ah, 0ah ; write char function
	mov bh, 00h ; display page
	mov bl, 00h ; color
	mov cx, 01h ; reps
	
	int 10h		; video function

	mov ah, 03h ; get cursor position function
	int 10h

	inc dl		; advance cursor

	cmp dl, 80	; screen width
	jl WriteCharMoveFinish
	jmp WriteCharScroll
	
WriteCharNewLine:

	mov ah, 03h ; get cursor position function
	int 10h

WriteCharScroll:

	mov dl, 00h ; wrap
	
	inc dh      ; new line
	cmp dh, 25  ; screen height
	jl WriteCharMoveFinish

	mov dh, 24  ; stay at bottom
	
	mov ah, 02h
	int 10h
		
	; and scroll
	mov ah, 06h ; scroll down function
	mov al, 01h ; num lines
	mov bh, 07h ; color?
	mov ch, 00h ; upper line
	mov cl, 00h ; left col
	mov dh, 18h ; lower line
	mov dl, 4Fh ; right col
	mov dx, 184Fh ; ??
	int 10h
	
	jmp WriteCharFinish
	
WriteCharMoveFinish:

	mov ah, 02h
	int 10h

WriteCharFinish:

	ret

WriteChar endp