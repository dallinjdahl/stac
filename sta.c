#define POSIX

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sizedefs.h"
#include "primdefs.h"
#ifdef POSIX
#include <sys/stat.h>
#endif

#define DSIZE 5000
#define RSSIZE 100
#define STSIZE 100

#define DREGS 5 //number of in-disk registers (dict, rsp, tosp, link, base)

#define NEXT IP=disk[disk[*rsp]++]
#define RPUSH disk[++(*rsp)]=
#define PUSH disk[--(*tosp)]=
#define TOS disk[*tosp]
#define NTOS disk[(*tosp)+1]
#define NTORS disk[(*rsp)-1]
#define TORS disk[*rsp]
#define DROP ++(*tosp)
#define RDROP --(*rsp)

typedef int cell;

#define TWOLEVEL(EFFECT) NTOS=EFFECT;DROP;NEXT
#define COMPPRIM(PRIM) enter(primaddr[PRIM])
#define COLON(NAME) intern(DOCOL, 0);cell NAME=*dict-1
#define IF(BNAME) enter(notbranch);cell BNAME=*dict;enter(0)
#define ELSE(BNAME) COMPPRIM(BRANCH);\
	disk[BNAME]=(*dict)-BNAME;\
BNAME=*dict;enter(0)
#define THEN(BNAME) disk[BNAME]=*dict-BNAME-1 // the reason the minus one isn't necessary in the else,
					//is we do it an instruction early, when dict points to the branch offset

cell disk[DSIZE] = {DREGS, DSIZE-(RSSIZE+STSIZE+1), DSIZE-1, 0, 16},
    *dict = disk, *rsp = disk+1, *tosp = disk+2,
    *link = disk+3, *base = disk+4, w, IP, primaddr[NOT+1], cs;

char strbuf[100] = {'\0'};

FILE *f;

struct stat st;

int mputs(char *s){
	int i = fputs(s, stdout);
	fflush(stdout);
	return i;
}
int mputchar(int c){
	int i = putchar(c);
	fflush(stdout);
	return i;
}
int scant(char c, char *s) {  
	int i = 1;
	if((int)c == 127){
		while(i){
			*s = getchar();
			switch (*s) {
			case ' ':
			case '\t':
			case '\n':
				break;
			default:
				s++;
				i = 0;
				break;
			}
		}
	}
	for( i = 0; ; s++, i++) {
		*s = getchar();
		if (*s == c) {
			*s = '\0';
			return i;
		}
		if ((int) c == 127) {
			switch (*s) {
			case ' ':
			case '\t':
			case '\n':
			case '\0':
				*s = '\0';
				return i+1;
			}
		}
		if (*s == EOF){
			exit(0);
		}
	}
}

int streql(cell *s1, cell *s2) {
	char *ss1 = (char *)(s1+1);
	char *ss2 = (char *)(s2+1);
	if (*s1 != *s2)
		return 0;
	for (int i = 0; i < *s1; i++){
		if (ss1[i] != ss2[i])
			return 0;
	}
	return 1;
}

int putnumstr(cell *s1) {
	int len = *s1;
	char *ss1 = (char *)(s1+1);
	for(int i = 0; i < len; i++, ss1++){
		putchar((int)(*ss1));
	}
	return 0;
}

char convertdigit(short x) { //assumes positive integer
	if( x < 10) {
		return (char) x + '0';
	}
	return (char) x + 'a' - 10;
}

char *itoa(int i){
	char *p = strbuf;
	unsigned int j = i;
	unsigned int shifter = j;
	do{ //Move to where representation ends
		++p;
		shifter /= *base;
	} while(shifter);
	*p = '\0';
	do{ //Move back, inserting digits as you go
		*--p =convertdigit(j%(*base));
		j /= *base;
	} while(j);
	return strbuf;
}

void dumpstack(int i, cell *sp){
	mputs("STACK[");
	for(int j = 0; j < i; j++){
		mputs(" ");
		mputs(itoa(disk[*sp+j]));
	}
	puts(" ]");
}

