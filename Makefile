CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g -I.

ifeq ($(OS),Windows_NT)
PYTHON ?= python
else
PYTHON ?= python3
endif

VECTOR_OBJ = lab3/vector/generic.o

OBJ_SHARED = $(VECTOR_OBJ) posting.o avl/avl.o rbtree/rbtree.o btree/btree.o \
             index/index.o index/search.o

.PHONY: all app u_tests test clean

all: app u_tests

app: $(OBJ_SHARED) main.o
	$(CC) $(CFLAGS) -o app $(OBJ_SHARED) main.o

test_avl: $(VECTOR_OBJ) posting.o avl/avl.o avl/tests.o
	$(CC) $(CFLAGS) -o test_avl $(VECTOR_OBJ) posting.o avl/avl.o avl/tests.o

test_rb: $(VECTOR_OBJ) posting.o rbtree/rbtree.o rbtree/tests.o
	$(CC) $(CFLAGS) -o test_rb $(VECTOR_OBJ) posting.o rbtree/rbtree.o rbtree/tests.o

test_btree: $(VECTOR_OBJ) posting.o btree/btree.o btree/tests.o
	$(CC) $(CFLAGS) -o test_btree $(VECTOR_OBJ) posting.o btree/btree.o btree/tests.o

u_tests: test_avl test_rb test_btree
	./test_avl
	./test_rb
	./test_btree

test: app
	@echo "=== E2E: preprocessing ==="
	mkdir -p data/test
	$(PYTHON) preprocess.py \
		--input  data/test/Questions.csv \
		--output data/test/docs.jsonl
	@echo "=== E2E: indexing ==="
	./app index --type=avl   --data=data/test/docs.jsonl --index=data/test/idx_avl.txt
	./app index --type=rb    --data=data/test/docs.jsonl --index=data/test/idx_rb.txt
	./app index --type=btree --data=data/test/docs.jsonl --index=data/test/idx_btree.txt
	@echo "=== E2E: searching ==="
	./app search --type=avl   --index=data/test/idx_avl.txt   --json "python list"
	./app search --type=rb    --index=data/test/idx_rb.txt    --json "python list"
	./app search --type=btree --index=data/test/idx_btree.txt --json "python list"
	@echo "=== E2E OK ==="

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f app test_avl test_rb test_btree
	rm -f *.o avl/*.o rbtree/*.o btree/*.o index/*.o lab3/vector/*.o
	rm -f data/index_*.txt data/test/docs.jsonl data/test/idx_*.txt data/bench_*.jsonl data/bench_*.txt
