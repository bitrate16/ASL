#include <cstdlib>
#include <cstdio>
#include <cstring>

/*

List of assebpler commanes given below.

Also supported "preprocessor":

#define  KEY VALUE
#undef   KEY

#if      EXPRESSION
#else
#elseif  EXPRESSION
#endif

> defined KEY
> not EXPRESSION
> EXPRESSION == EXPRESSION
> EXPRESSION != EXPRESSION
> EXPRESSION >  EXPRESSION
> EXPRESSION <  EXPRESSION
> EXPRESSION >= EXPRESSION
> EXPRESSION <= EXPRESSION
> (EXPRESSION)

> VALUE ~ STRING | EXPRESSION[VALUE[\STRING], INTEGER, +, -]

*/


// Values from arduino tests:
#define SIZEOF_INT 2


#define METAKEY     0xEBA1
// #define EOF             -1
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

// Argument constants
int VMH_INT_SIZE = SIZEOF_INT;

#define STRING_ALLOC_SIZE 16

char *readLine(FILE *f, int *eof) {
	char *buf  = (char*) malloc(STRING_ALLOC_SIZE);
	int size   = STRING_ALLOC_SIZE;
	int length = 0;
	
	if (eof)
		*eof = 0;
	
	while (1) {
		int c = fgetc(f);
		if (c == EOF) {
			if (eof)
				*eof = 1;
			break;
		}
		
		if (c == '\n')
			break;
		
		if (length >= size)
			buf = (char*) realloc(buf, size <<= 1);
		
		buf[length++] = c;
	}
	
	if (length >= size)
		buf = (char*) realloc(buf, size <<= 1);
	
	buf[length] = 0;
	return buf;
};
 
bool alpha(char c) {
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
};

bool digit(char c) {
	return '0' <= c && c <= '9';
};

bool white(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
};

int match(char *start, char *match) {
	if (strlen(start) < strlen(match))
		return 0;
	
	int i = 0;
	while (1) {
		if (match[i] == '\0' || white(match[i]) && white(start[i]))
			return i + 1;
		if (start[i] != match[i])
			return 0;
		++i;
	}
};

struct pre_keys {
	pre_keys *next = NULL;
	char *key;
	char *value;
};
 
int pre_process(FILE *in, FILE *out) {
	pre_keys *keys = NULL;
		
	while (1) {
		int eof;
		char *buf = readLine(in, &eof);
		
		// printf("> %s\n", buf);
		
		while (*buf == ' ' || *buf == '\t')
			buf++;
		
		if (*buf == ';' || strlen(buf) == 0)
			goto pre_loop_continue;
		
		// XXX: Not finished yet
		
		// Do processing
		/*
		char *tmp = buf;
		
		if (match('#')) {
			++tmp;
			
			int off = 0;
			
			if (off = match("define")) {
				tmp += off;
				
				char *copy = (char*) malloc(strlen(tmp) + 1);
				strcpy(copy, tmp);
				
				pre_keys = (char*) malloc(sizeof(pre_keys));
				pre_keys->next = NULL;
				pre_keyx->key = copy;
				pre_keys
			}
		}*/

		if (match(buf, "#"))
			goto pre_loop_continue;

		fprintf(out, "%s\n", buf);
		
	pre_loop_continue:
		
		free(buf);
	
		if (eof)
			break;
	}
	
loop_end:
	
	pre_keys *temp;
	while (keys) {
		temp = keys->next;
		free(keys);
		keys = temp;
	}
	
	return 1;
};	

enum TOKEN_TYPE {
	TSTRING,
	TINT,
	TPLUS,
	TMINUS,
	TKEY,
	TCOLON,
	TEOF
};

struct token {
	TOKEN_TYPE type;
	char       *str;
	int         val;
};

int matchwhite(char *start) {
	int i = 0;
	
	while (1) {
		if (start[i] == '\0')
			return i;
		
		if (!white(start[i]))
			return i;
		
		++i;
	}
};

int upperchar(int c) {
	return ('a' <= c && c <= 'z') ? c + 'A' - 'a' : c;
};