void state(){
	mputs("Dict = ");
	puts(itoa(*dict));
	mputs("rstack = ");
	puts(itoa(*rsp - (DSIZE-(RSSIZE+STSIZE+1))));
	mputs("dstack = ");
	puts(itoa(DSIZE-*tosp-1));
}

void enter(cell x){
	disk[(*dict)++] = x;
}

void intern(cell x, cell imm) {
	enter(*link);
	*link = *dict-1;
	w = *dict;
	enter(0);
	cell slen = scant((char) 127, (char *)(disk+(*dict)));
	disk[w] = slen;
	*dict += slen/PACK + 1; // to allow for partially filled cells
	enter(imm);
	enter(x);
//	mputs("interning ");
//	putnumstr(disk+w);
//	mputs(" @ ");
//	puts(itoa(*dict-1));
}

char *cstring(int *x) {
	char *s = (char *)(x+1);
	int l = *x;
	for (int i = 0; i < l; i++) {
		strbuf[i] = s[i];
	}
	strbuf[l] = '\0';
	return strbuf;
}

void execute(cell x) {
	cell op = disk[x];
	char *s;
	switch (op) {
	case DOCOL:
		w = ++IP;
		RPUSH ++IP;
		IP = disk[w];
		break;
	case KEY:
		PUSH getchar();
		NEXT;
		break;
	case WORD:
		w = *dict;
		enter(0);
		int slen = scant((char) TOS, (char *)(disk + (*dict)));
		DROP;
		disk[w] = slen;
		PUSH w;
		*dict += slen/PACK + 1;
		NEXT;
		break;
	case FIND:
		w = *link;
		while (!(streql(disk+TOS, (disk+w+1))) && w) {
			w = disk[w];
		}
		if (!w) {
			PUSH 1;
		} else {
			TOS = w + 2 + ((disk[w+1])/PACK + 2);
			w = disk[TOS-1];
			PUSH w;
		}
		NEXT;
		break;
	case EXIT:
		RDROP;
		NEXT;
		break;
	case PEEK:
		TOS = disk[TOS];
		NEXT;
		break;
	case POKE:
		disk[TOS] = NTOS;
		DROP;
		DROP;
		NEXT;
		break;
	case LIT:
		PUSH disk[TORS++];
		NEXT;
		break;
	case PUSNXT:
		PUSH disk[NTORS++];
		NEXT;
		break;
	case BRANCH:
		TORS += disk[TORS] + 1; //was off by one
		NEXT;
		break;
	case PDROP:
		DROP;
		NEXT;
		break;
	case TOR:
		w = TORS;
		TORS = TOS;
		RPUSH w;
		DROP;
		NEXT;
		break;
	case FROMR:
		PUSH NTORS;
		w = TORS;
		RDROP;
		TORS = w;
		NEXT;
		break;
	case DUP:
		w = TOS;
		PUSH w;
		NEXT;
		break;
	case SWAP:
		w = TOS;
		TOS = NTOS;
		NTOS = w;
		NEXT;
		break;
	case ROT:
		w = TOS;
		TOS = disk[(*tosp)+2];
		disk[(*tosp)+2] = NTOS;
		NTOS = w;
		NEXT;
		break;
	case PLUS:
		TWOLEVEL(TOS + NTOS);
		break;
	case MINUS:
		TWOLEVEL(NTOS - TOS);
		break;
	case MULT:
		TWOLEVEL(TOS * NTOS);
		break;
	case DIV:
		TWOLEVEL(NTOS / TOS);
		break;
	case ARSHIFT:
		NTOS >>= TOS;
		DROP;
		NEXT;
		break;
	case RSHIFT:
		NTOS = (unsigned int) NTOS >> TOS;
		DROP;
		NEXT;
		break;
	case LSHIFT:
		NTOS <<= TOS;
		DROP;
		NEXT;
		break;
	case LESS:
		TWOLEVEL(NTOS < TOS ? -1 : 0);
		break;
	case GREAT:
		TWOLEVEL(NTOS > TOS ? -1 : 0);
		break;
	case ULESS:
		TWOLEVEL(NTOS < (unsigned int)TOS ? -1 : 0);
		break;
	case UGREAT:
		TWOLEVEL(NTOS > (unsigned int)TOS ? -1 : 0);
		break;
	case EQL:
		NTOS = NTOS == TOS ? -1 : 0;
		DROP;
		NEXT;
		break;
	case EMIT:
		mputchar(TOS);
		DROP;
		NEXT;
		break;
	case PUTS:
		putnumstr(disk+TOS);
		DROP;
		NEXT;
		break;
	case ATOI:
		w = TOS+1;
		
		TOS = (cell) strtol((char *)(disk+w),&s, *base);
		if(s == (char *)(disk+w)){
			DROP;
			mputs("NO NUMBER FOUND: ");
			putnumstr(disk+w-1);
			puts("");
		}
		NEXT;
		break;
	case PUNUM:
		puts(itoa(TOS));
		DROP;
		NEXT;
		break;
	case AND:
	       	NTOS &= TOS;
		DROP;
		NEXT;
		break;
	case OR:
		NTOS |= TOS;
		DROP;
		NEXT;
		break;
	case XOR:
		NTOS ^= TOS;
		DROP;
		NEXT;
		break;
	case NOT:
		TOS = ~TOS;
		NEXT;
		break;
	case DUMP:
		cstring(disk+TOS);
		DROP;
		f = fopen(strbuf, "wb");
		fwrite(disk + NTOS, sizeof(cell), TOS, f);
		fclose(f);
		DROP;
		DROP;
		NEXT;
		break;
	case ADUMP:
		cstring(disk+TOS);
		DROP;
		f = fopen(strbuf, "ab");
		fwrite(disk + NTOS, sizeof(cell), TOS, f);
		fclose(f);
		DROP;
		DROP;
		NEXT;
		break;
	case FSIZE:
		cstring(disk+TOS);
#ifdef POSIX
		stat(strbuf, &st);
		TOS = (st.st_size/sizeof(cell)) + 1;
#else
		f = fopen(strbuf, "rb");
		fseek(f, 0, SEEK_END);
		TOS = (ftell(f)/sizeof(cell)) + 1;
		fclose(f);
#endif
		NEXT;
		break;
	case LOADN: 
		cstring(disk+TOS);
		DROP;
		w = *dict;
		*dict += TOS;
		f = fopen(strbuf, "rb");
		fread(disk+w, sizeof(cell), TOS, f);
		fclose(f);
		NEXT;
		break;
	default: //this should be unreachable
		mputs("execute fallthrough: ");
		puts(itoa(op));
		dumpstack(3,tosp);
		dumpstack(5,rsp);
		dumpstack(15, disk+(*link));
		mputs("IP = ");
		puts(itoa(IP));
		state();
		*rsp = DSIZE-(RSSIZE+STSIZE+1);
		IP = cs;
	}
}

