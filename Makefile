.PHONY: all clean

BUILD=out
APP=app
DBG_APP=dbg

SOURCE=*.c lib/*.c
LD_LIBRARY_PATH=$(CURDIR)/$(BUILD)
SO_DIR=$(CURDIR)/lib/lib64
LD_PRELOAD=$(SO_DIR)/libruntime.so

CC=clang
CARGS=-std=c99 -Wall -pedantic -L$(SO_DIR) -lruntime

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
