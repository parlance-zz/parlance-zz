Include "6502.bb"
Include "ppu2.bb"
Include "apu.bb"

;windows and gadgets

;main_window

Global main_window=CreateWindow("PNES 0.1",320,240,256,240,Desktop(),1+2+4+8+32)
SetMinWindowSize main_window,256+8,240+66

SetStatusText main_window,"Idle"

file_menu=CreateMenu("&File",0,WindowMenu(main_window))
CreateMenu "&Load Rom",1,file_menu
CreateMenu "&Unload Rom",2,file_menu
CreateMenu "",0,file_menu
CreateMenu "&Quit",99,file_menu

view_menu=CreateMenu("&View",10,WindowMenu(main_window))
CreateMenu "&Debugger",11,view_menu
CreateMenu "&Fullscreen",12,view_menu

UpdateWindowMenu main_window

Global main_canvas=CreateCanvas(0,0,256,240,main_window)
SetGadgetLayout main_canvas,1,1,1,1

;debugger

Const debug_mode=True
Global debug_log=WriteFile("log.txt")

debugger_window=CreateWindow("Debugger",320+16,240+16,320,320,main_window,1+32)
HideGadget debugger_window

Global op_list=CreateListBox(0,0,320-64,320,debugger_window)

reset_button=CreateButton("Reset",320-64,0,64,28,debugger_window)
runframe_button=CreateButton("Run frame",320-64,28,64,28,debugger_window)
halt_button=CreateButton("Halt",320-64,28*2,64,28,debugger_window)
run_button=CreateButton("Run",320-64,28*3,64,28,debugger_window)
step_button=CreateButton("Step",320-64,28*4,64,28,debugger_window)
clear_button=CreateButton("Clear",320-64,28*5,64,28,debugger_window)
dump_button=CreateButton("Dump",320-64,28*6,64,28,debugger_window)

;internals

Const IDLE=0,EMULATING=1
Global emu_status,rom_loaded
Global fullscreen

Global pnes_path$=SystemProperty("appdir"),rom_path$,save_path$

Const DEVICE_KEYBOARD=0,DEVICE_JOYSTICK=1
Global p1_device,p1_joy_port,p1_x_dead_zone#,p1_y_dead_zone#,p1_joy_a,p1_joy_b,p1_joy_start,p1_joy_select
Global p1_up_key,p1_down_key,p1_left_key,p1_right_key,p1_a_key,p1_b_key,p1_start_key,p1_select_key
Global p2_device,p2_joy_port,p2_x_dead_zone#,p2_y_dead_zone#,p2_joy_a,p2_joy_b,p2_joy_start,p2_joy_select
Global p2_up_key,p2_down_key,p2_left_key,p2_right_key,p2_a_key,p2_b_key,p2_start_key,p2_select_key

Global sound_enabled,video_enabled

;entry point

load_ini()

