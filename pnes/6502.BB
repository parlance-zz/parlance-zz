;TO DO

;1) watch the amazing stack position (s) get out of sync step by step on genie:bbnes
;   then horribly screw itself by executing an RTI and jumping to $0000
;   THIS IS THE PROBLEM IN EVERY COMMERCIAL ROM! FIX IT! NOW! YOU IDIOT!
;2) speedups
;3) mapper support

;opcode constants

Const BRK			= $00
Const ORA_IX		= $01
;Const FE			= $02
;Const FE			= $03
;Const FE			= $04
Const ORA_ZP		= $05
Const ASL_ZP		= $06
;Const FE			= $07
Const PHP			= $08
Const ORA_IM		= $09
Const ASL_ACM		= $0A
;Const FE			= $0B
;Const FE			= $0C
Const ORA_ABS		= $0D
Const ASL_ABS		= $0E
;Const FE			= $0F
Const BPL			= $10
Const ORA_IY		= $11
;Const FE			= $12
;Const FE			= $13
;Const FE			= $14
Const ORA_ZPX		= $15
Const ASL_ZPX		= $16
;Const FE			= $17
Const CLC			= $18
Const ORA_ABSY		= $19
;Const FE			= $1A
;Const FE			= $1B
;Const FE			= $1C
Const ORA_ABSX		= $1D
Const ASL_ABSX		= $1E
;Const FE			= $1F
Const JSR			= $20
Const AND_IX		= $21
;Const FE			= $22
;Const FE			= $23
Const BIT_ZP		= $24
Const AND_ZP		= $25
Const ROL_ZP		= $26
;Const FE			= $27
Const PLP			= $28
Const AND_IM		= $29
Const ROL_ACM		= $2A
;Const FE			= $2B
Const BIT_ABS		= $2C
Const AND_ABS		= $2D
Const ROL_ABS		= $2E
;Const FE			= $2F
Const BMI			= $30
Const AND_IY		= $31
;Const FE			= $32
;Const FE			= $33
;Const FE			= $34
Const AND_ZPX		= $35
Const ROL_ZPX		= $36
;Const FE			= $37
Const SEC			= $38
Const AND_ABSY		= $39
;Const FE			= $3A
;Const FE			= $3B
;Const FE			= $3C
Const AND_ABSX		= $3D
Const ROL_ABSX		= $3E
;Const FE			= $3F
Const RTI			= $40
Const EOR_IX		= $41
;Const FE			= $42
;Const FE			= $43
;Const FE			= $44
Const EOR_ZP		= $45
Const LSR_ZP		= $46
;Const FE			= $47
Const PHA			= $48
Const EOR_IM		= $49
Const LSR_ACM		= $4A
;Const FE			= $4B
Const JMP_ABS		= $4C
Const EOR_ABS		= $4D
Const LSR_ABS		= $4E
;Const FE			= $4F
Const BVC			= $50
Const EOR_IY		= $51
;Const FE			= $52
;Const FE			= $53
;Const FE			= $54
Const EOR_ZPX		= $55
Const LSR_ZPX		= $56
;Const FE			= $57
Const CLI			= $58
Const EOR_ABSY		= $59
;Const FE			= $5A
;Const FE			= $5B
;Const FE			= $5C
Const EOR_ABSX		= $5D
Const LSR_ABSX		= $5E
;Const FE			= $5F
Const RTS			= $60
Const ADC_IX		= $61
;Const FE			= $62
;Const FE			= $63
;Const FE			= $64
Const ADC_ZP		= $65
Const ROR_ZP		= $66
;Const FE			= $67
Const PLA			= $68
Const ADC_IM		= $69
Const ROR_ACM		= $6A
;Const FE			= $6B
Const JMP_I			= $6C
Const ADC_ABS		= $6D
Const ROR_ABS		= $6E
;Const FE			= $6F
Const BVS			= $70
Const ADC_IY		= $71
;Const FE			= $72
;Const FE			= $73
;Const FE			= $74
Const ADC_ZPX		= $75
Const ROR_ZPX		= $76
;Const FE			= $77
Const SEI			= $78
Const ADC_ABSY		= $79
;Const FE			= $7A
;Const FE			= $7B
;Const FE			= $7C
Const ADC_ABSX		= $7D
Const ROR_ABSX		= $7E
;Const FE			= $7F
;Const FE			= $80
Const STA_IX		= $81
;Const FE			= $82
;Const FE			= $83
Const STY_ZP		= $84
Const STA_ZP		= $85
Const STX_ZP		= $86
;Const FE			= $87
Const DEY			= $88
;Const FE			= $89
Const TXA			= $8A
;Const FE			= $8B
Const STY_ABS		= $8C
Const STA_ABS		= $8D
Const STX_ABS		= $8E
;Const FE			= $8F
Const BCC			= $90
Const STA_IY		= $91
;Const FE			= $92
;Const FE			= $93
Const STY_ZPX		= $94
Const STA_ZPX		= $95
Const STX_ZPY		= $96
;Const FE			= $97
Const TYA			= $98
Const STA_ABSY		= $99
Const TXS			= $9A
;Const FE			= $9B
;Const FE			= $9C
Const STA_ABSX		= $9D
;Const FE			= $9E
;Const FE			= $9F
Const LDY_IM		= $A0
Const LDA_IX		= $A1
Const LDX_IM		= $A2
;Const FE			= $A3
Const LDY_ZP		= $A4
Const LDA_ZP		= $A5
Const LDX_ZP		= $A6
;Const FE			= $A7
Const TAY			= $A8
Const LDA_IM		= $A9
Const TAX			= $AA
;Const FE			= $AB
Const LDY_ABS		= $AC
Const LDA_ABS		= $AD
Const LDX_ABS		= $AE
;Const FE			= $AF
Const BCS			= $B0
Const LDA_IY		= $B1
;Const FE			= $B2
;Const FE			= $B3
Const LDY_ZPX		= $B4
Const LDA_ZPX		= $B5
Const LDX_ZPY		= $B6
;Const FE			= $B7
Const CLV			= $B8
Const LDA_ABSY		= $B9
Const TSX			= $BA
;Const FE			= $BB
Const LDY_ABSX		= $BC
Const LDA_ABSX		= $BD
Const LDX_ABSY		= $BE
;Const FE			= $BF
Const CPY_IM		= $C0
Const CMP_IX		= $C1
;Const FE			= $C2
;Const FE			= $C3
Const CPY_ZP		= $C4
Const CMP_ZP		= $C5
Const DEC_ZP		= $C6
;Const FE			= $C7
Const INY			= $C8
Const CMP_IM		= $C9
Const DEX			= $CA
;Const FE			= $CB
Const CPY_ABS		= $CC
Const CMP_ABS		= $CD
Const DEC_ABS		= $CE
;Const FE			= $CF
Const BNE			= $D0
Const CMP_IY		= $D1
;Const FE			= $D2
;Const FE			= $D3
;Const FE			= $D4
Const CMP_ZPX		= $D5
Const DEC_ZPX		= $D6
;Const FE			= $D7
Const CLD			= $D8
Const CMP_ABSY		= $D9
;Const FE			= $DA
;Const FE			= $DB 
;Const FE			= $DC
Const CMP_ABSX		= $DD
Const DEC_ABSX		= $DE
;Const FE			= $DF
Const CPX_IM		= $E0
Const SBC_IX		= $E1
;Const FE			= $E2
;Const FE			= $E3
Const CPX_ZP		= $E4
Const SBC_ZP		= $E5
Const INC_ZP		= $E6
;Const FE			= $E7
Const INX			= $E8
Const SBC_IM		= $E9
Const NOP			= $EA
;Const FE			= $EB
Const CPX_ABS		= $EC
Const SBC_ABS		= $ED
Const INC_ABS		= $EE
;Const FE			= $EF
Const BEQ			= $F0
Const SBC_IY		= $F1
;Const FE			= $F2
;Const FE			= $F3
;Const FE			= $F4
Const SBC_ZPX		= $F5
Const INC_ZPX		= $F6
;Const FE			= $F7
Const SED			= $F8
Const SBC_ABSY		= $F9
;Const FE			= $FA
;Const FE			= $FB
;Const FE			= $FC
Const SBC_ABSX		= $FD
Const INC_ABSX		= $FE
;Const FE			= $FF

;opcode addressing modes

Const MODE_IMPLIED				= $00
Const MODE_RELATIVE				= $01
Const MODE_IMMEDIATE			= $02
Const MODE_ABSOLUTE				= $03
Const MODE_ABSOLUTE_X			= $04
Const MODE_ABSOLUTE_Y			= $05
Const MODE_INDIRECT				= $06
Const MODE_INDIRECT_X			= $07
Const MODE_INDIRECT_Y			= $08
Const MODE_INDIRECT_ZEROPAGE	= $09
Const MODE_AINDIRECT_ABSOLUTE_X	= $0A
Const MODE_ZEROPAGE				= $0B
Const MODE_ZEROPAGE_X			= $0C
Const MODE_ZEROPAGE_Y			= $0D

Dim op_mode($FF+1)

