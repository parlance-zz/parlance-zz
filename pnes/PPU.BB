;TO DO

;2) $2005/$2006/$2007/$2002 ppu addressing magic (scrolling!)
;3) sort out additional timing issues
;4) speed up scanline renderer

;internals

Global rom_mirroring_mode

Const HORIZONTAL_MIRRORING	= 0
Const VERTICAL_MIRRORING	= 1
Const FOUR_SCREEN_MIRRORING	= 2

Global last_vram_address,vram_address,load_low
Global spr_address

Global scroll_x,scroll_y,ppu_regs

Dim palette($FF+1)

;let;s load our pallete shall we?

pal_file=ReadFile("pnes.pal")

If Not pal_file

	RuntimeError "Could not open pnes.pal. Can;t run without a palette!"
	
Else

	For index=$0 To $FF
	
		red=ReadByte(pal_file)
		green=ReadByte(pal_file)
		blue=ReadByte(pal_file)
		
		palette(index)=(red Shl 16) Or (green Shl 8) Or blue
		
	Next
	
	CloseFile pal_file
	
EndIf

;functions

Function run_frame()
	
	If fullscreen
	
		SetBuffer BackBuffer()
		
	Else
	
		SetBuffer CanvasBuffer(main_canvas)
		
	EndIf
	
	PokeByte ram,$2002,(PeekByte(ram,$2002) And $7F) ;we;re no longer in vblank
	
	bg_color=palette(PeekByte(vram,$3F00))
	
	ClsColor (bg_color And $FF0000) Shr 16,(bg_color And $00FF00) Shr 8,(bg_color And $0000FF)
	Cls
	
	;draw bg sprites underneath map
	;although technicly sprites are rendered simultaneously with the BG per scanline, it;s just too slow
	
	LockBuffer
	
	If (PeekByte(ram,$2001) And $10) Then draw_sprites(1)

	UnlockBuffer
		
	;render the bg and simultaneously execute the cpu
	
	cycles=0
	
	For scan_line=scroll_y To scroll_y+240-1;0 To 240-1
		
		If scan_line = PeekByte(spr_ram,0) Then PokeByte ram, $2002, (PeekByte(ram,$2002) Or $40) ;sprite 0 hit
		
		cycles=cycles+114
		cpu_execute()
	
		;draw bg ScanLine
		
		If PeekByte(ram,$2001) And $8
		
			LockBuffer
	
			;image_mask=1-(PeekByte(ram,$2001) and $2) Shr 1  ;THIS isn;t working properly
			;it;s supposed to clip the left 8 columns or something if enabled
					
			For pixel_x=scroll_x To scroll_x+256-1;0 to 256-1
	
				dx=pixel_x/8		
				dy=scan_line/8
				
				px=pixel_x Mod 8
				py=scan_line Mod 8
				
				name_table=PeekByte(ram,$2000) And $3
				
				Select name_table
				
					Case 0
					
						base_address=read_vram($2000+dy*32+dx)*16
						attributes=read_vram($23C0+(dy/4)*8+dx/4)
						
					Case 1
					
						base_address=read_vram($2400+dy*32+dx)*16
						attributes=read_vram($27C0+(dy/4)*8+dx/4)
						
					Case 2
					
						base_address=read_vram($2800+dy*32+dx)*16
						attributes=read_vram($2BC0+(dy/4)*8+dx/4)
						
					Case 3
					
						base_address=read_vram($2C00+dy*32+dx)*16
						attributes=read_vram($2FC0+(dy/4)*8+dx/4)
						
				End Select
	
				If PeekByte(ram,$2000) And $10 Then base_address=base_address+$1000
				
				tile_index=dx Mod 4+(dy Mod 4)*4
				
				Select tile_index
				
					Case 0,1,4,5
					
						base_color_index=(attributes And $3) Shl 2
						
					Case 2,3,6,7
					
						base_color_index=attributes And $0C
						
					Case 8,9,12,13
					
						base_color_index=(attributes And $30) Shr 2
						
					Case 10,11,14,15
					
						base_color_index=(attributes And $C0) Shr 4
						
				End Select
				
				color_index=((PeekByte(vram,base_address+py) And ($80 Shr px)) Or ((PeekByte(vram,base_address+py+8) And ($80 Shr px)) Shl 1 )) Shr (7-px)
				;If (color_index or base_color_index) Then WritePixelFast pixel_x,scan_line,palette(PeekByte(vram,$3F00+(color_index or base_color_index)))
				If (color_index Or base_color_index) Then WritePixelFast pixel_x-scroll_x,scan_line-scroll_y,palette(PeekByte(vram,$3F00+(color_index Or base_color_index)))
		
			Next
			
			UnlockBuffer
			
		EndIf
			
	Next
	
	;and now the fg sprites
	
	LockBuffer
		
	If (PeekByte(ram,$2001) And $10) Then draw_sprites(0)
	
	UnlockBuffer
		
	;and refresh
	
	If fullscreen
	
		Flip
		
	Else
	
		FlipCanvas main_canvas
		
	EndIf
	
	;vblank
	PokeByte ram,$2002,(PeekByte(ram,$2002) Or $80)
	If (PeekByte(ram,$2000) And $80) Then nmi_6502()
	
	cycles=2500
	cpu_execute()
	
