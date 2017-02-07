CC = gcc
LEX = flex
YACC = bison
RM = rm -f

CFLAGS = -Wall -Wextra -c
LDFLAGS = -lfl

PROGRAM = fcc

SRCDIR = src

PARSE_SRC = $(SRCDIR)/parse.c
PARSE_H = $(SRCDIR)/parse.h
SCAN_SRC = $(SRCDIR)/scan.c
SCAN_H = $(SRCDIR)/scan.h

_OBJ = fcc.o ast.o symtab.o parse.o scan.o
OBJ = $(patsubst %,$(SRCDIR)/%,$(_OBJ))

all: parser compiler

$(PARSE_SRC) $(PARSE_H): $(SRCDIR)/feeble-c.y
	$(YACC) --defines=$(PARSE_H) -o $(PARSE_SRC) $<

$(SCAN_SRC) $(SCAN_H): $(SRCDIR)/feeble-c.l $(PARSE_H)
	$(LEX) --header-file=$(SCAN_H) -o $(SCAN_SRC) $<

parser: $(PARSE_SRC) $(PARSE_H) $(SCAN_SRC)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

compiler: $(OBJ)
	$(CC) -o $(PROGRAM) $^ $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) $(PARSE_SRC) $(PARSE_H) $(SCAN_SRC) $(SCAN_H) $(OBJ) $(PROGRAM)