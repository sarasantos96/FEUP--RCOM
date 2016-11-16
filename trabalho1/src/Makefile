CC = gcc

CFLAGS= -Wall

RM = rm -f

all: transmitter.o receiver.o

transmitter.o: alarme.c dataLayer.c ApplicationLayer.c writenoncanonical.c
		$(CC) $(CFLAGS) -o transmitter alarme.c dataLayer.c ApplicationLayer.c writenoncanonical.c -lm

receiver.o:  alarme.c dataLayer.c ApplicationLayer.c noncanonical.c
		 $(CC) $(CFLAGS) -o receiver alarme.c dataLayer.c ApplicationLayer.c noncanonical.c -lm