End Function

Function write_vram(address,num)
	
	If address>$4000 Then address=address-$4000
	
	If address<$2000
	
		PokeByte vram,address,num
		
	Else If address<$3F00
		
		If address>$3000 Then address=address-$1000
		
		name_table=(address-$2000)/$400
		offset=(address-$2000) Mod $400
		
		Select rom_mirroring_mode
		
			Case HORIZONTAL_MIRRORING
			
				Select name_table
				
					Case 0,1
					
						PokeByte vram,$2000 Or offset,num
						
					Case 2,3
					
						PokeByte vram,$2400 Or offset,num
						
				End Select
				
			Case VERTICAL_MIRRORING
			
				Select name_table
				
					Case 0,2
					
						PokeByte vram,$2000 Or offset,num
						
					Case 1,3
					
						PokeByte vram,$2400 Or offset,num
						
				End Select
			
			Case FOUR_SCREEN_MIRRORING
			
				PokeByte vram,address,num
				
		End Select
		
	Else If address<$4000
	
		address=$3F00+((address-$3F00) Mod $20)
		
		PokeByte vram,address,num
		
	EndIf
	
End Function

Function read_vram(address)

	If address>$4000 Then address=address-$4000
	
	If address<$2000
	
		Return PeekByte(vram,address)
		
	Else If address<$3F00
		
		If address>$3000 Then address=address-$1000
		
		name_table=(address-$2000)/$400
		offset=(address-$2000) Mod $400
		
		Select rom_mirroring_mode
		
			Case HORIZONTAL_MIRRORING
			
				Select name_table
				
					Case 0,1
					
						Return PeekByte(vram,$2000 Or offset)
						
					Case 2,3
					
						Return PeekByte(vram,$2400 Or offset)
						
				End Select
				
			Case VERTICAL_MIRRORING
			
				Select name_table
				
					Case 0,2
					
						Return PeekByte(vram,$2000 Or offset)
						
					Case 1,3
					
						Return PeekByte(vram,$2400 Or offset)
						
				End Select
			
			Case FOUR_SCREEN_MIRRORING
			
				Return PeekByte(vram,address)
				
		End Select
		
	Else If address<$4000
	
		If address>$3F20 Then address=address-$20
		If address>$3F10 Then address=address-$10
		
		Return PeekByte(vram,address)
		
	EndIf
	
End Function

