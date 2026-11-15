.PHONY: all clean

CC=clang
CARGS=-std=c99 -Wall -pedantic

BUILD=out
APP=app
DBG_APP=dbg

SOURCE=*.c lib/*.c

all: build
	@echo 'built successfully'
	@echo 'app put into $(BUILD)/$(APP)'

build: $(SOURCE)
	mkdir -p $(BUILD)
	$(CC) $(CARGS) *.c -o $(BUILD)/$(APP)

build-dbg: $(SOURCE)
	mkdir -p $(BUILD)
	$(CC) -DDBG $(CARGS) *.c -o $(BUILD)/$(DBG_APP)

clean:
	rm -rf *.out $(BUILD) *.log