Repeat

	event_id=WaitEvent(2)
	
	Select event_id

		Case $401 ;gadget action
		
			Select EventSource()
			
				Case reset_button
				
					reset()
					emu_status=IDLE
					
				Case runframe_button
				
					emu_status=IDLE
					
					run_frame()
					
				Case halt_button
				
					emu_status=IDLE
					
				Case run_button
				
					emu_status=EMULATING
					
				Case step_button
				
					emu_status=IDLE
					cycles=-1
					cpu_execute()
					
				Case clear_button
				
					ClearGadgetItems op_list
					
				Case dump_button
				
					dmp_file=WriteFile("ram.bin")
					WriteBytes ram,dmp_file,0,$10000
					CloseFile dmp_file
					
					dmp_file=WriteFile("vram.bin")
					WriteBytes vram,dmp_file,0,$4000
					CloseFile dmp_file
					
					dmp_file=WriteFile("spr_ram.bin")
					WriteBytes spr_ram,dmp_file,0,$100
					CloseFile dmp_file
					
					dmp_file=WriteFile("dump_info.txt")
					
					WriteLine dmp_file,"A : "+Hex$(a)
					WriteLine dmp_file,"X : "+Hex$(x)
					WriteLine dmp_file,"Y : "+Hex$(y)
					WriteLine dmp_file,"S : "+Hex$(s)
					WriteLine dmp_file,"PC : "+Hex$(pc)
					WriteLine dmp_file,"P : "+Hex$(p)
					
					CloseFile dmp_file
					
			End Select
			
		Case $803 ;window close
		
			Select EventSource()
			
				Case main_window
				
					save_ini()
					End
					
				Case debugger_window
				
					HideGadget debugger_window
					
			End Select
			
		Case $1001 ;menu
		
			Select EventData()
			
				Case 1
				
					ChangeDir rom_path$
					rom_name$=RequestFile$("Load ROM","nes",False)
					If rom_name$<>"" Then load_rom(rom_name$)
					
					SetGadgetText main_window,"PNES 0.1 - "+rom_name$
					
				Case 2
					
					unload_rom()
					SetGadgetText main_window,"PNES 0.1"
					
				Case 11
				
					ShowGadget debugger_window
					
				Case 12
				
					fullscreen=True
					FreeGadget main_window
					Graphics 320,240
						
				Case 99
				
					save_ini()
					End
					
			End Select
			
	End Select
	
	If emu_status=EMULATING
	
		If Not rom_loaded
		
			Notify "No ROM loaded!",0
			
			emu_status=IDLE
			SetStatusText main_window,"Idle"
			
		Else
			
			run_frame()
			;old_core()
			
		EndIf
		
	EndIf
	
	If KeyHit(1) And fullscreen=True
	
		fullscreen=False
		End;FRICK! need to rebuild window coming.After() back from fullscreen
		
	EndIf
	
Forever

;functions

Function load_rom(file$)

	SetStatusText main_window,"Loading "+file$+"..."
	
	rom_file=ReadFile(file$)
	
	If rom_file
	
		unload_rom()
		
		header=CreateBank(3)
		ReadBytes header,rom_file,0,3
		
		tmp_header$=Chr$(PeekByte(header,0))+Chr$(PeekByte(header,1))+Chr$(PeekByte(header,2))
		
		If tmp_header$<>"NES"
		
			Notify ";"+file$+"; not a valid NES file!"+tmp_header$,1
			Return
			
		EndIf
		
		SeekFile rom_file,4
		
		prg_pages=ReadByte(rom_file)
		chr_pages=ReadByte(rom_file)
		
		rom_control1=ReadByte(rom_file)
		rom_control2=ReadByte(rom_file)
		
		mapper=(rom_control1 And LOWER_BITS) Or (rom_control2 And UPPER_BITS)
		
		mirroring_mode=rom_control1 And BIT0
		sram_enabled=rom_control1 And BIT1
		trainer_present=rom_control1 And BIT2
		rom_four_screen_mirroring=rom_control1 And BIT3
		
		If rom_four_screen_mirroring
			
			rom_mirroring_mode = FOUR_SCREEN_MIRRORING
			
		Else
		
			rom_mirroring_mode=mirroring_mode
			
		EndIf
		
		;only mapper 0 supported at this point =or
		
		Select mapper
		
			Case 0
		
				SeekFile rom_file,16
				
				If trainer_present
				
					ReadBytes ram,rom_file,$7000,512
					
				EndIf
				
				Select prg_pages
				
					Case 1
					
						ReadBytes ram,rom_file,$8000,$4000
						CopyBank ram,$8000,ram,$C000,$4000;mirror it
						
					Case 2
					
						ReadBytes ram,rom_file,$8000,$8000
						
				End Select

				ReadBytes vram,rom_file,0,chr_pages*8*1024
				
			Default
			
				Notify "Unsupported mapper type ;"+mapper+";!",1
				Return
				
		End Select
				
		CloseFile rom_file
		
		emu_status=EMULATING
		;emu_status=IDLE
		SetStatusText main_window,"Emulating..."
		;SetStatusText main_window,"Idle"
		
		SetGadgetText main_window,"PNES 0.1 - "+file$
		
		rom_loaded=True
		
		reset()
	
		; ************** <TEMP>
		
		;rom_file = ReadFile("e:\programming\nes\pnes\smb")
		
		;ReadBytes ram, rom_file, 0, $10000
		
		;CloseFile rom_file
		
		;pc = $8057
		;s = $FF
		;p = $85
		;a = $90
		;x = $05
		;y = $02
		
		; ************** </TEMP>
				
	Else
	
		 RuntimeError "Could not open rom file ;"+file$+";!"
		
	EndIf
	