op_mode($00)		= MODE_IMPLIED
op_mode($01)		= MODE_INDIRECT_X
op_mode($02)		= MODE_IMPLIED
op_mode($03)		= MODE_IMPLIED
op_mode($04)		= MODE_ZEROPAGE
op_mode($05)		= MODE_ZEROPAGE
op_mode($06)		= MODE_ZEROPAGE
op_mode($07)		= MODE_IMPLIED
op_mode($08)		= MODE_IMPLIED
op_mode($09)		= MODE_IMMEDIATE
op_mode($0A)		= MODE_IMPLIED
op_mode($0B)		= MODE_IMPLIED
op_mode($0C)		= MODE_ABSOLUTE
op_mode($0D)		= MODE_ABSOLUTE
op_mode($0E)		= MODE_ABSOLUTE
op_mode($0F)		= MODE_IMPLIED
op_mode($10)		= MODE_RELATIVE
op_mode($11)		= MODE_INDIRECT_Y
op_mode($12)		= MODE_INDIRECT_ZEROPAGE
op_mode($13)		= MODE_IMPLIED
op_mode($14)		= MODE_ZEROPAGE
op_mode($15)		= MODE_ZEROPAGE_X
op_mode($16)		= MODE_ZEROPAGE_X
op_mode($17)		= MODE_IMPLIED
op_mode($18)		= MODE_IMPLIED
op_mode($19)		= MODE_ABSOLUTE_Y
op_mode($1A)		= MODE_IMPLIED
op_mode($1B)		= MODE_IMPLIED
op_mode($1C)		= MODE_ABSOLUTE
op_mode($1D)		= MODE_ABSOLUTE_X
op_mode($1E)		= MODE_ABSOLUTE_X
op_mode($1F)		= MODE_IMPLIED
op_mode($20)		= MODE_ABSOLUTE
op_mode($21)		= MODE_INDIRECT_X
op_mode($22)		= MODE_IMPLIED
op_mode($23)		= MODE_IMPLIED
op_mode($24)		= MODE_ZEROPAGE
op_mode($25)		= MODE_ZEROPAGE
op_mode($26)		= MODE_ZEROPAGE
op_mode($27)		= MODE_IMPLIED
op_mode($28)		= MODE_IMPLIED
op_mode($29)		= MODE_IMMEDIATE
op_mode($20)		= MODE_ABSOLUTE
op_mode($2A)		= MODE_IMPLIED
op_mode($2B)		= MODE_IMPLIED
op_mode($2C)		= MODE_ABSOLUTE
op_mode($2D)		= MODE_ABSOLUTE
op_mode($2E)		= MODE_ABSOLUTE
op_mode($2F)		= MODE_IMPLIED
op_mode($30)		= MODE_RELATIVE
op_mode($31)		= MODE_INDIRECT_Y
op_mode($32)		= MODE_INDIRECT_ZEROPAGE
op_mode($33)		= MODE_IMPLIED
op_mode($34)		= MODE_ZEROPAGE_X
op_mode($35)		= MODE_ZEROPAGE_X
op_mode($36)		= MODE_ZEROPAGE_X
op_mode($37)		= MODE_IMPLIED
op_mode($38)		= MODE_IMPLIED
op_mode($39)		= MODE_ABSOLUTE_Y
op_mode($3A)		= MODE_IMPLIED
op_mode($3B)		= MODE_IMPLIED
op_mode($3C)		= MODE_ABSOLUTE_X
op_mode($3D)		= MODE_ABSOLUTE_X
op_mode($3E)		= MODE_ABSOLUTE_X
op_mode($3F)		= MODE_IMPLIED
op_mode($40)		= MODE_IMPLIED
op_mode($41)		= MODE_INDIRECT_X
op_mode($42)		= MODE_IMPLIED
op_mode($43)		= MODE_IMPLIED
op_mode($44)		= MODE_IMPLIED
op_mode($45)		= MODE_ZEROPAGE
op_mode($46)		= MODE_ZEROPAGE
op_mode($47)		= MODE_IMPLIED
op_mode($48)		= MODE_IMPLIED
op_mode($49)		= MODE_IMMEDIATE
op_mode($4A)		= MODE_IMPLIED
op_mode($4B)		= MODE_IMPLIED
op_mode($4C)		= MODE_ABSOLUTE
op_mode($4D)		= MODE_ABSOLUTE
op_mode($4E)		= MODE_ABSOLUTE
op_mode($4F)		= MODE_IMPLIED
op_mode($50)		= MODE_RELATIVE
op_mode($51)		= MODE_INDIRECT_Y
op_mode($52)		= MODE_INDIRECT_ZEROPAGE
op_mode($53)		= MODE_IMPLIED
op_mode($54)		= MODE_IMPLIED
op_mode($55)		= MODE_ZEROPAGE_X
op_mode($56)		= MODE_ZEROPAGE_X
op_mode($57)		= MODE_IMPLIED
op_mode($58)		= MODE_IMPLIED
op_mode($59)		= MODE_ABSOLUTE_Y
op_mode($5A)		= MODE_IMPLIED
op_mode($5B)		= MODE_IMPLIED
op_mode($5C)		= MODE_IMPLIED
op_mode($5D)		= MODE_ABSOLUTE_X
op_mode($5E)		= MODE_ABSOLUTE_X
op_mode($5F)		= MODE_IMPLIED
op_mode($60)		= MODE_IMPLIED
op_mode($61)		= MODE_INDIRECT_X
op_mode($62)		= MODE_IMPLIED
op_mode($63)		= MODE_IMPLIED
op_mode($64)		= MODE_ZEROPAGE
op_mode($65)		= MODE_ZEROPAGE
op_mode($66)		= MODE_ZEROPAGE
op_mode($67)		= MODE_IMPLIED
op_mode($68)		= MODE_IMPLIED
op_mode($69)		= MODE_IMMEDIATE
op_mode($6A)		= MODE_IMPLIED
op_mode($6B)		= MODE_IMPLIED
op_mode($6C)		= MODE_INDIRECT
op_mode($6D)		= MODE_ABSOLUTE
op_mode($6E)		= MODE_ABSOLUTE
op_mode($6F)		= MODE_IMPLIED
op_mode($70)		= MODE_RELATIVE
op_mode($71)		= MODE_INDIRECT_Y
op_mode($72)		= MODE_INDIRECT_ZEROPAGE
op_mode($73)		= MODE_IMPLIED
op_mode($74)		= MODE_ZEROPAGE_X
op_mode($75)		= MODE_ZEROPAGE_X
op_mode($76)		= MODE_ZEROPAGE_X
op_mode($77)		= MODE_IMPLIED
op_mode($78)		= MODE_IMPLIED
op_mode($79)		= MODE_ABSOLUTE_Y
op_mode($7A)		= MODE_IMPLIED
op_mode($7B)		= MODE_IMPLIED
op_mode($7C)		= MODE_INDIRECT_ABSOLUTE_X
op_mode($7D)		= MODE_ABSOLUTE_X
op_mode($7E)		= MODE_ABSOLUTE_X
op_mode($7F)		= MODE_IMPLIED
op_mode($80)		= MODE_RELATIVE
op_mode($81)		= MODE_INDIRECT_X
op_mode($82)		= MODE_IMPLIED
op_mode($83)		= MODE_IMPLIED
op_mode($84)		= MODE_ZEROPAGE
op_mode($85)		= MODE_ZEROPAGE
op_mode($86)		= MODE_ZEROPAGE
op_mode($87)		= MODE_IMPLIED
op_mode($88)		= MODE_IMPLIED
op_mode($89)		= MODE_IMMEDIATE
op_mode($8A)		= MODE_IMPLIED
op_mode($8B)		= MODE_IMPLIED
op_mode($8C)		= MODE_ABSOLUTE
op_mode($8D)		= MODE_ABSOLUTE
op_mode($8E)		= MODE_ABSOLUTE
op_mode($8F)		= MODE_IMPLIED
op_mode($90)		= MODE_RELATIVE
op_mode($91)		= MODE_INDIRECT_Y
op_mode($92)		= MODE_INDIRECT_ZEROPAGE
op_mode($93)		= MODE_IMPLIED
op_mode($94)		= MODE_ZEROPAGE_X
op_mode($95)		= MODE_ZEROPAGE_X
op_mode($96)		= MODE_ZEROPAGE_X
op_mode($97)		= MODE_IMPLIED
op_mode($98)		= MODE_IMPLIED
op_mode($99)		= MODE_ABSOLUTE_Y
op_mode($9A)		= MODE_IMPLIED
op_mode($9B)		= MODE_IMPLIED
op_mode($9C)		= MODE_ABSOLUTE
op_mode($9D)		= MODE_ABSOLUTE_X
op_mode($9E)		= MODE_ABSOLUTE_X
op_mode($9F)		= MODE_IMPLIED
op_mode($A0)		= MODE_IMMEDIATE
op_mode($A1)		= MODE_INDIRECT_X
op_mode($A2)		= MODE_IMMEDIATE
op_mode($A3)		= MODE_IMPLIED
op_mode($A4)		= MODE_ZEROPAGE
op_mode($A5)		= MODE_ZEROPAGE
op_mode($A6)		= MODE_ZEROPAGE
op_mode($A7)		= MODE_IMPLIED
op_mode($A8)		= MODE_IMPLIED
op_mode($A9)		= MODE_IMMEDIATE
op_mode($AA)		= MODE_IMPLIED
op_mode($AB)		= MODE_IMPLIED
op_mode($AC)		= MODE_ABSOLUTE
op_mode($AD)		= MODE_ABSOLUTE
op_mode($AE)		= MODE_ABSOLUTE
op_mode($AF)		= MODE_IMPLIED
op_mode($B0)		= MODE_RELATIVE
op_mode($B1)		= MODE_INDIRECT_Y
op_mode($B2)		= MODE_INDIRECT_ZEROPAGE
op_mode($B3)		= MODE_IMPLIED
op_mode($B4)		= MODE_ZEROPAGE_X
op_mode($B5)		= MODE_ZEROPAGE_X
op_mode($B6)		= MODE_ZEROPAGE_X
op_mode($B7)		= MODE_IMPLIED
op_mode($B8)		= MODE_IMPLIED
op_mode($B9)		= MODE_ABSOLUTE_Y
op_mode($BA)		= MODE_IMPLIED
op_mode($BB)		= MODE_IMPLIED
op_mode($BC)		= MODE_ABSOLUTE_X
op_mode($BD)		= MODE_ABSOLUTE_X
op_mode($BE)		= MODE_ABSOLUTE_Y
op_mode($BF)		= MODE_IMPLIED
op_mode($C0)		= MODE_IMMEDIATE
op_mode($C1)		= MODE_INDIRECT_X
op_mode($C2)		= MODE_IMPLIED
op_mode($C3)		= MODE_IMPLIED
op_mode($C4)		= MODE_ZEROPAGE
op_mode($C5)		= MODE_ZEROPAGE
op_mode($C6)		= MODE_ZEROPAGE
op_mode($C7)		= MODE_IMPLIED
op_mode($C8)		= MODE_IMPLIED
op_mode($C9)		= MODE_IMMEDIATE
op_mode($CA)		= MODE_IMPLIED
op_mode($CB)		= MODE_IMPLIED
op_mode($CC)		= MODE_ABSOLUTE
op_mode($CD)		= MODE_ABSOLUTE
op_mode($CE)		= MODE_ABSOLUTE
op_mode($CF)		= MODE_IMPLIED
op_mode($D0)		= MODE_RELATIVE
op_mode($D1)		= MODE_INDIRECT_Y
op_mode($D2)		= MODE_INDIRECT_ZEROPAGE
op_mode($D3)		= MODE_IMPLIED
op_mode($D4)		= MODE_IMPLIED
op_mode($D5)		= MODE_ZEROPAGE_X
op_mode($D6)		= MODE_ZEROPAGE_X
op_mode($D7)		= MODE_IMPLIED
op_mode($D8)		= MODE_IMPLIED
op_mode($D9)		= MODE_ABSOLUTE_Y
op_mode($DA)		= MODE_IMPLIED
op_mode($DB)		= MODE_IMPLIED
op_mode($DC)		= MODE_IMPLIED
op_mode($DD)		= MODE_ABSOLUTE_X
op_mode($DE)		= MODE_ABSOLUTE_X
op_mode($DF)		= MODE_IMPLIED
op_mode($E0)		= MODE_IMMEDIATE
op_mode($E1)		= MODE_INDIRECT_X
op_mode($E2)		= MODE_IMPLIED
op_mode($E3)		= MODE_IMPLIED
op_mode($E4)		= MODE_ZEROPAGE
op_mode($E5)		= MODE_ZEROPAGE
op_mode($E6)		= MODE_ZEROPAGE
op_mode($E7)		= MODE_IMPLIED
op_mode($E8)		= MODE_IMPLIED
op_mode($E9)		= MODE_IMMEDIATE
op_mode($EA)		= MODE_IMPLIED
op_mode($EB)		= MODE_IMPLIED
op_mode($EC)		= MODE_ABSOLUTE
op_mode($ED)		= MODE_ABSOLUTE
op_mode($EE)		= MODE_ABSOLUTE
op_mode($EF)		= MODE_IMPLIED
op_mode($E0)		= MODE_IMMEDIATE
op_mode($F0)		= MODE_RELATIVE
op_mode($F1)		= MODE_INDIRECT_Y
op_mode($F2)		= MODE_INDIRECT_ZEROPAGE
op_mode($F3)		= MODE_IMPLIED
op_mode($F4)		= MODE_IMPLIED
op_mode($F5)		= MODE_ZEROPAGE_X
op_mode($F6)		= MODE_ZEROPAGE_X
op_mode($F7)		= MODE_IMPLIED
op_mode($F8)		= MODE_IMPLIED
op_mode($F9)		= MODE_ABSOLUTE_Y
op_mode($FA)		= MODE_IMPLIED
op_mode($FB)		= MODE_IMPLIED
op_mode($FC)		= MODE_IMPLIED
op_mode($FD)		= MODE_ABSOLUTE_X
op_mode($FE)		= MODE_ABSOLUTE_X
op_mode($FF)		= MODE_IMPLIED

