MAIN    LDX   	ZERO
		JSUB 	INLOOP
		.
INLOOP 	RD 		INDEV
		COMP  	EOF
		JEQ     ENDDING		
		COMP	NUMEND
		JLT   	ISNUM
        WD   	OUTDEV
		JSUB  	INLOOP
		
ISNUM	COMP  	EOF
		JEQ     ENDDING
		ADD     NUM
		WD   	OUTDEV
		JSUB  	INLOOP
		
ENDDING RSUB
				
INDEV 	BYTE 	X'F1' 
OUTDEV 	BYTE 	X'F2' 
DATA 	RESB 	10
EOF     WORD   	36
NUMEND  WORD	58
NUM     WORD    49
ZERO 	WORD	0
TEMP1   RESW  	1
TEMP2   RESW  	1


