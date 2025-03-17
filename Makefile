override CFLAGS = -Og -g -Wall -Wextra -Wno-unused 
# override CFLAGS += -O2 -Wall -Wno-unused

TREE = rbtree
TEST = rbtreetest

TEST_SRC = tests/$(TEST).c
TREE_SRC = src/$(TREE).c
TREE_HEADER = src/$(TREE).h

all: runtest

$(TREE): $(TREE_SRC) $(TREE_HEADER) Makefile
	gcc -o $@ $(TREE_SRC) $(CFLAGS) -I $(TREE_HEADER)

$(TEST): $(TEST_SRC) $(TREE_SRC) $(TREE_HEADER) Makefile
	gcc -o $@ $(TEST_SRC) $(CFLAGS) -I $(TREE_HEADER) $(TREE_SRC)

runtest: $(TEST)
	./$(TEST)

clean:
	rm -rf *~ *.o *.dSYM $(TREE) $(TEST)
