// Another shitty language (ASL)

/*

Supported operations on the VM:

[result_value is used to return pointer to result of an operation]

* memcpy [dest_ptr:int], [source_ptr:int], [size:int]
 - *ptr = value
 - referencing **ptr = *other_ptr
* memref [dest_ptr_ptr:int], [source_ptr:int], [size:int]
 - derefering *ptr = **other_ptr
* memderef [dest_ptr:int], [source_ptr_ptr:int], [size:int]
 - *ptr = [data]
* memwrite [dest_ptr:int}, [size:int], [[data:byte]]

 - Fill block with [value] byte's
* memset [dest_ptr:int], [size:int], [value:byte]

* alloc [result_ptr:int], [size:int]
* free [ptr]

 - int (*) (int, int*)
* native_call [callable_ptr:int], [result_ptr:int], [argc:int], [[argv:int]]
* call [code_address:int], [argc:int], [[argv:int]]
 - Stack: (cdecl blyat :D)
  |----------------|
  | RETURN ADDRESS |
  |      ARG0      |
  |      ARG1      |
  |      ....      |
  |      ARGN      |
  |      ARGC      |
  |----------------|
* ret
 - pop ARGC
 - pop [ARGC] elements
 - pop [RETURN ADDRESS]
 - jmp [RETURN ADDRESS]

 -- Next operators working with pointer-to-integer addresses
 
* jmpif [value_ptr], [true_node]
* jmpifelse [value_ptr], [true_node], [false_node]
* jmp [code_address]

* add [result_ptr], [op1_ptr], [op2_ptr]
* sub [result_ptr], [op1_ptr], [op2_ptr]
* mul [result_ptr], [op1_ptr], [op2_ptr]
* div [result_value_ptr], [result_mod_ptr], [op1_ptr], [op2_ptr]

* bit_and [result_ptr], [op1_ptr], [op2_ptr], [size]
* bit_or [result_ptr], [op1_ptr], [op2_ptr], [size]
* bit_xor [result_ptr], [op1_ptr], [op2_ptr], [size]
* bit_not [result_ptr], [op_ptr], [size]

* and [result_ptr], [op1_ptr], [op2_ptr], [size]
* or [result_ptr], [op1_ptr], [op2_ptr], [size]
* not [result_ptr], [op_ptr], [size]

* eq[result_ptr], [op1_ptr], [op2_ptr], [size]
* neq[result_ptr], [op1_ptr], [op2_ptr], [size]

* gt[result_ptr], [op1_ptr], [op2_ptr], [size]
* ge[result_ptr], [op1_ptr], [op2_ptr], [size]
* lt[result_ptr], [op1_ptr], [op2_ptr], [size]
* le[result_ptr], [op1_ptr], [op2_ptr], [size]

Additional:

* catch [try_node], [catch_node], [result_ptr]
* throw [value_ptr]

 - result points to stack position
* push [result_ptr:int], [value_ptr], [size]
* pop [result_ptr:int], [value_ptr], [size]

*/

#include <SPI.h>
#include <SD.h>
#include <avr/pgmspace.h>

// Block of pre-defined constants

#define METAKEY     0xEBA1
#define EOF             -1
#define MEMREF           6
#define MEMDEREF         7
#define MEMWRITE         8
#define MEMCPY           9
#define MEMSET          10
#define ALLOC           11
#define FREE            12
#define NATIVE_CALL     13
#define CALL            14
#define JMPIF           15
#define JMPIFELSE       16
#define JMP             17
#define ADD             18
#define SUB             19
#define MUL             20
#define DIV             21
#define BIT_AND         22
#define BIT_OR          23
#define BIT_XOR         24
#define BIT_NOT         25
#define AND             26
#define OR              27
#define NOT             28
#define PUSH            29
#define POP             30
#define CATCH           31
#define THROW           32
#define EQ              33
#define NEQ             34
#define GT              35
#define GE              36
#define LT              37
#define LE              38
#define RET             39
#define NONE            40

#define BAUD_RATE     9600
#define SD_PIN           4
#define STACK_SIZE     512

#define BOOTFILE        "ASL"

enum SOURCE_TYPE {
	SOURCE_STREAM,       // Stream from a file
	SOURCE_ARRAY,        // Direct from passed array
	SOURCE_NONE
};