Function old_core()
	
	cycles=26500
	cpu_execute()
		
	If fullscreen
	
		SetBuffer BackBuffer()
		
	Else
	
		SetBuffer CanvasBuffer(main_canvas)
		
	EndIf
						
	LockBuffer
	
	If (PeekByte(ram,$2001) And $10) Then draw_sprites(1)
		
	;draw bg
	
	If PeekByte(ram,$2001) And $8
	
		image_mask=1-(PeekByte(ram,$2001) And $2) Shr 1  ;THIS isn;t working properly
						
		For dx=image_mask*4 To 32-1
		
			For dy=0 To 30-1
				
				name_table=PeekByte(ram,$2000) And $3
				
				Select name_table
				
					Case 0
					
						base_address=read_vram($2000+dy*32+dx)*16
						attributes=read_vram($23C0+(dy/4)*8+dx/4)
						
					Case 1
					
						base_address=read_vram($2400+dy*32+dx)*16
						attributes=read_vram($27C0+(dy/4)*8+dx/4)
						
					Case 2
					
						base_address=read_vram($2800+dy*32+dx)*16
						attributes=read_vram($2BC0+(dy/4)*8+dx/4)
						
					Case 3
					
						base_address=read_vram($2C00+dy*32+dx)*16
						attributes=read_vram($2FC0+(dy/4)*8+dx/4)
						
				End Select
				
				If PeekByte(ram,$2000) And $10 Then base_address=base_address+$1000
				
				tile_index=dx Mod 4+(dy Mod 4)*4
				
				Select tile_index
				
					Case 0,1,4,5
					
						base_color_index=(attributes And $3) Shl 2
						
					Case 2,3,6,7
					
						base_color_index=attributes And $0C
						
					Case 8,9,12,13
					
						base_color_index=(attributes And $30) Shr 2
						
					Case 10,11,14,15
					
						base_color_index=(attributes And $C0) Shr 4
						
				End Select
				
				draw_tile(base_address,base_color_index,dx*8,dy*8)
				
			Next
			
		Next
	
	EndIf
	
	If (PeekByte(ram,$2001) And $10) Then draw_sprites(0)
	
	UnlockBuffer
	
	If fullscreen

		Flip
			
	Else
		
		FlipCanvas main_canvas
			
	EndIf
						
End Function

Function draw_tile(base_address,base_color_index,pos_x,pos_y)
	
	pos_x=pos_x-scroll_x
	pos_y=pos_y-scroll_y
	
	If pos_x<-8 Or pos_y<-8 Or pos_x>256 Or pos_y>240
	
		Return
		
	EndIf
	
	sprite_mask=PeekByte(ram,$2001) And $4
	
	If sprite_mask And pos_y<64 Then Return
	
	If pos_x<0 Or pos_y<0 Or pos_x>256-8-1 Or pos_y>240-8-1
		
		For py=0 To 8-1
			
			For px=0 To 8-1
			
				color_index=((PeekByte(vram,base_address+py) And ($80 Shr px)) Or ((PeekByte(vram,base_address+py+8) And ($80 Shr px)) Shl 1 ))  Shr (7-px)
				If (color_index Or base_color_index) Then WritePixel pos_x+px,pos_y+py,palette(PeekByte(vram,$3F00+(color_index Or base_color_index)))
				
			Next
		
		Next
		
	Else
		
		For py=0 To 8-1
			
			For px=0 To 8-1
			
				color_index=((PeekByte(vram,base_address+py) And ($80 Shr px)) Or ((PeekByte(vram,base_address+py+8) And ($80 Shr px)) Shl 1 )) Shr (7-px)
				If (color_index Or base_color_index) Then WritePixelFast pos_x+px,pos_y+py,palette(PeekByte(vram,$3F00+(color_index Or base_color_index)))
				
			Next
		
		Next
		
	EndIf
	
End Function

