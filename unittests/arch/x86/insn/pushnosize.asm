[bits 16]
push 0			; 6A 00 - equivalent to push byte 0
push byte 0		; 6A 00
push word 0		; 6A 00 - optimized
push dword 0		; 66 6A 00 - optimized
push strict byte 0	; 6A 00
push strict word 0	; 68 00 00
push strict dword 0	; 66 68 00 00 00 00
push 128		; 68 80 00 - doesn't fit in byte, equivalent to push word 128
push byte 128		; 6A 80 [warning: value does not fit in signed 8 bit field]
push word 128		; 68 80 00
push dword 128		; 66 68 80 00 00 00
push strict byte 128	; 6A 80 [warning: value does not fit in signed 8 bit field]
push strict word 128	; 68 80 00
push strict dword 128	; 66 68 80 00 00 00

[bits 32]
push 0			; 6A 00 - equivalent to push byte 0
push byte 0		; 6A 00
push word 0		; 66 6A 00 - optimized
push dword 0		; 6A 00 - optimized
push strict byte 0	; 6A 00
push strict word 0	; 66 68 00 00
push strict dword 0	; 68 00 00 00 00
push 128		; 68 80 00 00 00 - doesn't fit in byte -> push dword 128
push byte 128		; 6A 80 [warning: value does not fit in signed 8 bit field]
push word 128		; 66 68 80 00
push dword 128		; 68 80 00 00 00
push strict byte 128	; 6A 80 [warning: value does not fit in signed 8 bit field]
push strict word 128	; 66 68 80 00
push strict dword 128	; 68 80 00 00 00

[bits 64]
push 0			; 6A 00 - same as bits 32 output
push byte 0		; 6A 00 - 64 bits pushed onto stack
push word 0		; 66 6A 00 - 66h prefix, optimized to byte
push dword 0		; 6A 00 - optimized to byte; note 64 bits pushed onto stack
push strict byte 0	; 6A 00 - 64 bits pushed onto stack
push strict word 0	; 66 68 00 00
push strict dword 0	; 68 00 00 00 00 - note 64 bits pushed onto stack
push 128		; 68 80 00 00 00
push byte 128		; 6A 80 [warning: value does not fit in signed 8 bit field]
push word 128		; 66 68 80 00
push dword 128		; 68 80 00 00 00
push strict byte 128	; 6A 80 [warning: value does not fit in signed 8 bit field]
push strict word 128	; 66 68 80 00
push strict dword 128	; 68 80 00 00 00
