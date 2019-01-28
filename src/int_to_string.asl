#define STACK              0x453
#define BRIDGE_FUNC        0x101D
#define BRIDGE_PRINT_ADDR  0x0
#define BRIDGE_PRINT       0x1
#define BRIDGE_PRINT_BUF   0x2
#define BRIDGE_READ        0x3
#define BRIDGE_READ_CH     0x4
#define BRIDGE_DELAY       0x5
#define NULL               0x0

; 16 bytes for "registers"
#define RegA 0x453
#define RegB 0x455
#define RegC 0x457
#define RegD 0x459
#define RegE 0x461
#define RegF 0x463
#define RegG 0x465
#define RegH 0x467

jmp START

; int_to_string(int, char*)
sub_int_to_string:
	pop  NULL
	; RegE => char*
	pop  RegE
	; RegE => char
	memderef RegE, RegE
	; RegA => int
	pop  RegA
	push RegB, 0
	push RegC, 10
	; RegD => '0'
	memwrite RegD, 1, "0"
	push RegD, RegD
	
	sub_int_to_string_loop:
		eq RegB, RegA, RegB, 2
		jmpif sub_int_to_string_loop_end
		
		div RegA, RegB, RegA, RegC
		add RegB, RegB, RegD
		memcpy RegE, RegB, 1
		add RegE, RegE, 1
		
		jmp sub_int_to_string_loop
		
	sub_int_to_string_loop_end:
	memwrite RegE, 1, 0
	
	pop NULL
	ret

START:

	alloc RegF, 32
	
	push NULL, 12
	push RegF
	
	call sub_int_to_string

	ncall BRIDGE_FUNC, NULL, 2, BRIDGE_PRINT, RegF
	
	
