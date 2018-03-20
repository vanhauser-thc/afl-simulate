all:	afl-simulate

afl-simulate:	afl-simulate.c
	gcc -O3 -o afl-simulate afl-simulate.c

install: all
	cp -v afl-simulate /usr/local/bin

clean:
	rm -f afl-simulate *~
