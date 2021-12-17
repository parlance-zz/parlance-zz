;internals

;Global square_wave,triangle_wave,noise_wave
;Global square_channel,triangle_channel,noise_channel

; let;s load some shapes
; note ; this is a shitty way of doing it & it probably won;t work:bb This whole idea needs an overhaul:bb

;square_wave=LoadSound("square.wav")
;triangle_wave=LoadSound("triangle.wav")
;noise_wave=LoadSound("noise.wav")

;If square_wave=0 Or triangle_wave=0 Or noise_wave=0

;	RuntimeError "Couldn;t load APU waveforms."
	
;EndIf

;LoopSound square_wave
;LoopSound triangle_wave
;LoopSound noise_wave

;pulse1_channel=PlaySound(square_wave,1)
;pulse2_channel=PlaySound(square_wave,1)
;SetChannelVolume pulse1_channel,0
;SetChannelVolume pulse2_channel,0

;triangle_channel=PlaySound(triangle_wave,1)
;noise_channel=PlaySound(noise_wave,1)