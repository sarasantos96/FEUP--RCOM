#ifndef APPLICATIONLAYER_H
#define APPLICATIONLAYER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "dataLayer.h"

int initApplicationLayer(unsigned int timeout,unsigned int numTransmissions, char * port, int status);
int writeApp(char* filepath, unsigned int namelength);
int readApp();
int closeAppError();
int closeApplicationLayer();
void createInfoPackage(unsigned char *package,unsigned char *buf, int size);
int sendControlPacket(unsigned char controlFlag);
int receiveControlPacket(unsigned char controlFlag);

#endif
