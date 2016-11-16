#ifndef DATALAYER_H
#define DATALAYER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utilities.h"
#include "alarme.h"

struct linkLayer {
	char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
	int baudRate; /*Velocidade de transmissão*/
	unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
	unsigned int timeout;
	char frame[MAX_SIZE]; /*Trama*/
	unsigned char machineState;
	unsigned char ns;
};

extern struct linkLayer layer;

void inicializa_layer(unsigned int maxTransm, unsigned int timeout);
void byte_stuffing(unsigned char *data, unsigned int i,unsigned char byte);
unsigned int byte_destuffing(unsigned char* destuff,unsigned char* stuff, unsigned int length);
int extractMessage(unsigned char* message,unsigned char* package, unsigned int length, unsigned char nr);
int llopen(char* porta,int status,int perm, unsigned int maxTransm, unsigned int timeout);
int llclose(int fd);
int llwrite(int fd, unsigned char* buffer,unsigned int length);
int llread(int fd, unsigned char* message);
int sendControl(int fd, enum Control control);
int receivePackage(int fd,unsigned char *buf);
unsigned char receiveControl(int fd,int status);
void updateStateReceiveRR(unsigned char state);
void updateStateReceiveUA(unsigned char state);
void updateStateReceiveSET(unsigned char state);
void updateStateReceiveDISC(unsigned char state);
int buildData(int fd, unsigned char *package,unsigned char *data, unsigned int size, char control);
unsigned char getBCC2(unsigned char *buffer, unsigned int length);


#endif
