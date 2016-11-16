/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ApplicationLayer.h"

int main(int argc, char** argv){
	if ( (argc < 2) ||
		 ((strcmp("/dev/ttyS0", argv[1])!=0) &&
		  (strcmp("/dev/ttyS1", argv[1])!=0) )) {
	  printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
	  exit(1);
	}

	initApplicationLayer(3, 3, argv[1], RECEIVER);
	int res = readApp();
	if(res != 0){
		closeAppError();
	}else{
		closeApplicationLayer();
	}
	return 0;
}
