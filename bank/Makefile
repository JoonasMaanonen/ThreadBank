IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR) -Wall -pedantic -pthread

ODIR=obj
LDIR=../lib

_LIBS= errorHandler.c
LIBS = -lm $(patsubst %,$(LDIR)/%,$(_LIBS))

_DEPS = errorHandler.h parser.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = bankServer.o parser.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

bankServer: $(OBJ)
		gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
		rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
