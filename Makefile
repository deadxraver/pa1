.PHONY: all clean

CC=clang
CARGS=-std=c99 -Wall -pedantic

all: build
	@echo 'built successfully'

build: *.c *.h
	$(CC) $(CARGS) *.c

clean:
	rm -rf *.out