struct SOURCE {
	SOURCE_TYPE type = SOURCE_NONE;
	File        file;
	char      *array = 0;
	int         size = 0;
	int       cursor = 0;
	bool       alloc = 0;
	
	int address      = 0;
} source;

struct STACK {
	unsigned char stack[STACK_SIZE];
	int size = 0;
	
	// Next operations returns -1 on failture and unsigned char on success
	
	int push(char *buf, int size, bool reversed = 0) {
		if (this->size + size >= STACK_SIZE)
			return 0;
		
		for (int i = 0; i < size; ++i)
			push(buf ? buf[reversed ? size - i - 1 : i] : 0);
	};
	
	int pop(char *buf, int size) {
		if (this->size - size < 0)
			return 0;
		
		for (int i = 0; i < size; ++i)
			if (buf)
				buf[i] = pop();
			else
				pop();
	};
	
	int push(char value) {
		if (size == STACK_SIZE) 
			return -1;
		
		stack[size++] = value;
	};
	
	int pop() {
		if (size == 0)
			return -1;
		
		return stack[size--];
	};
} stack;

void (*reset_func) (void) = 0;

#define SOURCE_BROKEN        13
#define STACK_OVERFLOW       14
#define STACK_UNDERFLOW      15
#define END_OF_PROGRAM       16
#define DIVIDE_BY_ZERO       17
#define UNEXPECTED_COMMAND   18
#define ADDRESS_OUT_OF_RANGE 19
#define BOOTLOADER_FAIL      20
#define SD_FAILED            21
#define NO_SOURCE            22

#define PRINT_DUMP_ON_EXCEPTION
#define PRINT_EXCEPTION

// If assembler resulta layout like that:
// 0xABCDEF => 
// | 00 | 01 | 02 |
//   AB   CD   EF => incorrect matching int{ [0]=0xAB, [1]=0xCD, [2]=0xEF }
// Use this key
// #define VM_LITTLE_ENDIAN
// Else:
// | 00 | 01 | 02 |
//   EF   CD   AB

void printint(int n, int size, int base, char fill) {
	if (n < 0) {
		--size;
		Serial.print('-');
		n = -n;
	}
	
	char buf[size];
	int i = 0;
	while (n) {
		buf[i] = n % base;
		if (base > 10 && buf[i] > 9)
			buf[i] += 'A' - 10;
		else
			buf[i] += '0';
			
		n /= base;
		++i;
	}
	for (; i < size; ++i)
		buf[i] = fill;
	
	for (i = 0; i < size; ++i)
		Serial.print((char) buf[size - i - 1]);
};

void exception(char code) {
#ifdef PRINT_EXCEPTION

	if (code == SOURCE_BROKEN) {
		Serial.print(F("SOURCE_BROKEN [0x"));
		Serial.print(source.address, 16);
		Serial.println(F("]"));
	}
	if (code == STACK_OVERFLOW)
		Serial.println(F("STACK_OVERFLOW"));
	if (code == STACK_UNDERFLOW)
		Serial.println(F("STACK_UNDERFLOW"));
	if (code == END_OF_PROGRAM) {
		Serial.print(F("END_OF_PROGRAM [0x"));
		Serial.print(source.address, 16);
		Serial.println(F("]"));
	}
	if (code == DIVIDE_BY_ZERO)
		Serial.println(F("DIVIDE_BY_ZERO"));
	if (code == UNEXPECTED_COMMAND)
		Serial.println(F("UNEXPECTED_COMMAND"));
	if (code == ADDRESS_OUT_OF_RANGE)
		Serial.println(F("ADDRESS_OUT_OF_RANGE"));
	if (code == BOOTLOADER_FAIL)
		Serial.println(F("BOOTLOADER_FAIL"));
	if (code == SD_FAILED)
		Serial.println(F("SD_FAILED"));
	if (code == NO_SOURCE)
		Serial.println(F("NO_SOURCE"));

#endif

#ifdef PRINT_DUMP_ON_EXCEPTION

	if (source.type == SOURCE_ARRAY) {
		Serial.print(F("Array layout: ["));
		Serial.print(source.size);
		Serial.println(']');
		
		Serial.print(F("----"));
		for (int i = 0; i < 16; ++i) {
			Serial.print(F(" | "));
			printint(i, 2, 16, '0');
		}
		
		for (int i = 0; i < source.size; ++i) {
			if (i % 16 == 0) {
				Serial.println();
				printint(i / 16, 4, 16, '0');
			}
			
			Serial.print(F(" | "));
			printint(source.array[i] & 0xFF, 2, 16, '0');;
		}		
		
		Serial.println();
	}
	if (source.type == SOURCE_STREAM) {
		Serial.print(F("File layout: ["));
		source.file.seek(0);
		int file_size = source.file.size();
		Serial.print(file_size);
		Serial.println(']');
		
		Serial.print(F("----"));
		for (int i = 0; i < 16; ++i) {
			Serial.print(F(" | "));
			printint(i, 2, 16, '0');
		}
		
		for (int i = 0; i < file_size; ++i) {
			if (i % 16 == 0) {
				Serial.println();
				printint(i / 16, 4, 16, '0');
			}
			
			Serial.print(F(" | "));
			char c;
			source.file.read(&c, 1);
			printint(c & 0xFF, 2, 16, '0');;
		}		
		
		Serial.println();
	}

#endif
	
	delay(1000);
	reset_func();
};

