#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>

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
#define SIZEOF_INT sizeof(int)

// Use little endian for integers
// #define USE_LITTLE_ENDIAN

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
#define NONE            40
#define HALT            41

// Argument constants
int VMH_INT_SIZE = SIZEOF_INT;

#define STRING_ALLOC_SIZE 16

int lineno = 0;

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
	++lineno;
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
	int ksize = 0;
	int klen  = 0;
			
	// Stack with all matched char sequences.
	// Used for solving selector problem:
	// when shortest matching part being replaced before match the while pers aka:
	// > #define AAA   foo
	// > #define AAAB  bar
	// 
	// sth.AAAB => st.fooB -> sth.bar
	std::vector<int> match_stack;
		
	while (1) {
		int eof;
		char *buf = readLine(in, &eof);
		char *buf_bak = buf;
		
		// printf("[%d]> %s\n", lineno, buf);
		
		while (*buf == ' ' || *buf == '\t')
			buf++;
		
		if (*buf == ';' || strlen(buf) == 0) {
			free(buf_bak);		
			if (eof)
				break;
			continue;
		}
		
		// Do processing
		
		char *tmp = buf;
		
		if (match(tmp, "#define") == sizeof("#define")) {
			tmp += sizeof("#define");
				
			int key_start = -1;
			int val_start = -1;
			int val_end   = -1;
			
			while (1) {
				if (*tmp == '\0')
					break;
				if (white(*tmp))
					++tmp;
				else 
					break;
			}
			
			if (*tmp == 0) {
				printf("failed parse define");
				goto pre_loop_continue;
			}
			
			key_start = tmp - buf;
			
			while (1) {
				if (*tmp == '\0')
					break;
				if (!white(*tmp))
					++tmp;
				else 
					break;
			}
			
			if (*tmp == 0) {
				printf("failed parse define");
				goto pre_loop_continue;
			}
			
			val_start = tmp - buf;
			
			while (1) {
				if (*tmp == '\0')
					break;
				++tmp;
			}
			
			val_end = tmp - buf;
			
			char *key = (char*) malloc(val_start - key_start + 1);
			char *val = (char*) malloc(val_end - val_start + 1);
			
			memcpy(key, buf + key_start, val_start - key_start);
			key[val_start - key_start] = '\0';
			
			memcpy(val, buf + val_start, val_end - val_start);
			val[val_end - val_start] = '\0';
			
			for (int i = 0; i < klen; ++i) {
				if (strcmp(keys[i].key, key) == 0) {
					free(keys[i].key);
					free(keys[i].value);
					keys[i].value = val;
					keys[i].value = key;
					
					goto pre_loop_continue;
				}
			}
			
			if (ksize == 0) keys = (pre_keys*) malloc(sizeof(pre_keys) * (ksize = 16));
			else if (klen >= ksize) keys = (pre_keys*) realloc(keys, sizeof(pre_keys) * (ksize <<= 1));
			
			keys[klen].key = key;
			keys[klen++].value = val;
			
			//printf("%s => %s\n", keys[klen-1].key, keys[klen-1].value);
			
			goto pre_loop_continue;
		} 
/*
		if (match(buf, "#"))
			goto pre_loop_continue;
*/
		//printf(">> %s %d %d\n", buf, strlen(buf), klen);
		for (int i = 0; i < strlen(buf); ++i) {
			bool matched = 0;
			int k = 0;
			for (; k < klen; ++k) {
				//printf("match = %d, strlen = %d\n", match(buf+i, keys[k].key), strlen(keys[k].key));
				if (match(buf + i, keys[k].key) == strlen(keys[k].key) + 1) {
					matched = 1;
					match_stack.push_back(k);
				}
			}
			
			if (matched) {
				int len = strlen(keys[k = match_stack[0]].key);
				
				for (int g = 1; g < match_stack.size(); ++g)
					if (len < strlen(keys[match_stack[g]].key)) {
						k = match_stack[g];
						len = strlen(keys[match_stack[g]].key);
					}
				
				match_stack.clear();
				
				//printf("%s => %s\n", keys[k].key, keys[k].value);
				i += strlen(keys[k].key) - 1;
				fprintf(out, "%s", keys[k].value);
			} else
				fputc(buf[i], out);
		}
		fputc('\n', out);
		// fprintf(out, "%s\n", buf);
		
	pre_loop_continue:
		
		free(buf_bak);
	
		if (eof)
			break;
	}
	
