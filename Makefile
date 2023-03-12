TREE_SRC = src/tree_test.c
INCLUDE = include compare.c tree_redblack.c
INCLUDE := $(patsubst %.c,src/%.c, $(INCLUDE))

# normal/debug flags
CFLAGS_W = -Wall -Wextra -g -Wpedantic
LDFLAGS_W = -DLOG_LEVEL=0 -DERROR_FATAL

# optimization flags
CFLAGS_O = -O3
LDFLAGS_O = -DLOG_LEVEL=4

all: tree

tree: $(TREE_SRC) Makefile
	gcc -o $@ $(CFLAGS_W) $(TREE_SRC) -I$(INCLUDE) ${LDFLAGS_W}

clean:
	rm -f *~ *.o *.exe *.app tree
	rm -r *.dSYM/