bool read(char *buf, int size) {
	if (size == 0)
		return 1;
	
	if (source.type == SOURCE_ARRAY)
		if (source.cursor + size > source.size)
			return 0;
		else {
			for (int i = 0; i < size; ++i)
				buf[i] = source.array[source.cursor + i];

#ifdef VM_BIG_ENDIAN
			for (int i = 0; i < size / 2; ++i) {
				char temp = buf[i];
				buf[i] = buf[size - i - 1];
				buf[size - i - 1] = temp;
			}
#endif

			source.cursor += size;
			
			source.address += size;
			return 1;
		}
	else {
		if (source.file.read(buf, size) != -1)
			return 0;

#ifdef VM_BIG_ENDIAN
			for (int i = 0; i < size / 2; ++i) {
				char temp = buf[i];
				buf[i] = buf[size - i - 1];
				buf[size - i - 1] = temp;
			}
#endif
			
		source.address += size;
		
		return 1;
	}
};

void jump(int address) {
	if (source.type == SOURCE_ARRAY) {
		if (source.size <= address)
			exception(ADDRESS_OUT_OF_RANGE);
		source.cursor  = address;
		source.address = address;
	} else {
		if (!source.file.seek(address))
			exception(ADDRESS_OUT_OF_RANGE);
		source.address = address;
	}
};

bool file_to_array(char *filename) {
	File boot = SD.open(filename);
	
	if (!boot)
		return 0;
	
	if (source.type == SOURCE_ARRAY && source.alloc) 
		free(source.array);
	else if (source.type == SOURCE_STREAM)
		source.file.close();
	
	source.type  = SOURCE_ARRAY;
	source.size  = boot.size();
	source.array = malloc(source.size);
	source.alloc = 1;
	source.cursor = 0;
	source.address = 0;
	
	if (source.array == NULL) {
		boot.close();
		return 0;
	}
	
	boot.read(source.array, source.size);
	
	boot.close();
	
	return 1;
};

bool load_file(char *filename) {
	if (source.type == SOURCE_ARRAY && source.alloc) 
		free(source.array);
	else if (source.type == SOURCE_STREAM)
		source.file.close();
	
	source.file = SD.open(filename);
	
	if (!source.file)
		return 0;
	
	source.type = SOURCE_STREAM;
	source.address = 0;
	
	return 1;
};

#define BRIDGE_PRINT_ADDR  0
#define BRIDGE_PRINT       1
#define BRIDGE_PRINT_BUF   2
#define BRIDGE_READ        3
#define BRIDGE_READ_CH     4
#define BRIDGE_DELAY       5