void pinit() {
	for(int i = DOCOL; i <= LOADN; i++) {
		switch(i){
		case DOCOL:
			enter(i);
			break;
		default:
			intern(i, 0);
		}
		primaddr[i] = *dict - 1;
	}
}

void finit(){
	//push the address of the next open cell to the stack
	COLON(here);
	COMPPRIM(LIT);
	enter(0);
	COMPPRIM(PEEK);
	COMPPRIM(EXIT);
	//compile TOS
	COLON(comptos);
	enter(here);
	COMPPRIM(POKE);
	enter(here);
	COMPPRIM(LIT);
	enter(1);
	COMPPRIM(PLUS);
	COMPPRIM(LIT);
	enter(0);
	COMPPRIM(POKE);
	COMPPRIM(EXIT);
	//turn a nonzero value to -1, and keep zero values
	COLON(logify);
	COMPPRIM(LIT);
	enter(0);
	COMPPRIM(EQL);
	COMPPRIM(NOT);
	COMPPRIM(EXIT);
	//branch if false
	COLON(notbranch);
	COMPPRIM(LIT);  //I could have used logify and then not, but there's an extra 'not' in logify, so for efficiency I just implemented it inline.
	enter(0);
	COMPPRIM(EQL);
	COMPPRIM(PUSNXT);
	COMPPRIM(AND); //compute branch value depending on the boolean found on the stack.
	COMPPRIM(FROMR);
	COMPPRIM(PLUS);
	COMPPRIM(TOR);
	//add the branch value to the return value and store it back on the return stack, overwriting the old value.
	COMPPRIM(EXIT);
	//peek xt
	COLON(peekxt);
	COMPPRIM(LIT);
	enter(127);
	COMPPRIM(WORD);
	COMPPRIM(DUP);
	enter(comptos);
	COMPPRIM(FIND);
	enter(here);
	COMPPRIM(LIT);
	enter(1);
	COMPPRIM(MINUS);
	COMPPRIM(PEEK);
	COMPPRIM(LIT);
	enter(0);
	COMPPRIM(POKE);
	COMPPRIM(EXIT);
	//execute xt
	COLON(excut);
	cell extspace = *dict + 3;
	COMPPRIM(LIT);
	enter(extspace);
	COMPPRIM(POKE);
	enter(0);
	COMPPRIM(EXIT);
	//interpret
	intern(DOCOL, -1);
	COMPPRIM(FROMR);
	COMPPRIM(FROMR);
	COMPPRIM(FROMR);
	COMPPRIM(PDROP);
	COMPPRIM(PDROP);
	COMPPRIM(PDROP);
	enter(*dict+1);
	enter(0);
	cell intloop = *dict-1;
	enter(peekxt);
	COMPPRIM(LIT);
	enter(1);
	COMPPRIM(EQL);
	COMPPRIM(NOT);
	IF(intfound);
	enter(excut);
	ELSE(intfound);
	COMPPRIM(ATOI);
	THEN(intfound);
	COMPPRIM(LIT);
	enter((cell)'\n');
	COMPPRIM(LIT);
	enter((cell)'k');
	COMPPRIM(LIT);
	enter((cell)'o');
	COMPPRIM(EMIT);
	COMPPRIM(EMIT);
	COMPPRIM(EMIT);
	COMPPRIM(FROMR);
	COMPPRIM(PDROP);
	enter(intloop);
	//compile
	intern(DOCOL, 0);
	COMPPRIM(FROMR);
	COMPPRIM(FROMR);
	COMPPRIM(FROMR);
	COMPPRIM(PDROP);
	COMPPRIM(PDROP);
	COMPPRIM(PDROP);
	enter(*dict+1);
	enter(0);
	cell comploop = *dict-1;
	enter(peekxt);
	COMPPRIM(DUP);
	COMPPRIM(LIT);
	enter(1);
	COMPPRIM(EQL);
	COMPPRIM(NOT);
	IF(compfound);
	IF(compimm);
	enter(excut);
	ELSE(compimm);
	enter(comptos);
	ELSE(compfound);
	COMPPRIM(PDROP);
	COMPPRIM(LIT);
	COMPPRIM(LIT);
	enter(comptos);
	COMPPRIM(ATOI);
	enter(comptos);
	THEN(compfound); THEN(compimm);
	COMPPRIM(FROMR);
	COMPPRIM(PDROP);
	enter(comploop);
	//colon compiler
	COLON(colon);
	enter(here);
	COMPPRIM(LIT);
	enter(3);
	COMPPRIM(DUP);
	COMPPRIM(PEEK);
	enter(comptos);
	COMPPRIM(POKE);
	COMPPRIM(LIT);
	enter(127);
	COMPPRIM(WORD);
	COMPPRIM(PDROP);
	COMPPRIM(LIT);
	enter(0);
	COMPPRIM(DUP);
	enter(comptos);
	enter(comptos);
	COMPPRIM(EXIT);
	//cold start to setup interpreter
	mputs("cs @ ");
	puts(itoa(*dict));
	cell coldstart = *dict;
	enter(DOCOL);
	enter(intloop);

	IP=coldstart;
	cs=coldstart;
}

int main() {
	pinit();
	finit();
	while(1) {
		//mputs("ip = ");
		//mputs(itoa(disk[IP]));
		execute(IP);
		//mputs(" rstack = ");
		//puts(itoa(*rsp - (DSIZE-(RSSIZE+STSIZE+1))));
		//state();
	}
}
