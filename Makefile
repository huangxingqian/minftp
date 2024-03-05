.RHNOY:clean
BIN	=main
CC	=gcc
CFLAGS	=-Wall -g
SUBDIR	=$(shell ls -d ./)
SUBSRC	=$(shell find $(SUBDIR) -name '*.c')
SUBOBJ	=$(SUBSRC:%.c=%.o)
INCDIR	=-I ./inc

$(BIN):$(ROOTOBJ) $(SUBOBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(SUBOBJ)

.c.o:
	$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@

clean:
	rm -f $(BIN) $(ROOTOBJ) $(SUBOBJ)