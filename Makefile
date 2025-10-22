CFLAGS=-Wall -Wextra -O2 -Iinclude
CC=gcc

SRC := $(shell find . -name "*.c")
OBJ := $(patsubst %,build/%,$(SRC:.c=.o))
DEP := $(OBJ:.o=.d)
EXE := app

.PHONY: all clean

all: $(EXE)

build/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

clean:
	rm -rf build $(EXE)

-include $(DEP)
