CC=	gcc
CFLAGS= -O3 -Wall -fomit-frame-pointer
LD=     gcc
LDFLAGS=-s

all:	text2bas.exe

OBJS = text2bas.o

text2bas.exe:	$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

%.o:	%.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o text2bas *.bak

export:     clean
	zip ../text2bas *.*