loop_end:
	
	for (int i = 0; i < klen; ++i) {
		free(keys[i].value);
		free(keys[i].key);
	}		
	free(keys);
	
	return 1;
};	

enum TOKEN_TYPE {
	TSTRING,
	TINT,
	TPLUS,
	TMINUS,
	TMUL,
	TDIV,
	TKEY,
	TCOMMA,
	COLON2,
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
		
		// COMMA
		if (in[cursor] == ',') {
			++cursor;
			if (!bsize) buf = (token*) malloc(sizeof(token) * (bsize = 16));
			else if (blen >= bsize) buf = (token*) realloc(buf, sizeof(token) * (bsize <<= 2));
			buf[blen++].type = TCOMMA;
			
			continue;
		}
		
		// COLON2
		if (in[cursor] == ':') {
			++cursor;
			if (!bsize) buf = (token*) malloc(sizeof(token) * (bsize = 16));
			else if (blen >= bsize) buf = (token*) realloc(buf, sizeof(token) * (bsize <<= 2));
			buf[blen++].type = COLON2;
			
			continue;
		}
		
		// PLUS
		if (in[cursor] == '+') {
			++cursor;
			if (!bsize) buf = (token*) malloc(sizeof(token) * (bsize = 16));
			else if (blen >= bsize) buf = (token*) realloc(buf, sizeof(token) * (bsize <<= 2));
			buf[blen++].type = TPLUS;
			
			continue;
		}
		