;opcode cycles

Dim op_cycles($FF+1)

op_cycles($00)		= 7
op_cycles($01)		= 6
op_cycles($02)		= 2
op_cycles($03)		= 2
op_cycles($04)		= 3
op_cycles($05)		= 3
op_cycles($06)		= 5
op_cycles($07)		= 2
op_cycles($08)		= 3
op_cycles($09)		= 3
op_cycles($0A)		= 2
op_cycles($0B)		= 2
op_cycles($0C)		= 4
op_cycles($0D)		= 4
op_cycles($0E)		= 6
op_cycles($0F)		= 2
op_cycles($10)		= 2
op_cycles($11)		= 5
op_cycles($12)		= 3
op_cycles($13)		= 2
op_cycles($14)		= 3
op_cycles($15)		= 4
op_cycles($16)		= 6
op_cycles($17)		= 2
op_cycles($18)		= 2
op_cycles($19)		= 4
op_cycles($1A)		= 2
op_cycles($1B)		= 2
op_cycles($1C)		= 4
op_cycles($1D)		= 4
op_cycles($1E)		= 7
op_cycles($1F)		= 2
op_cycles($20)		= 6
op_cycles($21)		= 6
op_cycles($22)		= 2
op_cycles($23)		= 2
op_cycles($24)		= 3
op_cycles($25)		= 3
op_cycles($26)		= 5
op_cycles($27)		= 2
op_cycles($28)		= 4
op_cycles($29)		= 3
op_cycles($20)		= 2
op_cycles($2A)		= 2
op_cycles($2B)		= 2
op_cycles($2C)		= 4
op_cycles($2D)		= 4
op_cycles($2E)		= 6
op_cycles($2F)		= 2
op_cycles($30)		= 2
op_cycles($31)		= 5
op_cycles($32)		= 3
op_cycles($33)		= 2
op_cycles($34)		= 4
op_cycles($35)		= 4
op_cycles($36)		= 6
op_cycles($37)		= 2
op_cycles($38)		= 2
op_cycles($39)		= 4
op_cycles($3A)		= 2
op_cycles($3B)		= 2
op_cycles($3C)		= 4
op_cycles($3D)		= 4
op_cycles($3E)		= 7
op_cycles($3F)		= 2
op_cycles($40)		= 6
op_cycles($41)		= 6
op_cycles($42)		= 2
op_cycles($43)		= 2
op_cycles($44)		= 2
op_cycles($45)		= 3
op_cycles($46)		= 5
op_cycles($47)		= 2
op_cycles($48)		= 3
op_cycles($49)		= 3
op_cycles($4A)		= 2
op_cycles($4B)		= 2
op_cycles($4C)		= 3
op_cycles($4D)		= 4
op_cycles($4E)		= 6
op_cycles($4F)		= 2
op_cycles($50)		= 2
op_cycles($51)		= 5
op_cycles($52)		= 3
op_cycles($53)		= 2
op_cycles($54)		= 2
op_cycles($55)		= 4
op_cycles($56)		= 6
op_cycles($57)		= 2
op_cycles($58)		= 2
op_cycles($59)		= 4
op_cycles($5A)		= 3
op_cycles($5B)		= 2
op_cycles($5C)		= 2
op_cycles($5D)		= 4
op_cycles($5E)		= 7
op_cycles($5F)		= 2
op_cycles($60)		= 6
op_cycles($61)		= 6
op_cycles($62)		= 2
op_cycles($63)		= 2
op_cycles($64)		= 3
op_cycles($65)		= 3
op_cycles($66)		= 5
op_cycles($67)		= 2
op_cycles($68)		= 4
op_cycles($69)		= 3
op_cycles($6A)		= 2
op_cycles($6B)		= 2
op_cycles($6C)		= 5
op_cycles($6D)		= 4
op_cycles($6E)		= 6
op_cycles($6F)		= 2
op_cycles($70)		= 2
op_cycles($71)		= 5
op_cycles($72)		= 3
op_cycles($73)		= 2
op_cycles($74)		= 4
op_cycles($75)		= 4
op_cycles($76)		= 6
op_cycles($77)		= 2
op_cycles($78)		= 2
op_cycles($79)		= 4
op_cycles($7A)		= 4
op_cycles($7B)		= 2
op_cycles($7C)		= 6
op_cycles($7D)		= 4
op_cycles($7E)		= 7
op_cycles($7F)		= 2
op_cycles($80)		= 2
op_cycles($81)		= 6
op_cycles($82)		= 2
op_cycles($83)		= 2
op_cycles($84)		= 2
op_cycles($85)		= 2
op_cycles($86)		= 2
op_cycles($87)		= 2
op_cycles($88)		= 2
op_cycles($89)		= 2
op_cycles($8A)		= 2
op_cycles($8B)		= 2
op_cycles($8C)		= 4
op_cycles($8D)		= 4
op_cycles($8E)		= 4
op_cycles($8F)		= 2
op_cycles($90)		= 2
op_cycles($91)		= 6
op_cycles($92)		= 3
op_cycles($93)		= 2
op_cycles($94)		= 4
op_cycles($95)		= 4
op_cycles($96)		= 4
op_cycles($97)		= 2
op_cycles($98)		= 2
op_cycles($99)		= 5
op_cycles($9A)		= 2
op_cycles($9B)		= 2
op_cycles($9C)		= 4
op_cycles($9D)		= 5
op_cycles($9E)		= 5
op_cycles($9F)		= 2
op_cycles($A0)		= 3
op_cycles($A1)		= 6
op_cycles($A2)		= 3
op_cycles($A3)		= 2
op_cycles($A4)		= 3
op_cycles($A5)		= 3
op_cycles($A6)		= 3
op_cycles($A7)		= 2
op_cycles($A8)		= 2
op_cycles($A9)		= 3
op_cycles($AA)		= 2
op_cycles($AB)		= 2
op_cycles($AC)		= 4
op_cycles($AD)		= 4
op_cycles($AE)		= 4
op_cycles($AF)		= 2
op_cycles($B0)		= 2
op_cycles($B1)		= 5
op_cycles($B2)		= 3
op_cycles($B3)		= 2
op_cycles($B4)		= 4
op_cycles($B5)		= 4
op_cycles($B6)		= 4
op_cycles($B7)		= 2
op_cycles($B8)		= 2
op_cycles($B9)		= 4
op_cycles($BA)		= 2
op_cycles($BB)		= 2
op_cycles($BC)		= 4
op_cycles($BD)		= 4
op_cycles($BE)		= 4
op_cycles($BF)		= 2
op_cycles($C0)		= 3
op_cycles($C1)		= 6
op_cycles($C2)		= 2
op_cycles($C3)		= 2
op_cycles($C4)		= 3
op_cycles($C5)		= 3
op_cycles($C6)		= 5
op_cycles($C7)		= 2
op_cycles($C8)		= 2
op_cycles($C9)		= 3
op_cycles($CA)		= 2
op_cycles($CB)		= 2
op_cycles($CC)		= 4
op_cycles($CD)		= 4
op_cycles($CE)		= 6
op_cycles($CF)		= 2
op_cycles($D0)		= 2
op_cycles($D1)		= 5
op_cycles($D2)		= 3
op_cycles($D3)		= 2
op_cycles($D4)		= 2
op_cycles($D5)		= 4
op_cycles($D6)		= 6
op_cycles($D7)		= 2
op_cycles($D8)		= 2
op_cycles($D9)		= 4
op_cycles($DA)		= 3
op_cycles($DB)		= 2
op_cycles($DC)		= 2
op_cycles($DD)		= 4
op_cycles($DE)		= 7
op_cycles($DF)		= 2
op_cycles($E0)		= 3
op_cycles($E1)		= 6
op_cycles($E2)		= 2
op_cycles($E3)		= 2
op_cycles($E4)		= 3
op_cycles($E5)		= 3
op_cycles($E6)		= 5
op_cycles($E7)		= 2
op_cycles($E8)		= 2
op_cycles($E9)		= 3
op_cycles($EA)		= 2
op_cycles($EB)		= 2
op_cycles($EC)		= 4
op_cycles($ED)		= 4
op_cycles($EE)		= 6
op_cycles($EF)		= 2
op_cycles($E0)		= 2
op_cycles($F0)		= 2
op_cycles($F1)		= 5
op_cycles($F2)		= 3
op_cycles($F3)		= 2
op_cycles($F4)		= 2
op_cycles($F5)		= 4
op_cycles($F6)		= 6
op_cycles($F7)		= 2
op_cycles($F8)		= 2
op_cycles($F9)		= 4
op_cycles($FA)		= 4
op_cycles($FB)		= 2
op_cycles($FC)		= 2
op_cycles($FD)		= 4
op_cycles($FE)		= 7
op_cycles($FF)		= 2

