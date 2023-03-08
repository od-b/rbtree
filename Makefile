TREE_SRC = src/tree_redblack.c
INCLUDE = include compare.c
INCLUDE := $(patsubst %.c,src/%.c, $(INCLUDE))

# normal/debug flags
CFLAGS = -Wall -Wextra -g -Wpedantic
LDFLAGS = -DLOG_LEVEL=0 -DERROR_FATAL

# optimization flags
CFLAGS_O = -O3
LDFLAGS_O = -DLOG_LEVEL=4

all: tree

tree: $(TREE_SRC) Makefile
	gcc -o $@ $(CFLAGS) $(TREE_SRC) -I$(INCLUDE) $(LDFLAGS)

clean:
	rm -f *~ *.o *.exe *.app tree
	rm -r *.dSYM/
