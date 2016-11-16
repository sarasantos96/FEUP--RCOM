#ifndef UTILITIES_H
#define UTILITIES_H

#define TRANSMITTER	0
#define RECEIVER	1
#define MAX_SIZE	256
#define BAUDRATE B9600
#define MODEMDEVICE  "/dev/ttyS1"
#define CONTROL_LENGTH 5
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 	0
#define TRUE 	1
#define FLAG 	0x7E
#define A 					0x03
#define A_RESPONSE 	0x01
#define C_SET 	0x03
#define C_UA 	0x07
#define C_DISC  0x0B
#define C_RR 0x05
#define C_REJ 0x01
#define START	1
#define STP 	2
#define SET_STATUS 0
#define DISC_STATUS 1
#define UA_STATUS 2
#define RR_STATUS 3
#define REJ_STATUS 4

//BYTE STUFFING
#define ESCAPE		0x7d
#define XORFLAG 	(FLAG ^ 0x20)
#define XORESC 		(ESCAPE ^ 0x20)

//CONTROL FLAG C
#define START_PACKET 2
#define END_PACKET 	3
#define DATA_PACKET 1

//CONTROL FLAG T
#define T_SIZE	0
#define T_FILENAME	1

enum MachineStates{
	STR, FLAG_RCV, A_RCV, C_RCV, BCC_OK, ST
};

struct applicationLayer {
	int fileDescriptor; /*Descritor correspondente à porta série*/
	int status; /*TRANSMITTER | RECEIVER*/
	unsigned int numTransmissions;
	unsigned int timeout; /*Valor do temporizador: 1 s*/
};

struct File{
  unsigned int fileNameLength;
  char* fileName;
  unsigned int filelength;
  char* buf;
};

enum Control{
	SET_CONTROL,
	UA_CONTROL,
	RR_CONTROL,
	REJ_CONTROL,
	DISC_CONTROL,
	ERROR_CONTROL
};

#endif
