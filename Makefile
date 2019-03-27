CCOPTS = -std=c99 -pedantic
CC = gcc $(CCOPTS)
EXES = stac getsizes primex
GENS = primdefs.h sizedefs.h

stac: sizedefs.h primdefs.h sta.c
	$(CC) sta.c -o stac

debug: sizedefs.h primdefs.h sta.c
	$(CC)  sta.c -o stac -ggdb

primdefs.h: primex genprim.sh sta.c
	./genprim.sh <sta.c | ./primex >primdefs.h

primex: primex.c
	$(CC) primex.c -o primex

sizedefs.h: getsizes.c 
	$(CC) getsizes.c -o getsizes
	./getsizes > sizedefs.h

clean:
	rm -rfv $(EXES) $(GENS)