End Function

Function unload_rom()

	;reset ram
	For b=0 To $10000-1
	
		PokeByte ram,b,0
		
	Next
	
	;reset vram
	For b=0 To $4000-1
	
		PokeByte vram,b,0
		
	Next
	
	;reset spr ram
	
	For b=0 To $100-1
	
		PokeByte spr_ram,b,0
		
	Next

	rom_loaded=False
	
	emu_status=IDLE
	SetStatusText main_window,"Idle"
	
End Function

Function load_ini()
	
	p1_x_dead_zone#=0.5
	p1_y_dead_zone#=0.5
	p2_x_dead_zone#=0.5
	p2_y_dead_zone#=0.5
	
	sound_enabled=-1
	video_enabled=-1
	
	ini_file=ReadFile("pnes.ini")
	
	If ini_file
	
		While Not Eof(ini_file)
		
			ini$=ReadLine$(ini_file)
			equals_pos=Instr(ini$,"=")
			
			If equals_pos
			
				parameter$=Lower$(Trim$(Left$(ini$,equals_pos-1)))
				param_value$=Lower$(Trim$(Right$(ini$,Len(ini$)-equals_pos)))
				
				Select parameter$
				
					Case "rom_path"
					
						rom_path$=param_value$
				
					Case "save_path"
					
						save_path$=param_value$
						
					Case "p1_device"
					
						Select param_value$
						
							Case "keyboard"
							
								p1_device=DEVICE_KEYBOARD
								
							Case "joystick"
							
								p1_device=DEVICE_JOYSTICK
								
						End Select
						
					Case "p1_joy_port"
					
						p1_joy_port=Int(param_value$)
						
					Case "p1_x_dead_zone"
					
						p1_x_dead_zone#=Float(param_value$)
						
					Case "p1_y_dead_zone"
					
						p1_y_dead_zone#=Float(param_value$)
						
					Case "p1_joy_a"
					
						p1_joy_a=Int(param_value$)
						
					Case "p1_joy_b"
					
						p1_joy_b=Int(param_value$)
						
					Case "p1_joy_select"
					
						p1_joy_select=Int(param_value$)
						
					Case "p1_joy_start"
					
						p1_joy_start=Int(param_value$)
						
					Case "p1_up_key"
					
						p1_up_key=Int(param_value$)
						
					Case "p1_down_key"
					
						p1_down_key=Int(param_value$)
					
					Case "p1_left_key"
					
						p1_left_key=Int(param_value$)
					
					Case "p1_right_key"
					
						p1_right_key=Int(param_value$)
					
					Case "p1_a_key"
					
						p1_a_key=Int(param_value$)
					
					Case "p1_b_key"
					
						p1_b_key=Int(param_value$)

					Case "p1_select_key"
					
						p1_select_key=Int(param_value$)
											
					Case "p1_start_key"
					
						p1_start_key=Int(param_value$)
					
					Case "p2_device"
					
						Select param_value$
						
							Case "keyboard"
							
								p2_device=DEVICE_KEYBOARD
								
							Case "joystick"
							
								p2_device=DEVICE_JOYSTICK
								
						End Select
						
					Case "p2_joy_port"
					
						p2_joy_port=Int(param_value$)
						
					Case "p2_x_dead_zone"
					
						p2_x_dead_zone#=Float(param_value$)
						
					Case "p2_y_dead_zone"
					
						p2_y_dead_zone#=Float(param_value$)
						
					Case "p2_joy_a"
					
						p2_joy_a=Int(param_value$)
						
					Case "p2_joy_b"
					
						p2_joy_b=Int(param_value$)
						
					Case "p2_joy_select"
					
						p2_joy_select=Int(param_value$)
						
					Case "p2_joy_start"
					
						p2_joy_start=Int(param_value$)
						
					Case "p2_up_key"
					
						p2_up_key=Int(param_value$)
						
					Case "p2_down_key"
					
						p2_down_key=Int(param_value$)
					
					Case "p2_left_key"
					
						p2_left_key=Int(param_value$)
					
					Case "p2_right_key"
					
						p2_right_key=Int(param_value$)
					
					Case "p2_a_key"
					
						p2_a_key=Int(param_value$)
					
					Case "p2_b_key"
					
						p2_b_key=Int(param_value$)

					Case "p2_select_key"
					
						p2_select_key=Int(param_value$)
											
					Case "p2_start_key"
					
						p2_start_key=Int(param_value$)
						
					Case "sound_enabled"
					
						sound_enabled=Int(param_value$)
					
					Case "video_enabled"
					
						video_enabled=Int(param_value$)
					
				End Select
				
				
			EndIf
		
		Wend
		
	EndIf
	
	If sound_enabled=-1 Then sound_enabled=1
	If video_enabled=-1 Then video_enabled=1
	
	CloseFile ini_file