int c2int(char c, int base) {
	c = upperchar(c);
	if (base == 10)
		if ('0' <= 'c' && c <= '9')
			return c - '0';
		else
			return -1;
	
	if (base == 8)
		if ('0' <= 'c' && c <= '7')
			return c - '0';
		else
			return -1;
		
	if (base == 16)
		if ('0' <= 'c' && c <= '9')
			return c - '0';
		else if ('A' <= 'c' && c <= 'F')
			return 10 + c - 'A';
		else
			return -1;
		
	return -1;
};

void delete_tokens(token *t) {
	int i = 0;
	
	while (1) {
		if (t[i].type == TSTRING)
			free(t[i].str);
		if (t[i].type == TKEY)
			free(t[i].str);
		if (t[i].type == TEOF)
			break;
		++i;
	}
	
	free(t);
};

token *tokenize(char *in) {
	int cursor = 0;
	int size   = strlen(in);
	token *buf = NULL;
	int blen   = 0;
	int bsize  = 0;
	
	while (1) {
		if (cursor >= size) goto return_label;
		
		// WHITESPACES
		while (1) {
			if (!in[cursor])
				goto return_label;
			
			if (!white(in[cursor]))
				break;
			
			++cursor;
		}
		
		if (!in[cursor])
			goto return_label;
		
		// KEYS
		if (alpha(in[cursor])) {
			char *str = (char*) malloc(STRING_ALLOC_SIZE);
			int slen  = 0;
			int ssize = STRING_ALLOC_SIZE;
			
			str[slen++] = in[cursor++];
			
			while (1) {
				if (!in[cursor])
					goto end_key;
				
				if (!alpha(in[cursor]) && !digit(in[cursor]))
					goto end_key;
				
				if (slen >= ssize)
					str = (char*) realloc(str, (ssize <<= 2));
				
				str[slen++] = in[cursor++];
			}
			
		end_key:
		
			if (slen >= ssize)
				str = (char*) realloc(str, (ssize <<= 2));
			
			str[slen] = '\0';
			
			if (!bsize) buf = (token*) malloc(sizeof(token) * (bsize = 16));
			else if (blen >= bsize) buf = (token*) realloc(buf, sizeof(token) * (bsize <<= 2));
			buf[blen].type  = TKEY;
			buf[blen++].str = str;
			
			continue;
		}
		
		// COLON
		if (in[cursor] == ',') {
			++cursor;
			if (!bsize) buf = (token*) malloc(sizeof(token) * (bsize = 16));
			else if (blen >= bsize) buf = (token*) realloc(buf, sizeof(token) * (bsize <<= 2));
			buf[blen++].type = TCOLON;
			
			continue;
		}
		
		// INT
		if (digit(in[cursor])) {
			int base = 10;
			
			if (in[cursor] == '0' && upperchar(in[cursor+1]) == 'X') { base = 16; cursor += 2; }
			if (in[cursor] == '0' && upperchar(in[cursor+1]) == 'B') { base = 2;  cursor += 2; }
			if (in[cursor] == '0' && upperchar(in[cursor+1]) == 'O') { base = 8;  cursor += 2; }
			
			int num = 0;
			
			while (!0) {
				if (cursor >= size)
					break;
				
				if (!alpha(in[cursor]) && !digit(in[cursor]))
					break;
				
				int conv = c2int(in[cursor], base);
				
				if (conv == -1) {
					printf("base mismatch integer literal\n");
					goto error_label;
				}
				
				num = num * base + conv;
				++cursor;
			}
			
			if (!bsize) buf = (token*) malloc(sizeof(token) * (bsize = 16));
			else if (blen >= bsize) buf = (token*) realloc(buf, sizeof(token) * (bsize <<= 2));
			buf[blen].type  = TINT;
			buf[blen++].val = num;
			
			continue;
		}
		
		// STRINGS
		if (in[cursor] == '"') {
			char *str = (char*) malloc(STRING_ALLOC_SIZE);
			int slen  = 0;
			int ssize = STRING_ALLOC_SIZE;
			
			++cursor;
			
			while (1) {
				if (!in[cursor])
					goto end_string;
				
				if (in[cursor] == '"') {
					++cursor;
					goto end_string;
				}
				
				if (slen >= ssize)
					str = (char*) realloc(str, (ssize <<= 2));
				
				if (in[cursor] == '\\') {
					if (in[cursor] == 'x') {
						cursor += 2;
						if (cursor + 2 >= size) 
							printf("hex-code char notation expected");
						else {
							int a = c2int(in[cursor++], 16);
							int b = c2int(in[cursor++], 16);
							// ignore everything
							str[slen++] = ((a & 0xF) << 1) | (b & 0xF);
						}
					} else if (in[cursor] == '"') {
						str[slen++] = '"';
						cursor += 2;
					} else
						str[slen++] = in[cursor++];
				} else
					str[slen++] = in[cursor++];
			}
			
		end_string:
		
			if (slen >= ssize)
				str = (char*) realloc(str, (ssize <<= 2));
			
			str[slen] = '\0';
			
			if (!bsize) buf = (token*) malloc(sizeof(token) * (bsize = 16));
			else if (blen >= bsize) buf = (token*) realloc(buf, sizeof(token) * (bsize <<= 2));
			buf[blen].type  = TSTRING;
			buf[blen++].str = str;
			
			continue;
		}
		
		printf("unexpected character [%c]\n", in[cursor]);
		break;
	}
	
error_label:

	for (int i = 0; i < blen; ++i) {
		if (buf[i].type == TSTRING)
			free(buf[i].str);
		if (buf[i].type == TKEY)
			free(buf[i].str);
	}
	
	free(buf);
	
	return NULL;

	
return_label:

	if (bsize) {
		if (blen >= bsize)
			buf = (token*) realloc(buf, (blen <<= 1) * sizeof(token));
	
		buf[blen].type = TEOF;
	}
	
	return buf;
};

