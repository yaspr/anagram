all: a_op

a:
	gcc -std=c99  anagram.c -o anagram

a_op:
	gcc -std=c99 -O3 anagram.c -o anagram

clean:
	rm *~ anagram
