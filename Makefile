CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lm

SRC = c_src/transaction.c c_src/fraud_rules.c c_src/linked_list.c c_src/graph.c \
      c_src/queue.c c_src/stack.c c_src/bst.c c_src/hash_map.c c_src/algorithms.c c_src/c_engine.c

all: bin/c_engine bin/bank_cli

bin/c_engine: $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o bin/c_engine $(LDFLAGS)

bin/bank_cli: $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o bin/bank_cli $(LDFLAGS)

clean:
	rm -rf bin/c_engine bin/bank_cli bin/*.exe