End Function

Function save_ini()

	ChangeDir pnes_path$
	
	ini_file=WriteFile("pnes.ini")
	
	If ini_file
	
		WriteLine ini_file,"rom_path	= "+rom_path$
		WriteLine ini_file,"save_path	= "+save_path$
		
		Select p1_device
		
			Case DEVICE_KEYBOARD
			
				WriteLine ini_file,"p1_device	= keyboard"
				
			Case DEVICE_JOYSTICK
			
				WriteLine ini_file,"p1_device	= joystick"
				
		End Select
		
		WriteLine ini_file,"p1_joy_port	= "+p1_joy_port
		WriteLine ini_file,"p1_x_dead_zone	= "+p1_x_dead_zone#
		WriteLine ini_file,"p1_y_dead_zone	= "+p1_y_dead_zone#
		WriteLine ini_file,"p1_joy_a	= "+p1_joy_a
		WriteLine ini_file,"p1_joy_b	= "+p1_joy_a
		WriteLine ini_file,"p1_joy_select	= "+p1_joy_select
		WriteLine ini_file,"p1_joy_start	= "+p1_joy_start
		
		WriteLine ini_file,"p1_up_key	= "+p1_up_key
		WriteLine ini_file,"p1_down_key	= "+p1_down_key
		WriteLine ini_file,"p1_left_key	= "+p1_left_key
		WriteLine ini_file,"p1_right_key	= "+p1_right_key
		WriteLine ini_file,"p1_a_key	= "+p1_a_key
		WriteLine ini_file,"p1_b_key	= "+p1_b_key
		WriteLine ini_file,"p1_select_key	= "+p1_select_key
		WriteLine ini_file,"p1_start_key	= "+p1_start_key
		
		Select p2_device
		
			Case DEVICE_KEYBOARD
			
				WriteLine ini_file,"p2_device	= keyboard"
				
			Case DEVICE_JOYSTICK
			
				WriteLine ini_file,"p2_device	= joystick"
				
		End Select
		
		WriteLine ini_file,"p2_joy_port	= "+p2_joy_port
		WriteLine ini_file,"p2_x_dead_zone	= "+p2_x_dead_zone#
		WriteLine ini_file,"p2_y_dead_zone	= "+p2_y_dead_zone#
		WriteLine ini_file,"p2_joy_a	= "+p2_joy_a
		WriteLine ini_file,"p2_joy_b	= "+p2_joy_a
		WriteLine ini_file,"p2_joy_select	= "+p2_joy_select
		WriteLine ini_file,"p2_joy_start	= "+p2_joy_start
		
		WriteLine ini_file,"p2_up_key	= "+p2_up_key
		WriteLine ini_file,"p2_down_key	= "+p2_down_key
		WriteLine ini_file,"p2_left_key	= "+p2_left_key
		WriteLine ini_file,"p2_right_key	= "+p2_right_key
		WriteLine ini_file,"p2_a_key	= "+p2_a_key
		WriteLine ini_file,"p2_b_key	= "+p2_b_key
		WriteLine ini_file,"p2_select_key	= "+p2_select_key
		WriteLine ini_file,"p2_start_key	= "+p2_start_key
		
		WriteLine ini_file,"sound_enabled	= "+sound_enabled
		WriteLine ini_file,"video_enabled	= "+video_enabled
		
	Else
	
		RuntimeError "Could not write pnes.ini"
		
	EndIf
	
	CloseFile ini_file
	
End Function