		// MINUS
		if (in[cursor] == '-') {
			++cursor;
			if (!bsize) buf = (token*) malloc(sizeof(token) * (bsize = 16));
			else if (blen >= bsize) buf = (token*) realloc(buf, sizeof(token) * (bsize <<= 2));
			buf[blen++].type = TMINUS;
			
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
					printf("[%d]> %s\n", lineno, in);
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
					if (in[cursor+1] == 'x') {
						cursor += 2;
						if (cursor + 2 >= size) 
							printf("hex-code char notation expected");
						else {
							int a = c2int(in[cursor++], 16);
							int b = c2int(in[cursor++], 16);
							// ignore everything
							str[slen++] = ((a & 0xF) << 1) | (b & 0xF);
						}
					} else if (in[cursor+1] == '"') {
						str[slen++] = '"';
						cursor += 2;
					} else if (in[cursor+1] == 'n') {
						str[slen++] = '\n';
						cursor += 2;
					} else if (in[cursor+1] == 'r') {
						str[slen++] = '\r';
						cursor += 2;
					} else if (in[cursor+1] == 'f') {
						str[slen++] = '\f';
						cursor += 2;
					} else if (in[cursor+1] == 't') {
						str[slen++] = '\t';
						cursor += 2;
					} else if (in[cursor+1] == 'b') {
						str[slen++] = '\b';
						cursor += 2;
					} else if (in[cursor+1] == '0') {
						str[slen++] = '\0';
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
			buf = (token*) realloc(buf, (bsize <<= 1) * sizeof(token));
	
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

struct named_label {
	char *name;
	int addr;
};

named_label *labels;
int lsize = 0;
int llen  = 0;
int write_pos     = 0;
int write_enabled = 0;

int dispose_labels() {
	for (int i = 0; i < llen; ++i)
		free(labels[i].name);
	
	free(labels);
};

void cond_printf(char *s) {
	if (write_enabled)	
		printf(s);
};

void cond_printf(char *s, int k) {
	if (write_enabled)	
		printf(s, k);
};

void cond_printf(char *s, int k, char *s1) {
	if (write_enabled)	
		printf(s, k, s1);
};

void cond_printf(char *s, int k, int i) {
	if (write_enabled)	
		printf(s, k, i);
};

int write(FILE *dest, char value) {
	++write_pos;
	if (!write_enabled)
		return 1;
	fputc(value, dest);
	return 1;
};

int write(FILE *dest, char *ptr, int size) {
	write_pos += size;
	if (!write_enabled)
		return size;
	for (int i = 0; i < size; ++i)
		fputc(ptr[i], dest);
	return size;
};

int writeInt(FILE *dest, int i) {
	write_pos += VMH_INT_SIZE;
	if (!write_enabled)
		return VMH_INT_SIZE;
	
#ifdef USE_LITTLE_ENDIAN
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
#else
	switch (VMH_INT_SIZE) {
		case 1:
			write(dest, i & 0xFF);
			break;
		case 2:
			write(dest, i & 0xFF);
			write(dest, (i >> 8) & 0xFF);
			break;
		case 4:
			write(dest, i & 0xFF);
			write(dest, (i >> 8) & 0xFF);
			write(dest, (i >> 16) & 0xFF);
			write(dest, (i >> 24) & 0xFF);
			break;
		case 8:
			write(dest, i & 0xFF);
			write(dest, (i >> 8) & 0xFF);
			write(dest, (i >> 16) & 0xFF);
			write(dest, (i >> 24) & 0xFF);
			write(dest, (i >> 32) & 0xFF);
			write(dest, (i >> 40) & 0xFF);
			write(dest, (i >> 48) & 0xFF);
			write(dest, (i >> 56) & 0xFF);
			break;
	}
#endif

	return VMH_INT_SIZE;
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
		if (tok[i].type == TCOMMA)
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
	if (tok[i].type == TCOMMA)
		printf(",");
	if (tok[i].type == TPLUS)
		printf("+");
	if (tok[i].type == TMINUS)
		printf("-");
};

int search_for_label(char *label) {
	if (!write_enabled)
		return 1;
	
	for (int i = 0; i < llen; ++i)
		if (strcmp(labels[i].name, label) == 0)
			return labels[i].addr;
	return -1;
};

int opint(token *tok, int off, int *result) {
	tok += off;
	
	int i = 0;
	if (tok[i].type == TINT || tok[i].type == TKEY) {  
		int val = -1;
		if (tok[i].type == TKEY) {
			// XXX: First step: iterate over file & find all labels & their offsets
			val = search_for_label(tok[i].str);
			if (val == -1) {
				printf("No label matching %s found\n", tok[i].str);
				return 0;
			}
		} else
			val = tok[i].val;
		++i;
		
		while (1) {       
			if (tok[i].type == TPLUS) {  
				++i;
				int tval = -1;
				if (tok[i].type == TKEY) {
					tval = search_for_label(tok[i].str);
					if (val == -1) {
						printf("No label matching %s found\n", tok[i].str);
						return 0;
					}
				} else if (tok[i].type == TINT)
					tval = tok[i].val;
				else
					return 0;
				  
				val += tval;
				++i;
			} else if (tok[i].type == TMINUS) {      
				++i;
				int tval = -1;
				if (tok[i].type == TKEY) {
					tval = search_for_label(tok[i].str);
					if (val == -1) {
						printf("No label matching %s found\n", tok[i].str);
						return 0;
					}
				} else if (tok[i].type == TINT)
					tval = tok[i].val;
				else
					return 0;
				  
				val -= tval;
				++i;
			} else
				break;                
		}                          
		
		*result = val;
	} else
		return 0;
	                    
	return i;
}

// <op> int
int op1int(FILE *out, token *tok, int off, int token) {
	tok += off;
	int i = 0;
	int op1;
	
	int inc = opint(tok, i, &op1);
	if (!inc)
		return 0;
	i += inc;
	
	write(out, token);
	writeInt(out, op1);
	
	return i;
};

// <op> int, int
int op2int(FILE *out, token *tok, int off, int token) {
	tok += off;
	int i = 0;
	int op1;
	int op2;
	int inc = opint(tok, i, &op1);
	
	if (!inc)
		return 0;
	i += inc;
	
	if (tok[i].type != TCOMMA)
		return 0;
	++i;
	
	inc = opint(tok, i, &op2);
	if (!inc)
		return 0;
	i += inc;
	
	write(out, token);
	writeInt(out, op1);
	writeInt(out, op2);
	
	return i;
};

// <op> int, int, int
int op3int(FILE *out, token *tok, int off, int token) {
	tok += off;
	
	int i = 0;
	int op1;
	int op2;
	int op3;
	
	int inc = opint(tok, i, &op1);
	if (!inc)
		return 0;
	i += inc;
	
	if (tok[i].type != TCOMMA)
		return 0;
	++i;
	
	inc = opint(tok, i, &op2);
	if (!inc)
		return 0;
	i += inc;
	
	if (tok[i].type != TCOMMA)
		return 0;
	++i;
	
	inc = opint(tok, i, &op3);
	if (!inc)
		return 0;
	i += inc;
	
	write(out, token);
	writeInt(out, op1);
	writeInt(out, op2);
	writeInt(out, op3);
	
	return i;
};

// <op> int, int, int, int
int op4int(FILE *out, token *tok, int off, int token) {
	tok += off;
	
	int i = 0;
	int op1;
	int op2;
	int op3;
	int op4;
	
	int inc = opint(tok, i, &op1);
	if (!inc)
		return 0;
	i += inc;
	
	if (tok[i].type != TCOMMA)
		return 0;
	++i;
	
	inc = opint(tok, i, &op2);
	if (!inc)
		return 0;
	i += inc;
	
	if (tok[i].type != TCOMMA)
		return 0;
	++i;
	
	inc = opint(tok, i, &op3);
	if (!inc)
		return 0;
	i += inc;
	
	if (tok[i].type != TCOMMA)
		return 0;
	++i;
	
	inc = opint(tok, i, &op4);
	if (!inc)
		return 0;
	i += inc;
	
	write(out, token);
	writeInt(out, op1);
	writeInt(out, op2);
	writeInt(out, op3);
	writeInt(out, op4);
	
	return i;
};

// <op> int, int, byte
int op2int1b(FILE *out, token *tok, int off, int token) {
	tok += off;
	
	int i = 0;
	int op1;
	int op2;
	int op3;
	
	int inc = opint(tok, i, &op1);
	if (!inc)
		return 0;
	i += inc;
	
	if (tok[i].type != TCOMMA)
		return 0;
	++i;
	
	inc = opint(tok, i, &op2);
	if (!inc)
		return 0;
	i += inc;
	
	if (tok[i].type != TCOMMA)
		return 0;
	++i;
	
	inc = opint(tok, i, &op3);
	if (!inc)
		return 0;
	i += inc;
	
	write(out, token);
	writeInt(out, op1);
	writeInt(out, op2);
	write(out, op3);
	
	return i;
};

int assemble(FILE *in, FILE *out) {
	int error = 0;
	
	while (1) {
		int eof;
		char *buf = readLine(in, &eof);
		
		token *tok = tokenize(buf);
		
		if (tok == NULL) {
			free(buf);
			break;			
		}
		
		if (tok[0].type == TEOF || tok[0].type != TKEY)
			goto loop_continue;
		
		if (tok[0].type == TKEY && tok[1].type == COLON2 && !write_enabled) {
			if (lsize == 0)
				labels = (named_label*) malloc(sizeof(named_label) * (lsize = 16));
			if (llen >= lsize)
				labels = (named_label*) realloc(labels, (lsize <<= 1) * sizeof(named_label));
			
			labels[llen].name = (char*) calloc(strlen(tok[0].str) + 1, sizeof(char));
			strcpy(labels[llen].name, tok[0].str);
			labels[llen].addr = write_pos;
			++llen;
			
			goto loop_continue;
		}
		
		// print_tokens(tok);
		
		if (strcmp(tok[0].str, "memcpy") == 0) {
			if (!op3int(out, tok, 1, MEMCPY)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("memcpy int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "memref") == 0) {
			if (!op3int(out, tok, 1, MEMREF)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("memref int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "memderef") == 0) {
			if (!op3int(out, tok, 1, MEMDEREF)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("memderef int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "memwrite") == 0) {
			int i = 1;
			int op1;
			int op2;
			int inc;
			int temp;
			int expected_size;
			int total_size = 0;
			int write_len;
			
			if (!(inc = opint(tok, i, &op1)))
				goto else_memwrite;
			i += inc;
			
			if (tok[i].type != TCOMMA)
				goto else_memwrite;
			++i;
			
			if (!(inc = opint(tok, i, &op2)))
				goto else_memwrite;
			i += inc;
			
			expected_size = op2;
			
			write(out, MEMWRITE);
			writeInt(out, op1);
			writeInt(out, op2);
			
			while (1) {
				if (total_size >= expected_size)
					break;
				
				if (tok[i].type != TCOMMA) {
					cond_printf("write size < expected\n");
					goto else_memwrite;
				}
				++i;
				
				if (tok[i].type == TSTRING) {
					write_len = strlen(tok[i].str);
					
					if (total_size + write_len > expected_size) {
						cond_printf("[%d]> %s\n", lineno, buf);
						cond_printf("warning: memwrite data_length > expected\n");
						write(out, tok[i].str, expected_size - total_size);
						total_size = expected_size;
					} else {
						total_size += write_len;
						write(out, tok[i].str, write_len);
					}
					
					++i;
					if (total_size == expected_size)
						break;
					
					continue;
				} else if (tok[i].type == TINT) {
					if (!(inc = opint(tok, i, &temp)))
						goto else_memwrite;
					i += inc;
					++total_size;
					
					write(out, temp);
					continue;
				} else 
					goto else_memwrite;
			}
			
			goto loop_continue;
			
		else_memwrite:
			
			++error;
			cond_printf("[%d]> %s\n", lineno, buf);
			cond_printf("memwrite int, int, [[byte]]\n");
		} else if (strcmp(tok[0].str, "memset") == 0) {
			if (!op2int1b(out, tok, 1, MEMSET)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("memset int, int, byte\n");
			}
		} else if (strcmp(tok[0].str, "alloc") == 0) {
			if (!op2int(out, tok, 1, ALLOC)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("alloc int, int\n");
			}
		} else if (strcmp(tok[0].str, "free") == 0) {
			if (!op1int(out, tok, 1, FREE)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("free int\n");
			}
		} else if (strcmp(tok[0].str, "ncall") == 0) {
			int i = 1;
			int inc;
			int temp;
			int expected_args;
			int total_args = 0;	
			if (!(inc = op2int(out, tok, 1, NATIVE_CALL))) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("ncall int, int, int, [[int]]\n");
				goto loop_continue;
			}					
			i += inc;
			
			if (tok[i].type != TCOMMA)
				goto else_ncall;
			++i;
												
			if (!(inc = opint(tok, i, &temp)))
				goto else_ncall;
			i += inc;
													
			writeInt(out, temp);
			
			expected_args = temp;
			
			for (; total_args < expected_args; ++total_args) {									
				if (tok[i].type != TCOMMA)
					break;
				++i;
														
				if (!(inc = opint(tok, i, &temp)))
					goto else_ncall;
				i += inc;
														
				writeInt(out, temp);
			}
												
			if (total_args < expected_args) {
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("warning: argc < expected\n");
			}
													
			goto loop_continue;
			
		else_ncall:
			
			++error;
			cond_printf("[%d]> %s\n", lineno, buf);
			cond_printf("ncall int, int, int, [[int]]\n");
		} else if (strcmp(tok[0].str, "call") == 0) {
			int i = 1;
			int inc;
			int temp;
			int expected_args;
			int total_args = 0;	
			if (!(inc = op1int(out, tok, 1, CALL))) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("call int, int, [[int]]\n");
				goto loop_continue;
			}					
			i += inc;
			
			if (tok[i].type != TCOMMA)
				goto else_call;
			++i;
												
			if (!(inc = opint(tok, i, &temp)))
				goto else_call;
			i += inc;
													
			writeInt(out, temp);
			
			expected_args = temp;
			
			for (; total_args < expected_args; ++total_args) {									
				if (tok[i].type != TCOMMA)
					break;
				++i;
														
				if (!(inc = opint(tok, i, &temp)))
					goto else_call;
				i += inc;
														
				writeInt(out, temp);
			}
												
			if (total_args < expected_args) {
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("warning: argc < expected\n");
			}
													
			goto loop_continue;
			
		else_call:
			
			++error;
			cond_printf("[%d]> %s\n", lineno, buf);
			cond_printf("call int, int, [[int]]\n");
		} else if (strcmp(tok[0].str, "ret") == 0) {
			write(out, RET);
		} else if (strcmp(tok[0].str, "none") == 0) {
			write(out, NONE);
		} else if (strcmp(tok[0].str, "halt") == 0) {
			write(out, HALT);
		} else if (strcmp(tok[0].str, "jmpif") == 0) {
			if (!op2int(out, tok, 1, JMPIF)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("jmpif int, int\n");
			}
		} else if (strcmp(tok[0].str, "jmpifelse") == 0) {
			if (!op3int(out, tok, 1, JMPIFELSE)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("jmpifelse int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "jmp") == 0) {
			if (!op1int(out, tok, 1, JMP)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("jmp int\n");
			}
		} else if (strcmp(tok[0].str, "add") == 0) {
			if (!op3int(out, tok, 1, ADD)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("add int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "sub") == 0) {
			if (!op3int(out, tok, 1, SUB)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("sub int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "mul") == 0) {
			if (!op3int(out, tok, 1, MUL)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("mul int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "div") == 0) {
			if (!op4int(out, tok, 1, DIV)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("div int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "band") == 0) {
			if (!op4int(out, tok, 1, BIT_AND)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("band int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "bor") == 0) {
			if (!op4int(out, tok, 1, BIT_OR)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("bor int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "bxor") == 0) {
			if (!op4int(out, tok, 1, BIT_XOR)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("bxor int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "bnot") == 0) {
			if (!op3int(out, tok, 1, BIT_NOT)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("bnot int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "and") == 0) {
			if (!op4int(out, tok, 1, AND)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("adn int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "or") == 0) {
			if (!op4int(out, tok, 1, OR)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("or int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "not") == 0) {
			if (!op3int(out, tok, 1, NOT)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("not int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "eq") == 0) {
			if (!op4int(out, tok, 1, EQ)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("eq int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "neq") == 0) {
			if (!op4int(out, tok, 1, NEQ)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("neq int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "gt") == 0) {
			if (!op4int(out, tok, 1, GT)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("gt int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "ge") == 0) {
			if (!op4int(out, tok, 1, GE)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("ge int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "lt") == 0) {
			if (!op4int(out, tok, 1, LT)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("lt int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "le") == 0) {
			if (!op4int(out, tok, 1, LE)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("le int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "push") == 0) {
			if (!op3int(out, tok, 1, PUSH)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("push int, int, int, int\n");
			}
		} else if (strcmp(tok[0].str, "pop") == 0) {
			if (!op3int(out, tok, 1, POP)) {
				++error;
				cond_printf("[%d]> %s\n", lineno, buf);
				cond_printf("pop int, int, int, int\n");
			}
		}
		
	loop_continue:
		
		delete_tokens(tok);
		free(buf);
	
		if (eof)
			break;
	}
	
loop_end:

	if (error)
		cond_printf("Total errors: %d\n", error);

	return 1;
};


// g++ main.cpp --std=c++11 -w -g -o main && valgrind --leak-check=full ./main -i 2 test.asl

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
	lineno = 0;
	result = assemble(pre, out);
	fseek(pre, SEEK_SET, 0);
	write_enabled = 1;
	lineno = 0;
	result = assemble(pre, out);
	dispose_labels();
	
	fclose(pre);
	fclose(out);
	
	return 0;
	
	/*
	while (1) {
		int eof;
		char *buf = readLine(in, &eof);
		
		printf("[%d]> %s\n", lineno, buf);
		
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