int bridge_func(int argc, int *argv) {
	if (argc == 0)
		return 0;
	
	if (argv[0] == BRIDGE_PRINT_ADDR) {	// Prints out all addreses used in VM	
										// Also, put here output of all functions & constants that're used in there
		Serial.print(F("#: ")); Serial.print(F("sizeof(int) = 0x")); Serial.println(sizeof(int), 16);
		Serial.print(F("#: ")); Serial.print(F("stack_addr  = 0x")); Serial.println((int) stack.stack, 16);
		Serial.print(F("#: ")); Serial.print(F("bridge_func = 0x")); Serial.println((int) bridge_func, 16);
		return 1;
	} 
	if (argv[0] == BRIDGE_PRINT) { // Print argv[1] as regular c string
		if (argc < 2)
			return 0;
		
		Serial.print((char*) argv[1]);
	}
	if (argv[0] == BRIDGE_PRINT_BUF) { // Print argv[1] as character buffer sizeof argv[1]
		if (argc < 3)
			return 0;
		
		for (int i = 0; i < argv[1]; ++i)
			Serial.print(((char*) argv[2])[i]);
	}
	if (argv[0] == BRIDGE_READ) { // Read argv[1] chars into argv[2]
		if (argc < 3)
			return 0;
		
		for (int i = 0; i < argv[1]; ++i)
			((char*) argv[1])[i] = Serial.read();
	}
	if (argv[0] == BRIDGE_READ_CH) { // Read single character and write it to argv[1] addr
		if (argc < 2)
			return 0;
		
		*(char*) argv[1] = Serial.read();
	}
	if (argv[0] == BRIDGE_DELAY)
		if (argc < 2)
			return 0;
		else
			delay(argv[1]);
	return 1;
};

void setup() {	
	Serial.begin(BAUD_RATE);
	
	Serial.print(F("#: ")); Serial.print(F("sizeof(int) = 0x")); Serial.println(sizeof(int), 16);
	Serial.print(F("#: ")); Serial.print(F("stack_addr  = 0x")); Serial.println((int) stack.stack, 16);
	Serial.print(F("#: ")); Serial.print(F("bridge_func = 0x")); Serial.println((int) bridge_func, 16);
	
	// Boot directly from SD
	if (!SD.begin(SD_PIN)) {
		Serial.print(F("#: ")); Serial.println(F("SD init failed"));
		exception(SD_FAILED);
	}
	
	Serial.print(F("#: ")); Serial.println(F("SD init done"));
	
	load_file(BOOTFILE);
};