int matchseq(token *t, int off, int t1) {
	return  t[off+0].type == t1;
}

int matchseq(token *t, int off, int t1, int t2, int t3) {
	return  t[off+0].type == t1
			&&
			t[off+1].type == t2
			&&
			t[off+2].type == t3;
}

int matchseq(token *t, int off, int t1, int t2, int t3, int t4) {
	return  t[off+0].type == t1
			&&
			t[off+1].type == t2
			&&
			t[off+2].type == t3
			&&
			t[off+3].type == t4;
}

int matchseq(token *t, int off, int t1, int t2, int t3, int t4, int t5) {
	return  t[off+0].type == t1
			&&
			t[off+1].type == t2
			&&
			t[off+2].type == t3
			&&
			t[off+3].type == t4
			&&
			t[off+4].type == t5;
};

int matchseq(token *t, int off, int t1, int t2, int t3, int t4, int t5, int t6) {
	return  t[off+0].type == t1
			&&
			t[off+1].type == t2
			&&
			t[off+2].type == t3
			&&
			t[off+3].type == t4
			&&
			t[off+4].type == t5
			&&
			t[off+5].type == t6;
};

int matchseq(token *t, int off, int t1, int t2, int t3, int t4, int t5, int t6, int t7) {
	return  t[off+0].type == t1
			&&
			t[off+1].type == t2
			&&
			t[off+2].type == t3
			&&
			t[off+3].type == t4
			&&
			t[off+4].type == t5
			&&
			t[off+5].type == t6
			&&
			t[off+6].type == t7;
};

int matchseq(token *t, int off, int t1, int t2, int t3, int t4, int t5, int t6, int t7, int t8) {
	return  t[off+0].type == t1
			&&
			t[off+1].type == t2
			&&
			t[off+2].type == t3
			&&
			t[off+3].type == t4
			&&
			t[off+4].type == t5
			&&
			t[off+5].type == t6
			&&
			t[off+6].type == t7
			&&
			t[off+7].type == t8;
};

// Yes, i know about va_args

int write(FILE *dest, char value) {
	fputc(value, dest);
	return 1;
};

int write(FILE *dest, char *ptr, int size) {
	for (int i = 0; i < size; ++i)
		fputc(ptr[i], dest);
	return size;
};

int writeInt(FILE *dest, int i) {
	switch (VMH_INT_SIZE) {
		case 1:
			write(dest, i & 0xFF);
			break;
		case 2:
			write(dest, (i >> 8) & 0xFF);
			write(dest, i & 0xFF);
			break;
		case 4:
			write(dest, (i >> 24) & 0xFF);
			write(dest, (i >> 16) & 0xFF);
			write(dest, (i >> 8) & 0xFF);
			write(dest, i & 0xFF);
			break;
		case 8:
			write(dest, (i >> 56) & 0xFF);
			write(dest, (i >> 48) & 0xFF);
			write(dest, (i >> 40) & 0xFF);
			write(dest, (i >> 32) & 0xFF);
			write(dest, (i >> 24) & 0xFF);
			write(dest, (i >> 16) & 0xFF);
			write(dest, (i >> 8) & 0xFF);
			write(dest, i & 0xFF);
			break;
	}
};

