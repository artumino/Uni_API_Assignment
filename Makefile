PROG = main.c fs_utils.c local_random.c

default: all

all:
	gcc -DEVAL -static -std=c99 -O2 $(PROG) -lm -o main

run:
	main $(ARGS)