;bit constants

Const BIT0			= $01
Const BIT1			= $02
Const BIT2			= $04
Const BIT3			= $08
Const BIT4			= $10
Const BIT5			= $20
Const BIT6			= $40
Const BIT7			= $80

Const LOWER_BITS	= $D0
Const UPPER_BITS	= $0F

Const LOWER_BYTE	= $FF00
Const UPPER_BYTE	= $00FF

;status register bits

Const STATUS_CARRY_FLAG					= BIT0
Const STATUS_ZERO_FLAG					= BIT1
Const STATUS_INTERRUPT_FLAG				= BIT2
Const STATUS_BCD_FLAG					= BIT3
Const STATUS_SOFTWARE_INTERRUPT_FLAG	= BIT4
Const STATUS_UNUSED_FLAG				= BIT5
Const STATUS_OVERFLOW_FLAG				= BIT6
Const STATUS_SIGN_FLAG					= BIT7

;joypad strobing constants

Const A_BUTTON			= 0
Const B_BUTTON			= 1
Const SELECT_BUTTON	= 2
Const START_BUTTON	= 3
Const UP_BUTTON		= 4
Const DOWN_BUTTON		= 5
Const LEFT_BUTTON		= 6
Const RIGHT_BUTTON	= 7

;memory

Global ram=CreateBank($10000)
Global vram=CreateBank($4000)
Global spr_ram=CreateBank($100) ;does this need to be seperate from vram?

Global prg_size,chr_size

;registers

Global a,x,y,s,pc,p 		;accumulator,x register, y register, stack pointer, program counter and status register

;internals

Global op_code,save_pc,value,sum,save_flags,tmp,cycles
Global real_address

Global joypad1_strobe,joypad2_strobe
Global joypad1_button,joypad2_button

; **** addressing mode functions ****

Function relative_6502()
	
	save_pc=read_6502(pc)
	
	pc=pc+1
	
	pc=pc And $FFFF
	
	If (save_pc And $80) Then save_pc=save_pc-$100
	
	If save_pc Shr 8<>pc Shr 8
	
		cycles=cycles-1
		
	EndIf
	
End Function
	
Function immediate_6502()

	save_pc=pc
	pc=pc+1
	
	pc=pc And $FFFF
	
End Function

Function absolute_6502()
	
	save_pc=read_6502(pc)+(read_6502(pc+1) Shl 8)
	pc=pc+2
	
	pc=pc And $FFFF
	
End Function
	
Function absolute_x_6502()

	save_pc=read_6502(pc)+(read_6502(pc+1) Shl 8)
	
	pc=pc+2
	pc=pc And $FFFF
	
	If op_cycles(op_code)=4
	
		If save_pc Shr 8<>(save_pc+x) Shr 8
		
			cycles=cycles-1
			
		EndIf
		
	EndIf
	
	save_pc=save_pc+x
	save_pc=save_pc And $FFFF
	
End Function

Function absolute_y_6502()

	save_pc=read_6502(pc)+(read_6502(pc+1) Shl 8)
	
	pc=pc+2
	pc=pc And $FFFF
	
	If op_cycles(op_code)=4
	
		If save_pc Shr 8<>(save_pc+y) Shr 8
		
			cycles=cycles-1
			
		EndIf
		
	EndIf
	
	save_pc=save_pc+y
	save_pc=save_pc And $FFFF
	
End Function

Function indirect_6502()

	tmp=read_6502(pc)+(read_6502(pc+1) Shl 8)
	save_pc=read_6502(tmp)+(read_6502(tmp+1) Shl 8)
	
	pc=pc+2
	
	pc=pc And $FFFF
	
End Function

Function indirect_x_6502()
		
	value=read_6502(pc)+x
	pc=pc+1

	pc=pc And $FFFF
	
	save_pc=read_6502(value)+(read_6502(value+1) Shl 8)
	
End Function

Function indirect_y_6502()

	value=read_6502(pc)
	pc=pc+1
	
	pc=pc And $FFFF
	
	save_pc=read_6502(value)+(read_6502(value+1) Shl 8)
	
	If op_cycles(op_code)=5
	
		If save_pc Shr 8<>(save_pc+y) Shr 8
		
			cycles=cycles-1
			
		EndIf
		
	EndIf
	
	save_pc=save_pc+y
	
	save_pc=save_pc And $FFFF
	
End Function

Function indirect_zeropage_6502()
	
	value=read_6502(pc)
	pc=pc+1
	
	pc=pc And $FFFF
	
	save_pc=read_6502(value)+(read_6502(value+1) Shl 8)
	
End Function

Function indirect_absolute_x_6502()

	tmp=read_6502(pc)+(read_6502(pc+1) Shl 8)+x
	save_pc=read_6502(tmp)+(read_6502(tmp+1) Shl 8)
	
End Function

Function zeropage_6502()
	
	save_pc=read_6502(pc)
	pc=pc+1
	
	pc=pc And $FFFF
	
End Function

Function zeropage_x_6502()
	
	save_pc=read_6502(pc)+x
	pc=pc+1
	
	pc=pc And $FFFF
	
	save_pc=save_pc And $FF
	
End Function

Function zeropage_y_6502()
	
	save_pc=read_6502(pc)+y
	pc=pc+1
	
	pc=pc And $FFFF
	
	save_pc=save_pc And $FF
	
End Function

; **** instruction functions ****