void print_tokens(token *tok) {
	for (int i = 0;; ++i) {
		if (tok[i].type == TEOF)
			break;
		if (tok[i].type == TINT)
			printf("%d ", tok[i].val);
		if (tok[i].type == TKEY)
			printf("%s ", tok[i].str);
		if (tok[i].type == TSTRING)
			printf("\"%s\" ", tok[i].str);
		if (tok[i].type == TCOLON)
			printf(", ");
	}
	
	printf("\n");
};

void print_token(token *tok, int i) {
	if (tok[i].type == TEOF)
		printf("EOF");
	if (tok[i].type == TINT)
		printf("%d", tok[i].val);
	if (tok[i].type == TKEY)
		printf("%s", tok[i].str);
	if (tok[i].type == TSTRING)
		printf("\"%s\"", tok[i].str);
	if (tok[i].type == TCOLON)
		printf(",");
};

int assemble(FILE *in, FILE *out) {
	int error = 0;
	
	while (1) {
		int eof;
		char *buf = readLine(in, &eof);
		
		// printf("> %s\n", buf);

		if (match(buf, "#")) {
			free(buf);
		}
		
		token *tok = tokenize(buf);
		
		if (tok == NULL) {
			free(buf);
			break;			
		}
		
		if (tok[0].type == TEOF || tok[0].type != TKEY)
			goto loop_continue;
		
		// print_tokens(tok);
		
		if (strcmp(tok[0].str, "memcpy") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, MEMCPY);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("memcpy int, int, int\n");
		} else if (strcmp(tok[0].str, "memref") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, MEMREF);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("memref int, int, int\n");
		} else if (strcmp(tok[0].str, "memderef") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, MEMDEREF);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("memderef int, int, int\n");
		} else if (strcmp(tok[0].str, "memwrite") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT)) {
				write(out, MEMWRITE);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				
				int expected_size = tok[3].val;
				// printf("expected_size = %d\n", expected_size);
				
				if (expected_size) {
					if (!matchseq(tok, 4, TCOLON))
						goto else_memwrite;
				} else
					printf("Are you serious? memwrite nothing?\n");
				
				int i = 5;
				int tot_len = 0;
				while (1) {
					if (tot_len == expected_size)
						break;
					
					if (tok[i].type == TSTRING) {
						int write_len = strlen(tok[i].str);
						
						if (tot_len + write_len > expected_size) {
							printf("warning: memwrite data_length > expected\n");
							write(out, tok[i].str, expected_size - tot_len);
							tot_len = expected_size;
						} else {
							tot_len += write_len;
							write(out, tok[i].str, write_len);
						}
						
						if (tot_len == expected_size)
							break;
					} else if (tok[i].type == TINT) {
						// Assume integer is a byte value
						++tot_len;
						write(out, tok[i].val);
						
						++i;
						if (tot_len == expected_size)
							break;
					} else {
						printf("unexpected token [");
						print_token(tok, i);
						printf("]\n");
						++error;
						goto loop_continue;
					}
					
					if (tok[i].type == TEOF)
						break;
					if (tok[i].type == TCOLON) 
						continue;
					
					printf("unexpected token [");
					print_token(tok, i);
					printf("]\n");
					++error;
					goto loop_continue;
				}
				
				goto loop_continue;
			}
			
		else_memwrite:
			
			++error;
			printf("memwrite int, int, [[byte]]\n");
		} else if (strcmp(tok[0].str, "memset") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, MEMSET);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				write(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("memset int, int, byte\n");
		} else if (strcmp(tok[0].str, "alloc") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT)) {
				write(out, ALLOC);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("alloc int, int\n");
		} else if (strcmp(tok[0].str, "free") == 0) {
			if (matchseq(tok, 1, TINT)) {
				write(out, FREE);
				writeInt(out, tok[1].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("free int\n");
		} else if (strcmp(tok[0].str, "ncall") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, NATIVE_CALL);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				int expected_size = tok[5].val;
				// printf("expected_size = %d\n", expected_size);
				
				if (expected_size) 
					if (!matchseq(tok, 6, TCOLON))
						goto else_ncall;
				
				int i = 7;
				int tot_len = 0;
				while (1) {
					if (tot_len == expected_size)
						break;
					
					if (tok[i].type == TINT) {
						++tot_len;
						writeInt(out, tok[i].val);
						
						++i;
						if (tot_len == expected_size)
							break;
					} else {
						printf("unexpected token [");
						print_token(tok, i);
						printf("]\n");
						++error;
						goto loop_continue;
					}
					
					if (tok[i].type == TEOF)
						break;
					if (tok[i].type == TCOLON) 
						continue;
					
					printf("unexpected token [");
					print_token(tok, i);
					printf("]\n");
					++error;
					goto loop_continue;
				}
				
				goto loop_continue;
			} 
			
		else_ncall:
			
			++error;
			printf("ncall int, int, int, [[int]]\n");
		} else if (strcmp(tok[0].str, "call") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT)) {
				write(out, CALL);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				
				int expected_size = tok[3].val;
				// printf("expected_size = %d\n", expected_size);
				
				if (expected_size) 
					if (!matchseq(tok, 4, TCOLON))
						goto else_call;
				
				int i = 5;
				int tot_len = 0;
				while (1) {
					if (tot_len == expected_size)
						break;
					
					if (tok[i].type == TINT) {
						++tot_len;
						writeInt(out, tok[i].val);
						
						++i;
						if (tot_len == expected_size)
							break;
					} else {
						printf("unexpected token [");
						print_token(tok, i);
						printf("]\n");
						++error;
						goto loop_continue;
					}
					
					if (tok[i].type == TEOF)
						break;
					if (tok[i].type == TCOLON) 
						continue;
					
					printf("unexpected token [");
					print_token(tok, i);
					printf("]\n");
					++error;
					goto loop_continue;
				}
				
				goto loop_continue;
			} 
			
		else_call:
			
			++error;
			printf("call int, int, [[int]]\n");
		} else if (strcmp(tok[0].str, "ret") == 0) {
			write(out, RET);
		} else if (strcmp(tok[0].str, "jmpif") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT)) {
				write(out, JMPIF);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("jmpif int, int\n");
		} else if (strcmp(tok[0].str, "jmpifelse") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, JMPIFELSE);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("jmpifelse int, int, int\n");
		} else if (strcmp(tok[0].str, "jmp") == 0) {
			if (matchseq(tok, 1, TINT)) {
				write(out, JMP);
				writeInt(out, tok[1].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("jmp int\n");
		} else if (strcmp(tok[0].str, "add") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, ADD);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("add int, int, int\n");
		} else if (strcmp(tok[0].str, "sub") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, SUB);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("sub int, int, int\n");
		} else if (strcmp(tok[0].str, "mul") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, MUL);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("mul int, int, int\n");
		} else if (strcmp(tok[0].str, "div") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, DIV);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("div int, int, int, int\n");
		} else if (strcmp(tok[0].str, "band") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, BIT_AND);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("band int, int, int, int\n");
		} else if (strcmp(tok[0].str, "bor") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, BIT_OR);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("bor int, int, int, int\n");
		} else if (strcmp(tok[0].str, "bxor") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, BIT_XOR);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("bxor int, int, int, int\n");
		} else if (strcmp(tok[0].str, "bnot") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, BIT_NOT);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("bnot int, int, int\n");
		} else if (strcmp(tok[0].str, "and") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, AND);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("and int, int, int, int\n");
		} else if (strcmp(tok[0].str, "or") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, OR);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("or int, int, int, int\n");
		} else if (strcmp(tok[0].str, "not") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, NOT);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("not int, int, int\n");
		} else if (strcmp(tok[0].str, "eq") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, EQ);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("eq int, int, int, int\n");
		} else if (strcmp(tok[0].str, "neq") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, NEQ);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("neq int, int, int, int\n");
		} else if (strcmp(tok[0].str, "gt") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, GT);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("gt int, int, int, int\n");
		} else if (strcmp(tok[0].str, "ge") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, GE);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("ge int, int, int, int\n");
		} else if (strcmp(tok[0].str, "lt") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, LT);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("lt int, int, int, int\n");
		} else if (strcmp(tok[0].str, "le") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, LE);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				writeInt(out, tok[7].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("le int, int, int, int\n");
		} else if (strcmp(tok[0].str, "push") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, PUSH);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("push int, int, int\n");
		} else if (strcmp(tok[0].str, "pop") == 0) {
			if (matchseq(tok, 1, TINT, TCOLON, TINT, TCOLON, TINT)) {
				write(out, POP);
				writeInt(out, tok[1].val);
				writeInt(out, tok[3].val);
				writeInt(out, tok[5].val);
				
				goto loop_continue;
			}
			
			++error;
			printf("pop int, int, int\n");
		}
		
	loop_continue:
		
		delete_tokens(tok);
		free(buf);
	
		if (eof)
			break;
	}
	
