#
# Makefile for BiPartGen
#

CC = gcc
# CFLAGS = -g -O2 -Wall -Werror -Wno-unused-function -Wno-unused-parameter -std=c99
CFLAGS = -g -O3 -Wall -Werror -Wno-unused-function -Wno-unused-parameter -std=c99

FILES = src/bipartgen.o src/mchess.o src/pigeon.o src/additionalgraphs.o src/graph.o src/xmalloc.o

TESTDIR = tests

bipartgen: src/bipartgen.o src/mchess.o src/pigeon.o src/additionalgraphs.o src/graph.o src/xmalloc.o
	$(CC) $(CFLAGS) -o bipartgen $(FILES)

bipartgen.o: src/bipartgen.c src/mchess.o src/pigeon.o src/graph.o src/xmalloc.o
mchess.o: src/mchess.c src/mchess.h src/graph.o src/xmalloc.o
pigeon.o: src/pigeon.c src/pigeon.h src/graph.o src/xmalloc.o
additionalgraphs.o: src/additionalgraphs.c src/graph.o src/xmalloc.o
graph.o: src/graph.c src/graph.h src/xmalloc.o
xmalloc.o: src/xmalloc.c src/xmalloc.h

test: bipartgen graph_test mchess_test

graph_test: graph_test.o src/graph.o src/xmalloc.o
	$(CC) $(CFLAGS) -o $(TESTDIR)/graph_test $(TESTDIR)/graph_test.o src/graph.o src/xmalloc.o

graph_test.o: $(TESTDIR)/graph_test.c src/graph.o src/xmalloc.o

clean:
	rm -rf src/*.o
	rm -rf bipartgen
