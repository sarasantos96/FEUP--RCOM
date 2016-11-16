/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ApplicationLayer.h"

struct linkLayer layer;

int main(int argc, char** argv){
    if ( (argc < 3) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 filepath\n");
      exit(1);
    }
    FILE *f;
    f = fopen(argv[2],"r");
    if(f == NULL){
      printf("Ficheiro inválido\n");
      exit(1);
    }
    fclose(f);
	  initApplicationLayer(3, 3, argv[1],TRANSMITTER);
    writeApp(argv[2], strlen(argv[2]));
    closeApplicationLayer();

    return 0;
}
