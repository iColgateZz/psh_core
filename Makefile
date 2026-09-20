CFLAGS=-std=c99 -Wall -Wextra -O2 -Wno-override-init
CC=cc

SRC := $(shell find . -name "*.c")
OBJ := $(patsubst %,build/%,$(SRC:.c=.o))
DEP := $(OBJ:.o=.d)
EXE := app

.PHONY: all clean test

all: $(EXE)

build/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

test:
	CC="$(CC)" sh tests/run.sh

clean:
	rm -rf build $(EXE)

-include $(DEP)