loop_end:

	if (error)
		printf("total errors: %d", error);

	return 1;
};


// g++ main.cpp --std=c++11 -w -g -o main && valgrind --leak-check=full ./main test.asl

int main(int argc, char **argv) {
	const char *input      = "in.asl";
	const char *output     = "out.aslb";
	const char *pre_output = "pre.aslp";
	
	bool input_passed = 0;
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-i") == 0) { // Int size
			if (i == argc - 1) {
				printf("Usage -i <vm host int size>\n");
				return 1;
			}
			
			char *test;
			VMH_INT_SIZE = strtol(argv[++i], &test, 10);
			if (test == argv[i]) {
				printf("Invalid number [%s]\n", argv[i]);
				printf("Usage -i <vm host int size>\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-o") == 0) { // Output file
			if (i == argc - 1) {
				printf("Usage -o <file>\n");
				return 1;
			}
			
			output = argv[++i];
		} else if (strcmp(argv[i], "-p") == 0) { // Preprocessor file
			if (i == argc - 1) {
				printf("Usage -p <file>\n");
				return 1;
			}
			
			output = argv[++i];
		} else if (strcmp(argv[i], "-h") == 0) { // Help
			printf("Usage:\n");
			printf("-i <vm host int size>\n");
			printf(" Used for passing custom integer size.\n");
			printf(" For example, if vm host sizeof(int) == 2, use '-i 2'\n");
			printf("\n");
			printf("-o <file>\n");
			printf(" Used for passing custom output file.\n");
			printf(" For example, use '-o myoutput.foo'\n");
			printf("\n");
			printf("-p <file>\n");
			printf(" Used for passing custom output file for preprocessor.\n");
			printf(" For example, use '-o mypreproc.foo'\n");
			printf("\n");
			printf("-h\n");
			printf(" For this help.\n");
			printf("\n");
			printf("Example usage: '-i 128 -o file.aslb -p temp.aslp input.asl");
			return 0;
		} else {                              // Input file
			// Assume it's input file
			if (input_passed) {
				printf("Invalid input. Use '-h' for help.\n");
				return 1;
			}
			
			input = argv[i];
		}
	}
	
	/*
	if (argc > 1)
		input = argv[1];
	if (argc > 2)
		output = argv[2];
	*/
	
	// P R E P R O C E S S O R
	
	FILE *in = fopen(input, "r");
	
	if (!in) {
		printf("File %s not found\n", input);
		return 1;
	}
	
	FILE *pre = fopen(pre_output, "w+");
	
	if (!pre) {
		fclose(in);
		printf("File %s could not be created\n", pre_output);
		return 1;
	}
	
	int result = pre_process(in, pre);
	
	fclose(in);
	// fclose(out);
	
	if (!result) {
		fclose(pre);
		return 1;
	}
	
	// A S S E M B L E R
	
	FILE *out = fopen(output, "w");
	
	if (!out) {
		fclose(pre);
		printf("File %s could not be created\n", output);
		return 1;
	}
	
	fseek(pre, SEEK_SET, 0);
	result = assemble(pre, out);
	
	fclose(pre);
	fclose(out);
	
	return 0;
	
	/*
	while (1) {
		int eof;
		char *buf = readLine(in, &eof);
		
		printf("> %s\n", buf);
		
		while (*buf == ' ' || *buf == '\t')
			buf++;
		
		if (*buf == ';')
			goto loop_continue;
		
		token *tokens = tokenize(buf);
		
	loop_continue:
	
		free(buf);
	
		if (eof)
			break;
	}
	
	fclose(in);
	fclose(out);
	*/
};