Function draw_sprites(layer)
	
	sprite_size=(PeekByte(ram,$2000) And $20) Shr 5
	
	For sprite=64-1 To 0 Step -1;sprite #0 has highest priority
	
		attributes=PeekByte(spr_ram,sprite*4+2)
		
		If (attributes And $20)=layer*$20
			
			spr_y=PeekByte(spr_ram,sprite*4)
			spr_x=PeekByte(spr_ram,sprite*4+3)
			
			;clipping
			
			safe_draw=0
			
			If spr_x<-8 Or spr_x>256-1 Or spr_y>240-1 Then Goto next_sprite
			
			If spr_x<0 Or spr_x>256-8-1 Or spr_y<0 Then safe_draw=1
			
			If sprite_size
			
				If spr_y<-16 Then Goto next_sprite
				If spr_y>256-16-1 Then safe_draw=1
					
			Else
			
				If spy_y<-8 Then Goto next_sprite
				If spr_y>256-8-1 Then safe_draw=1
				
			EndIf
		
			base_address=PeekByte(spr_ram,sprite*4+1)*16
			opposite_address=base_address
			
			If PeekByte(ram,$2000) And $8
			
				base_address=base_address+$1000
				
			Else
			
				opposite_address=opposite_adress+$1000
				
			EndIf
			
			base_color_index=(attributes And $3) Shl 2
			
			h_flip = attributes And $40
			v_flip = attributes And $80
			
			If safe_draw
			
				For py=0 To 8-1
				
					For px=0 To 8-1
					
						If Not h_flip
						
							color_index=((PeekByte(vram,base_address+py) And ($80 Shr px)) Or ((PeekByte(vram,base_address+py+8) And ($80 Shr px)) Shl 1 )) Shr (7-px)
							
						Else
						
							color_index=((PeekByte(vram,base_address+py) And ($1 Shl px)) Or ((PeekByte(vram,base_address+py+8) And ($1 Shl px)) Shl 1 )) Shr (px)
						
						EndIf
						
						If (color_index Or base_color_index) Then WritePixel spr_x+px,spr_y+py,palette(PeekByte(vram,$3F10+(color_index Or base_color_index)))
						
					Next
					
				Next
				
				If sprite_size
				
					For py=8 To 16-1
					
						For px=0 To 8-1
						
						color_index=((PeekByte(vram,opposite_address+py) And ($80 Shr px)) Or ((PeekByte(vram,opposite_address+py+8) And ($80 Shr px)) Shl 1 )) Shr (7-px)
						If (color_index Or base_color_index) Then WritePixel spr_x+px,spr_y+py,palette(PeekByte(vram,$3F10+(color_index Or base_color_index)))
							
						Next
						
					Next
				
				EndIf
				
			Else
			
				For py=0 To 8-1
				
					For px=0 To 8-1
					
						If Not h_flip
						
							color_index=((PeekByte(vram,base_address+py) And ($80 Shr px)) Or ((PeekByte(vram,base_address+py+8) And ($80 Shr px)) Shl 1 )) Shr (7-px)
							
						Else
						
							color_index=((PeekByte(vram,base_address+py) And ($1 Shl px)) Or ((PeekByte(vram,base_address+py+8) And ($1 Shl px)) Shl 1 )) Shr (px)
						
						EndIf
						
						If (color_index Or base_color_index) Then WritePixelFast spr_x+px,spr_y+py,palette(PeekByte(vram,$3F10+(color_index Or base_color_index)))
						
					Next
					
				Next
				
				If sprite_size
				
					For py=8 To 16-1
					
						For px=0 To 8-1
						
						color_index=((PeekByte(vram,opposite_address+py) And ($80 Shr px)) Or ((PeekByte(vram,opposite_address+py+8) And ($80 Shr px)) Shl 1 )) Shr (7-px)
						If (color_index Or base_color_index) Then WritePixelFast spr_x+px,spr_y+py,palette(PeekByte(vram,$3F10+(color_index Or base_color_index)))
							
						Next
						
					Next
				
				EndIf
				
			EndIf
			
		EndIf
		
.next_sprite
		
	Next
	
End Function