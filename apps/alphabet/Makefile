
.PHONY: all clean
all: alphabet

alphabet: main.o print_alphabet.o create_alphabet.o
	linker -o alphabet main.o print_alphabet.o create_alphabet.o

main.o: alphabet.c alphabet.h
	il_gen -o main.o alphabet.c

print_alphabet.o: print_alphabet.c alphabet.h
	il_gen -o print_alphabet.o print_alphabet.c

create_alphabet.o: create_alphabet.c alphabet.h
	il_gen -o create_alphabet.o create_alphabet.c

clean:
	rm -rf alphabet main.o print_alphabet.o create_alphabet.o
