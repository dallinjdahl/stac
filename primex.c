#include <stdio.h>
#define PRIMS "primitives"
char s[15] = {'\0'}, si[4] = {'0', '\0'}, *sa[6] = {"#define ", s, " ", si, "\n", NULL};
int mputs(char **z){
	for(int i = 0; z[i]; i++) {
		fputs(z[i], stdout);
	}
	return 0;
}
int mgets(char *z, int i){
	for(int j = 0; j < (i-1) ; z++,j++){
		*z = getchar();
		switch (*z){
		case '\n':
			*z = '\0';
			return j;
		case EOF:
			*z = '\0';
			return -1;
		}
	}
	z[i-1] = '\0';
	return i-1;
}

char *itoa(int i){
        char *p = si;
        if(i<0){
		*(p++) = '-';
		i *= -1;
	}
        int shifter = i;
        do{ //Move to where representation ends
		++p;
		shifter /= 10;
	} while(shifter);
        *p = '\0';
        do{ //Move back, inserting digits as you go
		*(--p) = (char)((i%10)+48);
		i /= 10;
	} while(i);
        return si;
}

int main(){
	int e = 0;
	int i = 0;
	while(1){
		itoa(i);
		if (mgets(s, 15) < 0){
			return 0;
		}
		mputs(sa);
		i++;
	}
}