Function brk_6502()
		
	pc=pc+1
	
	write_6502($100+s,pc Shr 8)
	s=s-1
	
	s=s And $FF
	
	write_6502($100+s,pc And $FF)
	s=s-1
	
	s=s And $FF
	
	write_6502($100+s,p)
	s=s-1
	
	s=s And $FF
	
	p=p Or $14
	pc=read_6502($FFFE) Or (read_6502($FFFF) Shl 8)
	
	;debug2
	;emu_status=IDLE
	;cycles=-1
	;Notify "BRK",0
		
End Function

Function ora_6502()

	mode_execute()
	
	a=a Or read_6502(save_pc)
	
	If a
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (a And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function asla_6502()

	If a And $80
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf
	
	a=a Shl 1
	a=a And $FF
	
	If (a)
	
		p=p And $FD
		
	Else
	
		p=p Or $02
		
	EndIf
	
	If (a And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf

End Function

Function asl_6502()

	mode_execute()
	
	value=read_6502(save_pc)
	
	If value And $80
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf
	
	value=value Shl 1
	value=value And $FF
	
	write_6502(save_pc,value)
	
	If (value)
	
		p=p And $FD
	
	Else
	
		p=p Or $2
		
	EndIf
	
	If (value And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function php_6502()

	write_6502($100+s,p)
	s=s-1
	
	s=s And $FF
	
	;debug3
	;emu_status=IDLE
	;cycles=-1
	;Notify "PHP",0
		
End Function

Function pha_6502()

	write_6502($100+s,a)
	s=s-1
	
	s=s And $FF
	
	;debug4
	;emu_status=IDLE
	;cycles=-1
	;Notify "PHA",0
		
End Function

Function bpl_6502()

	If (p And $80)=0
		
		mode_execute()
		
		pc=pc+save_pc
		pc=pc And $FFFF
		
		cycles=cycles-1
		
	Else
		
		value=read_6502(pc)
		pc=pc+1
		pc=pc And $FFFF
		
	EndIf
	
End Function

Function clc_6502()

	p=p And $FE
	
End Function

Function and_6502()

	mode_execute()
	
	value=read_6502(save_pc)
	a=a And value
	
	If a
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (a And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf

End Function

Function bit_6502()

	mode_execute()
	
	value=read_6502(save_pc)
	
	If value And $80
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
	If $40 And value
	
		p=p Or $40
		
	Else
	
		p=p And $BF
		
	EndIf
	
	If (value And a)
	
		p=p And $FD
		
	Else
		
		p=p Or $2
		
	EndIf

End Function

Function rola_6502()
	
	a=a Shl 1
	If p And $1 Then a=(a Or $1)
	
	If a>$FF
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf

	a=a And $FF
	
	If a
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (a And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function rol_6502()
	
	mode_execute()
	
	value=read_6502(save_pc)
	
	value=value Shl 1
	If p And $1 Then value=(value Or $1)
	
	If value>$FF
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf

	value=value And $FF
	write_6502(save_pc,value)
	
	If value
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (value And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf

End Function

Function plp_6502()

	s=s+1
	s=s And $FF
	
	p=read_6502(s+$100) Or $20

End Function

Function bmi_6502()

	If (p And $80)
	
		mode_execute()
		
		pc=pc+save_pc
		pc=pc And $FFFF
		
		cycles=cycles-1
	
	Else
	
		value=read_6502(pc)
		
		pc=pc+1
		pc=pc And $FFFF
		
	EndIf

End Function

Function rti_6502()

	s=s+1
	s=s And $FF
	p=read_6502(s+$100) Or $20
	
	s=s+1
	s=s And $FF
	pc=read_6502(s+$100)
	
	s=s+1
	s=s And $FF
	
	pc=pc Or (read_6502(s+$100) Shl 8)
	
End Function

Function eor_6502()

	mode_execute()
	
	a=a Xor read_6502(save_pc)
	
	If a
	
		p=p And $FD
		
	Else
	
		p=p And $2
		
	EndIf
	
	If (a And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf

End Function

Function lsr_6502()
	
	mode_execute()
	
	value=read_6502(save_pc)
	
	If value And $1
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf
	
	value=value Shr 1
	
	write_6502(save_pc,value)
	
	If value
	
		p=p And $FD
		
	Else
	
		p=p And $2
		
	EndIf
	
	If (value And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function lsra_6502()

	If value And $1
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf
	
	a=a Shr 1
	
	If a
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (a And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function jmp_6502()

	mode_execute()
	
	pc=save_pc
	
End Function

Function bvc_6502()

	If (p And $40)=0
	
		mode_execute()
		
		pc=pc+save_pc
		pc=pc And $FFFF
		
		cycles=cycles-1
		
	Else
	
		value=read_6502(pc)
		
		pc=pc+1
		pc=pc And $FFFF
		
	EndIf

End Function

Function cli_6502()

	p=p And $FB

End Function

Function jsr_6502()

	pc=pc+1
	pc=pc And $FFFF
	
	write_6502($100+s,(pc Shr 8))
	s=s-1
	
	s=s And $FF
	
	write_6502($100+s,pc And $FF)
	s=s-1
	
	s=s And $FF
	
	pc=pc-1
	pc=pc And $FFFF
		
	mode_execute()
	pc=save_pc
		
	;debug5
	;emu_status=IDLE
	;cycles=-1
	;Notify "JSR",0
		
End Function

Function rts_6502()

	s=s+1
	s=s And $FF
	pc=read_6502(s+$100)
	
	s=s+1
	s=s And $FF
	pc=(pc Or (read_6502(s+$100) Shl 8))
	
	pc=pc+1
	pc=pc And $FFFF
	
	;Notify Hex$(pc),0
	
End Function

Function adc_6502()
	
	mode_execute()
	
	value=read_6502(save_pc)
	save_flags=(p And $1)
	
	sum=a+value+save_flags
	
	If sum And $FF
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If sum And $80
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
	If Not ((a Xor value) And $80) And ((a Xor sum) And $80)
	
		p=p Or $40
		
	Else
	
		p=p And $BF
		
	EndIf
	
	a=sum
	a=a And $FF
	
End Function

Function rora_6502()
	
	If a And $1
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf
	
	a=a Shr 1
	a=a And $FF
	
	If a
		
		p=p And $FD
		
	Else
	
		p=$2
		
	EndIf
	
	If (a And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function ror_6502()
	
	mode_execute()
	
	value=read_6502(save_pc)
	
	If value And $1
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf
	
	value=value Shr 1
	value=value And $FF
	
	write_6502(save_pc,value)
	
	If value
		
		p=p And $FD
		
	Else
	
		p=$2
		
	EndIf
	
	If (value And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function pla_6502()

	s=s+1
	s=s And $FF
	
	a=read_6502(s+$100)
	
	If a
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (a And $80)
	
		p=p Or $80
	
	Else
	
		p=p And $7F
		
	EndIf

End Function

Function bvs_6502()

	If (p And $40)
	
		mode_execute()
		
		pc=pc+save_pc
		pc=pc And $FFFF
		
		cycles=cycles-1
		
	Else
	
		value=read_6502(pc)
		
		pc=pc+1
		pc=pc And $FFFF
		
	EndIf

End Function

Function sei_6502()

	;p=p or $4 ;hmmmm ???

End Function

Function sta_6502()

	mode_execute()	
	write_6502(save_pc,a)

End Function

Function sty_6502()

	mode_execute()
	write_6502(save_pc,y)
	
End Function

Function stx_6502()

	mode_execute()
	write_6502(save_pc,x)

End Function

Function dey_6502()

	y=y-1
	
	If (y)
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (y <$0)
	
		y = (y + $100) And $FF
		
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function txa_6502()

	a=x
	
	If a
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (a And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
		
End Function

Function bcc_6502()

	If (p And $1)=0
	
		mode_execute()
		
		pc=pc+save_pc
		pc=pc And $FFFF
		
		cycles=cycles-1
		
	Else
	
		value=read_6502(pc)
		pc=pc+1
		
		pc=pc And $FFFF
		
	EndIf
	
End Function

Function tya_6502()

	a=y
	
	If a
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (a And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function tsx_6502()

	x=s
	
	If x
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (x And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function txs_6502()

	s=Abs(x) And $FF
	
End Function

Function ldy_6502()
	
	mode_execute()
	
	y=read_6502(save_pc)
	
	If y
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (y And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf

End Function

Function lda_6502()

	mode_execute()
	
	a=read_6502(save_pc)
	
	If a
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (a And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function ldx_6502()

	mode_execute()
	
	x=read_6502(save_pc)
	
	If x
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (x And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function tay_6502()

	y=a
	
	If y
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (y And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function tax_6502()

	x=a
	
	If x
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (x And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function bcs_6502()

	If (p And $1)

		mode_execute()
		
		pc=pc+save_pc
		pc=pc And $FFFF
		
		cycles=cycles-1
		
	Else
	
		value=read_6502(pc)
		pc=pc+1
		
		pc=pc And $FFFF
		
	EndIf
		
End Function

Function clv_6502()

	p=p And $BF

End Function

Function cpy_6502()

	mode_execute()
	
	value=y - read_6502(save_pc) ;hmmm:bb is this a signed subtract?
	
	If (value<$0) ;because otherwise this will always happen
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf
	
	If value And $FF
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (value < $0)
	
		value = (value + $100) And $FF
		
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf

End Function

Function cmp_6502()

	mode_execute()
	
	value=a - read_6502(save_pc) ;hmmm:bb is this a signed subtract?
	
	If (value<$0) ;because otherwise this will always happen
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf
	
	If value And $FF
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (value < $0)
	
		value = (value + $100) And $FF
		
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf

End Function

Function dec_6502()

	mode_execute()
	
	value=(read_6502(save_pc)-1)
	
	write_6502(save_pc,value)
	
	If value
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (value < $0)
	
		value = (value + $100) And $FF
		
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf

End Function

Function cld_6502()

	p=p And $F7

End Function

Function iny_6502()

	y=(y+1) And $FF
	
	If y
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (y And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf

End Function

Function dex_6502()

	x=(x-1)
	
	If x
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (x < $0)
	
		x = (x + $100) And $FF
		
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function bne_6502()

	If (p And $2)=0
	
		mode_execute()
		
		pc=pc+save_pc
		pc=pc And $FFFF
		
		cycles=cycles-1
		
	Else
	
		value=read_6502(pc)
		pc=pc+1
		
		pc=pc And $FFFF
		
	EndIf
	
End Function

Function cpx_6502()

	mode_execute()
	
	value=x - read_6502(save_pc) ;hmmm:bb is this a signed subtract?
	
	If (value<$0) ;because otherwise this will always happen
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf
	
	If value And $FF
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (value < $0)
	
		value = (value + $100) And $FF
		
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function sbc_6502()

	mode_execute()
	
	value=read_6502(save_pc)
	
	sum=a-value-(p And $1)
	
	If sum<$0 ;hmmm:bb once again, signed subtract?
	
		p=p Or $1
		
	Else
	
		p=p And $FE
		
	EndIf
		
	If (sum <$0)
	
		sum = (sum + $200) And $FF
		
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
	If sum And $FF
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If ((a Xor sum) And $80) And ((a Xor value) And $80)
	
		p=p Or $40
		
	Else
	
		p=p And $BF
		
	EndIf

	a=sum And $FF
		
End Function

Function inc_6502()

	mode_execute()
	
	value=(read_6502(save_pc)+1) And $FF
	write_6502(save_pc,value)
	
	If value
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (value And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function inx_6502()

	x=(x+1) And $FF
	
	If x
	
		p=p And $FD
		
	Else
	
		p=p Or $2
		
	EndIf
	
	If (x And $80)
	
		p=p Or $80
		
	Else
	
		p=p And $7F
		
	EndIf
	
End Function

Function nop_6502()
End Function

Function beq_6502()

	If p And $2
	
		mode_execute()
		
		pc=pc+save_pc
		pc=pc And $FFFF
		
		cycles=cycles-1
		
	Else
	
		value=read_6502(pc)
		pc=pc+1
		
		pc=pc And $FFFF
		
	EndIf
	
End Function

Function sed_6502()

	;p=p or $8

End Function

; this function is only used internally by the ppu

Function nmi_6502()
	
	write_6502($100+s,pc Shr 8)
	s=(s-1) And $FF
	
	write_6502($100+s,pc And $FF)
	s=(s-1) And $FF
	
	write_6502($100+s,p)
	s=(s-1) And $FF
	
	p=p Or $4
	pc=read_6502($FFFA) Or (read_6502($FFFB) Shl 8)
	
	;debug1
	;emu_status=IDLE
	;cycles=-1
	;Notify "NMI",0
	
End Function

; used for external interrupt requests ;via the irq line;
; no the fuck idea what that means

Function irq_6502()
	
	write_6502($100+s,pc Shr 8)
	s=(s-1) And $FF
		
	write_6502($100+s,pc And $FF)
	s=(s-1) And $FF
	
	write_6502($100+s,p)
	s=(s-1) And $FF
	
	p=p Or $4
	pc=read_6502($FFFE) Or (read_6502($FFFF) Shl 8)
			
End Function

; These 2 functions are the 1st main reason PNES
; is as slow as dung:bb The other is that BB doesn;t support 8 and 16 bit signed and unsigned integers:bb

Function write_6502(address,num)
	
	If address<$2000
	
		real_address=address Mod $800
		PokeByte ram,real_address,num
		
	Else If address<$4000
	
		real_address=((address-$2000) Mod $8)+$2000
		
		Select real_address
				
			Case $2000
			
				PokeByte ram,real_address,num
				;vram_address=(vram_address and $
				
			Case $2002
			
				Return
				
			Case $2003
			
				spr_address=num
				
			Case $2004
				
				PokeByte spr_ram,spr_address,num
				spr_address=(spr_address+1) And $FF
				
			Case $2005 ;BROKEN AS FUCK
			
				If load_low
				
					;vram_address=((vram_address And $FC1F) Or ((num And $F8) Shl 2))
					;vram_address=((vram_address And $8FFF) Or ((num And $7) Shl 12))
					;vram_address = vram_address Or num
				
					scroll_y = num
						
				Else
				
					;vram_address=(vram_address And $FFE0) Or ((num And $F8) Shr 3)
					
					;x=d;00000111
					;^--- wtf?
					scroll_x = num
					
				EndIf
				
				load_low=1-load_low
	
				;scroll_y=vram_address and $FF ;temp
				;scroll_x=(vram_address and ($FF Shl 8)) Shr 8 ;temp
				;scroll_x=(vram_address And $FF);*2

				If scroll_x And $80 Then scroll_x = scroll_x - $100
				If scroll_y And $80 Then scroll_y = scroll_y - $100
				
			Case $2006
								
				If (load_low)
				
					last_vram_address=vram_address
					vram_address=(vram_address And $FF00) Or num
					
				Else
				
					vram_address=(vram_address And $FF) Or (num Shl 8)
						
				EndIf
				
				load_low=1-load_low
				
				;scroll_y=vram_address and $FF ;temp
				;scroll_x=(vram_address and ($FF Shl 8)) Shr 8 ;temp
				;scroll_x=(vram_address And $FF);*2				
				
				;If scroll_x And $80 Then scroll_x = scroll_x - $100
				
			Case $2007
				
				write_vram(vram_address,num)
				
				If PeekByte(ram,$2000) And $2
				
					vram_address=(vram_address+32) And $3FFF
					
				Else
				
					vram_address=(vram_address+1) And $3FFF
					
				EndIf
			
			Default
			
				PokeByte ram,real_address,num
				
		End Select
		
	Else If address<$4020
		
		Select address
		
			Case $4003
			
				;emulate APU here
				
			Case $4014
				
				For byte=0 To $100-1
				
					PokeByte spr_ram,byte,read_6502($100*num+byte)
					
				Next
				
				cycles=cycles-512
				
			Case $4015
			
				;emulate APU here
				
			Case $4016 ;need to come back to this for 3p support by increasing joypad1_button range
			
				joypad1_button = 0 ; this seems to work better despite information leading me believe 2 consecutive writes
								   ; are required to reset the button count
				
				;If (value And $1)
				;
				;	If joypad1_strobe=0
				;	
				;		joypad1_strobe=1
				;		
				;	Else
				;	
				;		;expansion port shite?
				;		
				;	EndIf
				;	
				;Else
				;
				;	If joypad1_strobe=1
				;	
				;		joypad1_strobe=0
				;		joypad1_button=0
				;		
				;	Else
				;	
				;		;expansion port shite?
				;		
				;	EndIf
				;	
				;EndIf
				
			Case $4017 ;need to come back to this for 4p support by increasing joypad2_button range
			
				joypad2_button = 0 ; this seems to work better despite information leading me believe 2 consecutive writes
								   ; are required to reset the button count
											
				;If value And $1
				;
				;	If joypad2_strobe=0
				;	
				;		joypad2_strobe=1
				;		
				;	Else
				;	
				;		;expansion port shite?
				;		
				;	EndIf
				;	
				;Else
				;
				;	If joypad2_strobe=1
				;	
				;		joypad2_strobe=0
				;		joypad2_button=0
				;		
				;	Else
				;	
				;		;expansion port shite?
				;		
				;	EndIf
				;	
				;EndIf
				
			Default
			
				PokeByte ram,address,num
				
		End Select
		
	Else If address<$6000
	
		Return ; that;s expansion ROM there, big boy, no touchy
		
	Else If address<$8000 ;sram
	
		PokeByte ram,address,num
		
	EndIf
		
End Function

Function read_6502(address)

	If address<$2000
	
		real_address=address Mod $800
		Return PeekByte(ram,real_address)
		
	Else If address<$4000
	
		real_address=((address-$2000) Mod $8)+$2000
		
		Select real_address
		
			Case $2000
			
				Return
				
			Case $2001
			
				Return
				
			Case $2002
				
				num=PeekByte(ram,$2002)
				
				PokeByte ram,$2002,num And $7F
				PokeByte ram,$2005,0
				PokeByte ram,$2006,0
				
				Return num
				
				load_low=0
				
			Case $2004
			
				num=PeekByte(spr_ram,spr_address)
				spr_address=((spr_address+1) And $FF)
				
				Return num
				
			Case $2003,$2005,$2006
			
				Return
			
			Case $2007
			
				If vram_address>$3F00 ;since pallette ram resides on the internal vram bus it is transmited instantiously
				
					value=read_vram(vram_address)
					
				Else
				
					value=read_vram(last_vram_address)
					
				EndIf
					
				last_vram_address=vram_address
				
				If PeekByte(ram,$2000) And $2
				
					vram_address=(vram_address+32) And $3FFF
					
				Else
				
					vram_address=(vram_address+1) And $3FFF
					
				EndIf
				
				Return value
				
		End Select
		
	Else If address<$4020
	
		Select address
		
			Case $4001,$4002,$4003,$4004,$4005,$4006,$4007,$4008,$4009,$400A,$400B,$400C,$400D,$400E,$400F,$4010,$4011,$4012,$4013
			
				Return
				
			Case $4014
			
				Return
			
			Case $4015
			
				RuntimeError "PEENOOSE"
				
				;emulate APU here
				
			Case $4016
				
				Select joypad1_button
				
					Case A_BUTTON
					
						Select p1_device
						
							Case KEYBOARD
							
								return_value=KeyDown(p1_a_key)
								
							Case JOYSTICK
							
								If p1_joy_port
								
									return_value=JoyDown(p1_joy_a,p1_joy_port)
									
								Else
								
									return_value=JoyDown(p1_joy_a)
									
								EndIf
								
						End Select
						
					Case B_BUTTON
					
						Select p1_device
						
							Case KEYBOARD
							
								return_value=KeyDown(p1_b_key)
								
							Case JOYSTICK
							
								If p1_joy_port
								
									return_value=JoyDown(p1_joy_b,p1_joy_port)
									
								Else
								
									return_value=JoyDown(p1_joy_b)
									
								EndIf
								
						End Select
						
					Case UP_BUTTON
					
						Select p1_device
						
							Case KEYBOARD
							
								return_value=KeyDown(p1_up_key)
								
							Case JOYSTICK
							
								If p1_joy_port
								
									joy_y#=JoyY#(p1_joy_port)
									
								Else
								
									joy_y#=JoyY#()
									
								EndIf
								
								If joy_y#<-p1_y_dead_zone#
								
									return_value=1
									
								Else
								
									return_value=0
									
								EndIf
									
						End Select
						
					Case DOWN_BUTTON
					
						Select p1_device
						
							Case KEYBOARD
							
								return_value=KeyDown(p1_down_key)
								
							Case JOYSTICK
							
								If p1_joy_port
								
									joy_y#=JoyY#(p1_joy_port)
									
								Else
								
									joy_y#=JoyY#()
									
								EndIf
								
								If joy_y#>p1_y_dead_zone#
								
									return_value=1
									
								Else
								
									return_value=0
									
								EndIf
								
						End Select
						
					Case LEFT_BUTTON
					
						Select p1_device
						
							Case KEYBOARD
							
								return_value=KeyDown(p1_left_key)
								
							Case JOYSTICK
							
								If p1_joy_port
								
									joy_x#=JoyX#(p1_joy_port)
									
								Else
								
									joy_x#=JoyX#()
									
								EndIf
								
								If joy_x#<-p1_x_dead_zone#
								
									return_value=1
									
								Else
								
									return_value=0
									
								EndIf
								
						End Select
						
					Case RIGHT_BUTTON
					
						Select p1_device
						
							Case KEYBOARD
							
								return_value=KeyDown(p1_right_key)
								
							Case JOYSTICK
							
								If p1_joy_port
								
									joy_x#=JoyX#(p1_joy_port)
									
								Else
								
									joy_x#=JoyX#()
									
								EndIf
								
								If joy_x#>p1_x_dead_zone#
								
									return_value=1
									
								Else
								
									return_value=0
									
								EndIf
								
						End Select
						
					Case SELECT_BUTTON
					
						Select p1_device
						
							Case KEYBOARD
							
								return_value=KeyDown(p1_select_key)
								
							Case JOYSTICK
							
								If p1_joy_port
								
									return_value=JoyDown(p1_joy_select,p1_joy_port)
									
								Else
								
									return_value=JoyDown(p1_joy_select)
									
								EndIf
								
						End Select
						
					Case START_BUTTON
					
						Select p1_device
						
							Case KEYBOARD
							
								return_value=KeyDown(p1_start_key)
								
							Case JOYSTICK
							
								If p1_joy_port
								
									return_value=JoyDown(p1_joy_start,p1_joy_port)
									
								Else
								
									return_value=JoyDown(p1_joy_start)
									
								EndIf
								
						End Select
						
				End Select
				
				return_value = return_value Or $40
				
				;If (return_value And $1) Then return_value=(return_value Or $40)
				
				joypad1_button=joypad1_button+1
				joypad1_button=joypad1_button And $7
				
				Return return_value
								
			Case $4017
						
				Select joypad2_button-1
				
					Case A_BUTTON
					
						Select p2_device
						
							Case KEYBOARD
							
								Return KeyDown(p2_a_key)
								
							Case JOYSTICK
							
								If p2_joy_port
								
									Return JoyDown(p2_joy_a,p2_joy_port)
									
								Else
								
									Return JoyDown(p2_joy_a)
									
								EndIf
								
						End Select
						
					Case B_BUTTON
					
						Select p2_device
						
							Case KEYBOARD
							
								Return KeyDown(p2_b_key)
								
							Case JOYSTICK
							
								If p2_joy_port
								
									Return JoyDown(p2_joy_b,p2_joy_port)
									
								Else
								
									Return JoyDown(p2_joy_b)
									
								EndIf
								
						End Select
						
					Case UP_BUTTON
					
						Select p2_device
						
							Case KEYBOARD
							
								Return KeyDown(p2_up_key)
								
							Case JOYSTICK
							
								If p2_joy_port
								
									joy_y#=JoyY#(p2_joy_port)
									
								Else
								
									joy_y#=JoyY#()
									
								EndIf
								
								If joy_y#<-p2_y_dead_zone#
								
									Return 1
									
								Else
								
									Return 0
									
								EndIf
									
						End Select
						
					Case DOWN_BUTTON
					
						Select p2_device
						
							Case KEYBOARD
							
								Return KeyDown(p2_down_key)
								
							Case JOYSTICK
							
								If p2_joy_port
								
									joy_y#=JoyY#(p2_joy_port)
									
								Else
								
									joy_y#=JoyY#()
									
								EndIf
								
								If joy_y#>p2_y_dead_zone#
								
									Return 1
									
								Else
								
									Return 0
									
								EndIf
								
						End Select
						
					Case LEFT_BUTTON
					
						Select p2_device
						
							Case KEYBOARD
							
								Return KeyDown(p2_left_key)
								
							Case JOYSTICK
							
								If p2_joy_port
								
									joy_x#=JoyX#(p2_joy_port)
									
								Else
								
									joy_x#=JoyX#()
									
								EndIf
								
								If joy_x#<-p2_x_dead_zone#
								
									Return 1
									
								Else
								
									Return 0
									
								EndIf
								
						End Select
						
					Case RIGHT_BUTTON
					
						Select p2_device
						
							Case KEYBOARD
							
								Return KeyDown(p2_right_key)
								
							Case JOYSTICK
							
								If p2_joy_port
								
									joy_x#=JoyX#(p2_joy_port)
									
								Else
								
									joy_x#=JoyX#()
									
								EndIf
								
								If joy_x#>p2_x_dead_zone#
								
									Return 1
									
								Else
								
									Return 0
									
								EndIf
								
						End Select
						
					Case SELECT_BUTTON
					
						Select p2_device
						
							Case KEYBOARD
							
								Return KeyDown(p2_select_key)
								
							Case JOYSTICK
							
								If p2_joy_port
								
									Return JoyDown(p2_joy_select,p2_joy_port)
									
								Else
								
									Return JoyDown(p2_joy_select)
									
								EndIf
								
						End Select
						
					Case START_BUTTON
					
						Select p2_device
						
							Case KEYBOARD
							
								Return KeyDown(p2_start_key)
								
							Case JOYSTICK
							
								If p2_joy_port
								
									Return JoyDown(p2_joy_start,p2_joy_port)
									
								Else
								
									Return JoyDown(p2_joy_start)
									
								EndIf
								
						End Select
						
				End Select
				
				return_value = return_value Or $40
				
				;If (return_value And $1) Then return_value=(return_value Or $40)
				
				joypad2_button=joypad2_button+1
				joypad2_button=joypad2_button And $7
								
		End Select
		
	Else If address<$10000
	
		Return PeekByte(ram,address)
			
	EndIf
	
End Function

; other functions

Function mode_execute()

	Select op_mode(op_code)
	
		Case MODE_RELATIVE
		
			relative_6502()
			
		Case MODE_IMMEDIATE
		
			immediate_6502()
			
		Case MODE_ABSOLUTE
		
			absolute_6502()
			
		Case MODE_ABSOLUTE_X
		
			absolute_x_6502()
			
		Case MODE_ABSOLUTE_Y
		
			absolute_y_6502()
			
		Case MODE_INDIRECT
		
			indirect_6502()
			
		Case MODE_INDIRECT_X
		
			indirect_x_6502()
			
		Case MODE_INDIRECT_Y
		
			indirect_y_6502()
			
		Case MODE_INDIRECT_ZEROPAGE
		
			indirect_zeropage_6502()
			
		Case MODE_INDIRECT_ABSOLUTE_X
		
			indirect_absolute_x_6502()
			
		Case MODE_ZEROPAGE
		
			zeropage_6502()
			
		Case MODE_ZEROPAGE_X
		
			zeropage_x_6502()
			
		Case MODE_ZEROPAGE_Y
		
			zeropage_y_6502()
			
	End Select
	
End Function
	
Function cpu_execute()
	
	Repeat
		
		op_code=read_6502(pc)
		
		If debug_mode And emu_status=IDLE
	
			AddGadgetItem op_list,"PC:$"+Right$(Hex$(pc),4)+" OP:$"+Right$(Hex$(op_code),2)
			AddGadgetItem op_list,"X$:"+Right$(Hex$(x),2)+" Y$:"+Right$(Hex$(y),2)+" A$:"+Right$(Hex$(a),2)+" P$:"+Right$(Hex$(p),2)+" S$:"+Right$(Hex$(s),2)
			AddGadgetItem op_list,""
			
		EndIf
					
		pc=pc+1
		pc=pc And $FFFF
		
		cycles=cycles-op_cycles(op_code)
		
		Select op_code
		
			Case BRK
			
				brk_6502()
				
			Case ORA_IX
			
				ora_6502()
				
			Case ORA_ZP
			
				ora_6502()
				
			Case ASL_ZP
			
				asl_6502()
				
			Case PHP
			
				php_6502()
				
			Case ORA_IM
			
				ora_6502()
				
			Case ASL_ACM
			
				asla_6502()
				
			Case ORA_ABS
			
				ora_6502()
				
			Case ASL_ABS
			
				asl_6502()
				
			Case BPL
				
				bpl_6502()
				
			Case ORA_IY
			
				ora_6502()
			
			Case ORA_ZPX
			
				ora_6502()
				
			Case ASL_ZPX
			
				asl_6502()
				
			Case CLC
			
				clc_6502()
				
			Case ORA_ABSY
			
				ora_6502()
				
			Case ORA_ABSX
			
				ora_6502()
				
			Case ASL_ABSX
			
				asl_6502()
				
			Case JSR
			
				jsr_6502()
				
			Case AND_IX
			
				and_6502()
				
			Case BIT_ZP
			
				bit_6502()
				
			Case AND_ZP
			
				and_6502()
				
			Case ROL_ZP
			
				rol_6502()
				
			Case PLP
			
				plp_6502()
				
			Case AND_IM
			
				and_6502()
				
			Case ROL_ACM
			
				rola_6502()
				
			Case BIT_ABS
			
				bit_6502()
				
			Case AND_ABS
			
				and_6502()
				
			Case ROL_ABS
			
				rol_6502()
				
			Case BMI
			
				bmi_6502()
				
			Case AND_IY
			
				and_6502()
				
			Case AND_ZPX
			
				and_6502()
				
			Case ROL_ABSX
			
				rol_6502()
				
			Case RTI
			
				rti_6502()
				
			Case EOR_IX
			
				eor_6502()
				
			Case EOR_ZP
			
				eor_6502()
				
			Case LSR_ZP
			
				lsr_6502()
				
			Case PHA
			
				pha_6502()
				
			Case EOR_IM
			
				eor_6502()
				
			Case LSR_ACM
			
				lsra_6502()
				
			Case JMP_ABS
			
				jmp_6502()
				
			Case EOR_ABS
			
				eor_6502()
				
			Case LSR_ABS
			
				lsr_6502()
				
			Case BVC
			
				bvc_6502()
				
			Case EOR_IY
			
				eor_6502()
				
			Case EOR_ZPX
			
				eor_6502()
				
			Case LSR_ZPX
			
				lsr_6502()
				
			Case CLI
			
				cli_6502()
				
			Case EOR_ABSY
			
				eor_6502()
				
			Case EOR_ABSX
			
				eor_6502()
				
			Case LSR_ABSX
			
				lsr_6502()
				
			Case RTS
			
				rts_6502()
				
			Case ADC_IX
			
				adc_6502()
				
			Case ROR_ZP
			
				ror_6502()
				
			Case PLA
			
				pla_6502()
				
			Case ADC_IM
			
				adc_6502()
				
			Case ROR_ACM
			
				rora_6502()
				
			Case JMP_I
			
				jmp_6502()
				
			Case ADC_ABS
			
				adc_6502()
				
			Case ROR_ABS

				ror_6502()
				
			Case BVS
			
				bvs_6502()
				
			Case ADC_IY
			
				adc_6502()
				
			Case ADC_ZPX
			
				adc_6502()
				
			Case ROR_ZPX
			
				ror_6502()
				
			Case SEI
			
				sei_6502()
				
			Case ADC_ABSY
			
				adc_6502()
				
			Case ADC_ABSX
			
				adc_6502()
				
			Case ROR_ABSX
				
				ror_6502()
				
			Case STA_IX
			
				sta_6502()
				
			Case STY_ZP
			
				sty_6502()
				
			Case STA_ZP
			
				sta_6502()
				
			Case STX_ZP
				
				stx_6502()
				
			Case DEY
			
				dey_6502()
				
			Case TXA
			
				txa_6502()
				
			Case STY_ABS
			
				sty_6502()
				
			Case STA_ABS
			
				sta_6502()
				
			Case STX_ABS

				stx_6502()
				
			Case BCC
			
				bcc_6502()
				
			Case STA_IY
			
				sta_6502()
				
			Case STY_ZPX
			
				sty_6502()
				
			Case STA_ZPX
			
				sta_6502()
				
			Case STX_ZPY
			
				stx_6502()
				
			Case TYA
			
				tya_6502()
				
			Case STA_ABSY
			
				sta_6502()
				
			Case TXS
			
				txs_6502()
				
			Case STA_ABSX
			
				sta_6502()
				
			Case LDY_IM
			
				ldy_6502()
				
			Case LDA_IX
			
				lda_6502()
				
			Case LDX_IM
			
				ldx_6502()
				
			Case LDY_ZP
			
				ldy_6502()
				
			Case LDA_ZP
			
				lda_6502()
				
			Case LDX_ZP
			
				ldx_6502()
				
			Case TAY
			
				tay_6502()
				
			Case LDA_IM
			
				lda_6502()
				
			Case TAX
			
				tax_6502()
				
			Case LDA_IM

				lda_6502()
				
			Case TAX
			
				tax_6502()
				
			Case LDY_ABS
			
				ldy_6502()
				
			Case LDA_ABS
			
				lda_6502()
				
			Case LDX_ABS
			
				ldx_6502()
				
			Case BCS
			
				bcs_6502()
				
			Case LDA_IY
			
				lda_6502()
				
			Case LDY_ZPX
			
				ldy_6502()
				
			Case LDA_ZPX
			
				lda_6502()
				
			Case LDX_ZPY
			
				ldx_6502()
				
			Case CLV
			
				clv_6502()
				
			Case LDA_ABSY
			
				lda_6502()
				
			Case TSX
			
				tsx_6502()
				
			Case LDY_ABSX
			
				ldy_6502()
				
			Case LDA_ABSX
			
				lda_6502()
				
			Case LDX_ABSY
			
				ldx_6502()
				
			Case CPY_IM
			
				cpy_6502()
				
			Case CMP_IX
			
				cmp_6502()
				
			Case CPY_ZP
			
				cpy_6502()
				
			Case CMP_ZP

				cmp_6502()
							
			Case DEC_ZP

				dec_6502()
				
			Case INY
			
				iny_6502()
				
			Case CMP_IM
			
				cmp_6502()
				
			Case DEX
			
				dex_6502()
				
			Case CPY_ABS
			
				cpy_6502()
				
			Case CMP_ABS
			
				cmp_6502()
				
			Case DEC_ABS
			
				dec_6502()
				
			Case BNE
			
				bne_6502()
				
			Case CMP_IY
			
				cmp_6502()
				
			Case CMP_ZPX
			
				cmp_6502()
				
			Case DEC_ZPX
			
				dec_6502()
				
			Case CLD
			
				cld_6502()
				
			Case CPX_IM
			
				cpx_6502()
				
			Case SBC_IX
			
				sbc_6502()
				
			Case CPX_ZP
			
				cpx_6502()
				
			Case SBC_ZP
			
				sbc_6502()
				
			Case INC_ZP
			
				inc_6502()
				
			Case INX
			
				inx_6502()
				
			Case SBC_IM

				sbc_6502()
				
			Case NOP
			
				nop_6502()
				
			Case CPX_ABS
			
				cpx_6502()
				
			Case SBC_ABS
			
				sbc_6502()
				
			Case INC_ABS
			
				inc_6502()
				
			Case BEQ
			
				beq_6502()
				
			Case SBC_IY

				sbc_6502()
				
			Case SBC_ZPX
			
				sbc_6502()
				
			Case INC_ZPX
			
				inc_6502()
				
			Case SED
			
				sed_6502()
				
			Case SBC_ABSY
			
				sbc_6502()
				
			Case SBC_ABSX
			
				sbc_6502()
				
			Case INC_ABSX

				inc_6502()
				
		End Select
		
		If cycles<0 Then Exit
		
	Forever
	
End Function

Function reset()

	;get entry point
	pc=read_6502($FFFC)+(read_6502($FFFD) Shl 8)
	
	;reset cpu regs
	x=0
	y=0
	a=0
	;s=$FF
	s=$FD
	;p=$20
	p=$04
	
	;reset internals
	joypad1_strobe=0
	joypad2_strobe=0
	joypad1_button=0
	joypad2_button=0
	
	;reset ppu internals
	scroll_x=0
	scroll_y=0
	last_vram_address=0
	vram_address=0
	spr_address=0
	load_low=0
	ppu_regs=0
	
End Function