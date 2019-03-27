#include <stdio.h>

int main(){
	int pack = sizeof (int) / sizeof (char);
	int diff = sizeof (int) - (sizeof (char) * pack);
	printf("#define PACK %d\n#define DIFF %d\n", pack, diff);
	return 0;
}