void loop() {
	// owo
	
	if (source.type == SOURCE_NONE)
		exception(NO_SOURCE);
	
	while (1) {
		char c;
		if (!read(&c, 1))
			exception(END_OF_PROGRAM);
		
		if (c == EOF) { // What's next..?
			exception(END_OF_PROGRAM);
		} else if (c == MEMCPY) {
			int dest, source, size;
			if (!read((char*) &dest, sizeof(int))
				||
				!read((char*) &source, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
				
			for (int i = 0; i < size; ++i)
				((char*) dest)[i] = ((char*) source)[i];
		} else if (c == MEMREF) {
			int dest_pp, source, size;
			if (!read((char*) &dest_pp, sizeof(int))
				||
				!read((char*) &source, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
				
			for (int i = 0; i < size; ++i)
				(*(char**) dest_pp)[i] = ((char*) source)[i];
		} else if (c == MEMDEREF) {
			int dest_p, source, size;
			if (!read((char*) &dest_p, sizeof(int))
				||
				!read((char*) &source, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
				
			for (int i = 0; i < size; ++i)
				((char*) dest_p)[i] = (*(char**) source)[i];
		} else if (c == MEMSET) {
			int dest, size;
			char value;
			if (!read((char*) &dest, sizeof(int))
				||
				!read((char*) &size, sizeof(int))
				||
				!read((char*) &value, sizeof(char)))
				exception(SOURCE_BROKEN);
			
			for (int i = 0; i < size; ++i)
				((char*) dest)[i] = value;
		} else if (c == MEMWRITE) {
			int dest, size;
			if (!read((char*) &dest, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			if (!read(dest, size))
				exception(SOURCE_BROKEN);
		} else if (c == ALLOC) {
			int ptr, size;
			if (!read((char*) &ptr, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			*(int*) ptr = malloc(size);
		} else if (c == FREE) {
			int ptr;
			if (!read((char*) &ptr, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			free(ptr);
		} else if (c == NATIVE_CALL) {
			int ptr, result_ptr, argc;
			if (!read((char*) &ptr, sizeof(int))
				||
				!read((char*) &result_ptr, sizeof(int))
				||
				!read((char*) &argc, sizeof(int))) 
				exception(SOURCE_BROKEN);
				
			int ptrarray[argc];
			read((char*) ptrarray, argc * sizeof(int));
			
			int argv[argc];
			for (int i = 0; i < argc; ++i) {
				read((char*) (argv + i), sizeof(int));
			}
			
			int (*func) (int, int*) = ptr;
			
			int result = func(argc, ptrarray);
			if (result_ptr)
				*(int*) result_ptr = result;
		} else if (c == CALL) {
			int code_address, argc;
			if (!read((char*) &code_address, sizeof(int))
				||
				!read((char*) &argc, sizeof(int)))
				exception(SOURCE_BROKEN);
				
			if (stack.size + (argc + 2) * sizeof(int) >= STACK_SIZE)
				exception(STACK_OVERFLOW);
			
			stack.push((char*) &source.address, sizeof(int)); // Return Address
			
			for (int i = 0; i < argc; ++i) { // Reversed reversed arguments
				int argv;
				if (!read((char*) &argv, sizeof(int)))
					exception(SOURCE_BROKEN);
				
				stack.push((char*) &argv, sizeof(int), 1);
			}
			
			for (int i = 0; i < argc * sizeof(int); ++i) { // Reverse reverse = normal
				char temp = stack.stack[stack.size - i - 1];
				stack.stack[stack.size - i - 1] = stack.stack[stack.size - argc * sizeof(int) + i];
				stack.stack[stack.size - argc * sizeof(int) + i] = temp;
			}
			
			stack.push((char*) &argc, sizeof(int)); // argc
			
			jump(code_address);
		} else if (c == RET) {
			int argc;
			int ret;
			if (!(stack.pop((char*) &argc, sizeof(int)) && stack.pop(NULL, argc * sizeof(int)) && stack.pop((char*) &ret, sizeof(int))))
				exception(STACK_UNDERFLOW);
			
			jump(ret);
		} else if (c == JMPIF) {
			int cond, addr;
			if (!read((char*) &cond, sizeof(int))
				||
				!read((char*) &addr, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			if (cond && *(int*) cond)
				jump(addr);
		} else if (c == JMPIFELSE) {
			int cond, addr_true, addr_false;
			if (!read((char*) &cond, sizeof(int))
				||
				!read((char*) &addr_true, sizeof(int))
				||
				!read((char*) &addr_false, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			if (cond && *(int*) cond)
				jump(addr_true);
			else
				jump(addr_false);
		} else if (c == JMP) {
			int addr;
			if (!read((char*) &addr, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			jump(addr);
		} else if (c == ADD) {
			int result, op1, op2;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			*(int*) result = *(int*) op1 + *(int*) op2;
		} else if (c == SUB) {
			int result, op1, op2;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			*(int*) result = *(int*) op1 - *(int*) op2;
		} else if (c == MUL) {
			int result, op1, op2;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			*(int*) result = *(int*) op1 * *(int*) op2;
		} else if (c == DIV) {
			int result_val, result_mod, op1, op2;
			if (!read((char*) &result_val, sizeof(int))
				||
				!read((char*) &result_mod, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			if ((*(int*) op2) == 0)
				exception(DIVIDE_BY_ZERO);
			
			*(int*) result_val = *(int*) op1 / *(int*) op2;
			*(int*) result_mod = *(int*) op1 % *(int*) op2;
		} else if (c == BIT_AND) {
			int result, op1, op2, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			for (int i = 0; i < size; ++i)
				((char*) result)[i] = ((char*) op1)[i] & ((char*) op2)[i];
		} else if (c == BIT_OR) {
			int result, op1, op2, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			for (int i = 0; i < size; ++i)
				((char*) result)[i] = ((char*) op1)[i] | ((char*) op2)[i];
		} else if (c == BIT_XOR) {
			int result, op1, op2, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			for (int i = 0; i < size; ++i)
				((char*) result)[i] = ((char*) op1)[i] ^ ((char*) op2)[i];
		} else if (c == BIT_NOT) {
			int result, op, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			for (int i = 0; i < size; ++i)
				((char*) result)[i] = ~ ((char*) op)[i];
		} else if (c == AND) {
			int result, op1, op2, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			char a = 0;
			char b = 0;
			for (int i = 0; i < size; ++i) {
				a = a || ((char*) op1)[i];
				b = b || ((char*) op2)[i];
				((char*) result)[i] = 0;
			}
			
			((char*) result)[0] = a && b;
		} else if (c == OR) {
			int result, op1, op2, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			char a = 0;
			char b = 0;
			for (int i = 0; i < size; ++i) {
				a = a || ((char*) op1)[i];
				b = b || ((char*) op2)[i];
				((char*) result)[i] = 0;
			}
			
			((char*) result)[0] = a || b;
		} else if (c == NOT) {
			int result, op, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			char a = 0;
			for (int i = 0; i < size; ++i) {
				a = a || ((char*) op)[i];
				((char*) result)[i] = 0;
			}
			
			((char*) result)[0] = !a;
		} else if (c == EQ) {
			int result, op1, op2, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			for (int i = 0; i < size; ++i) 
				((char*) result)[i] = 0;
			
			for (int i = 0; i < size; ++i) {
				if (((char*) op1)[i] != ((char*) op2)[i]) {
					((char*) result)[0] = 0;
					goto no_cond1;
				}
			}
			((char*) result)[0] = 1;
			
		no_cond1:;
		} else if (c == NEQ) {
			int result, op1, op2, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			for (int i = 0; i < size; ++i) 
				((char*) result)[i] = 0;
			
			for (int i = 0; i < size; ++i) {
				if (((char*) op1)[i] == ((char*) op2)[i]) {
					((char*) result)[0] = 0;
					goto no_cond2;
				}
			}
			((char*) result)[0] = 1;
			
		no_cond2:;
		} else if (c == GT) {
			int result, op1, op2, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			for (int i = 0; i < size; ++i) 
				((char*) result)[i] = 0;
			
			for (int i = 0; i < size; ++i) {
				if (((char*) op1)[i] <= ((char*) op2)[i]) {
					((char*) result)[0] = 0;
					goto no_cond3;
				}
			}
			((char*) result)[0] = 1;
			
		no_cond3:;
		} else if (c == GE) {
			int result, op1, op2, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			for (int i = 0; i < size; ++i) 
				((char*) result)[i] = 0;
			
			for (int i = 0; i < size; ++i) {
				if (((char*) op1)[i] < ((char*) op2)[i]) {
					((char*) result)[0] = 0;
					goto no_cond4;
				}
			}
			((char*) result)[0] = 1;
			
		no_cond4:;
		} else if (c == LT) {
			int result, op1, op2, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			for (int i = 0; i < size; ++i) 
				((char*) result)[i] = 0;
			
			for (int i = 0; i < size; ++i) {
				if (((char*) op1)[i] >= ((char*) op2)[i]) {
					((char*) result)[0] = 0;
					goto no_cond5;
				}
			}
			((char*) result)[0] = 1;
			
		no_cond5:;
		} else if (c == LE) {
			int result, op1, op2, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &op1, sizeof(int))
				||
				!read((char*) &op2, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
			
			for (int i = 0; i < size; ++i) 
				((char*) result)[i] = 0;
			
			for (int i = 0; i < size; ++i) {
				if (((char*) op1)[i] > ((char*) op2)[i]) {
					((char*) result)[0] = 0;
					goto no_cond6;
				}
			}
			((char*) result)[0] = 1;
			
		no_cond6:;
		} else if (c == PUSH) {
			int result, value, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &value, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
				
			if (stack.size + size >= STACK_SIZE)
				exception(STACK_OVERFLOW);
			
			stack.push(value, size);
			if (result)
				*(int*) result = stack.stack + stack.size;
		} else if (c == POP) {
			int result, value, size;
			if (!read((char*) &result, sizeof(int))
				||
				!read((char*) &value, sizeof(int))
				||
				!read((char*) &size, sizeof(int)))
				exception(SOURCE_BROKEN);
				
			if (stack.size - size < 0)
				exception(STACK_UNDERFLOW);
			
			stack.pop(value, size);
			if (result)
				*(int*) result = stack.stack + stack.size;
		} else if (c == NONE) {
			// Nothing
		} else
			exception(UNEXPECTED_COMMAND);
	}
};


