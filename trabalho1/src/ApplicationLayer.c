#include "ApplicationLayer.h"

struct File file;
struct applicationLayer app;

int initApplicationLayer(unsigned int timeout,unsigned int numTransmissions, char * port, int status){
	app.numTransmissions = numTransmissions;
	app.timeout = timeout;
	app.status = status;
  int perm;
	if(status == TRANSMITTER)
		perm = O_RDWR | O_NOCTTY | O_NONBLOCK;
	else if( status == RECEIVER)
		perm = O_RDWR | O_NOCTTY;

  int fd = llopen(port, status,perm,numTransmissions, timeout);
  if(fd == -1){
    perror("Erro a abrir port.\n");
    exit(-1);
  }
	app.fileDescriptor = fd;

  if(status == TRANSMITTER){
    int flag_recebeu = 0;
    if(sendControl(app.fileDescriptor, SET_CONTROL) != 0) printf("Erro enviar\n");
  	else printf("Enviou set\n");
  	setAlarm(numTransmissions,timeout);

  	printf("A receber trama...\n");
    int nTries = getNumTries();
  	while(nTries < 3 && flag_recebeu == 0){
  		int state = receiveControl(app.fileDescriptor,UA_STATUS);
  		if(state == STP){
  			flag_recebeu = 1;
  			printf("Recebeu UA\n");
  		}else if(getAlarmeDispara()){
        if(sendControl(app.fileDescriptor, SET_CONTROL) != 0) printf("Erro enviar\n");
        setAlarmeDispara(0);
      }
      nTries = getNumTries();
  	}

  	if(nTries == 3)
  		printf("Transmissao falhada. A terminar...\n");

  	closeAlarm();
  }else if(status == RECEIVER){
    int flag_recebeu = 0;
  	printf("New termios structure set\n");

  	printf("Waiting for SET...\n");

  	while(!flag_recebeu){
  		int state = receiveControl(fd,SET_STATUS);
  		if(state == STP){
  			printf("Recebeu SET. A enviar UA...\n");
  			if(sendControl(fd,UA_CONTROL) == -1){
					printf("Erro no envio \n");
				}
  			flag_recebeu = 1;
  		}else{
  			printf("Não recebeu trama correta\n");
  		}
  	}
  }

  return 0;
}

int writeApp(char* filepath,unsigned int namelength){
  file.fileName = filepath;
  file.fileNameLength = namelength;

	FILE* f;

	f = fopen(file.fileName, "rb");
	if(f == NULL){
		printf("Erro na abertura do ficheiro\n");
		return -1;
	}
	//Guarda o tamanho do ficheiro
  fseek(f, 0, SEEK_END); //Coloca apontador no final do ficheiro
  file.filelength = ftell(f); //Guarda o tamanho do ficheiro
	fseek(f,0,SEEK_SET); //Coloca apontador no inicio do ficheiro

	unsigned char *fileBuf = (unsigned char *)malloc(file.filelength);
	fread(fileBuf, sizeof(char),file.filelength, f);
	if(fclose(f)!= 0){
		printf("Erro a fechar o ficheiro\n");
		return -1;
	}

	int res = sendControlPacket(START_PACKET);
	if( res == -1){
		printf("Erro no envio do pacote START\n");
		exit(-1);
	}

	printf("Start sending file...\n");

	int maxDataSize = MAX_SIZE - 5;
	int totalPacks = ceil((float) file.filelength / maxDataSize); //tamanho de ficheiro / (256-flags)
	int bytesRem = file.filelength;
	int bytesWritten = 0;
	int i;
	unsigned char package[MAX_SIZE];
	int flag_error = 0;

	for(i = 0; i < totalPacks; i++){
		printf("A enviar pacote nr %d\n", i+1);
		int length;

		if(bytesRem < maxDataSize){
			length = bytesRem;
		}else{
			length = maxDataSize;
		}

		createInfoPackage(package, &fileBuf[bytesWritten], length);

		res = llwrite(app.fileDescriptor, package, length + 5);
		if(res == -1){
			printf("Erro a enviar pacote.\n");
			flag_error = 1;
			break;
		}

		bytesRem -= length;
		bytesWritten += length;
		printf("Enviou pacote nr %d\n", i+1);
	}

	//Envia control END
	if(!flag_error){
		int end = sendControlPacket(END_PACKET);
		if(end == -1){
			printf("Erro a enviar controlo END\n");
			return -1;
		}
	}

  return 0;
}

void createInfoPackage(unsigned char *package, unsigned char *buf, int size){
	static unsigned char seqN = 0;
	unsigned char c = DATA_PACKET;
	unsigned char l2 = (size >> 8) & 0xFF;
	unsigned char l1 = size & 0xFF;

	package[0]=c;
	package[1]=seqN++;
	package[2]=l2;
	package[3]=l1;
	memcpy(&package[4],buf,size);
}

int readApp(){
    //Receive Packet START
    int start = receiveControlPacket(START_PACKET);

		if(start != 0){
			return -1;
		}

	//	printf("file length: %d\n filename: %s\n", file.filelength, file.fileName);
		//creates and opens file
		FILE *f = fopen(file.fileName, "wb");
		if(f == NULL){
			printf("Erro a criar ficheiro\n");
			return -1;
		}

		int maxDataSize = MAX_SIZE - 5;
		int totalPacks = ceil((float) file.filelength / maxDataSize); //tamanho de ficheiro / (256-flags)
		int i = 0;
		unsigned char fileBuf[MAX_SIZE];

		for(; i < totalPacks; i++){
			printf("A receber pacote nr %d\n", i+1);

			int length = llread(app.fileDescriptor, fileBuf);
			if(length == -1){
				i--;
			}else if(length == -2){
				return -2;
			}else{
				int j = 0;
				for(; j < length - 5; j++){
					fprintf(f, "%c", fileBuf[4 + j]);
				}
				printf("Pacote nr %d recebido\n", i+1);
			}
		}

		//Acabou de ler o ficheiro
		int close = fclose(f);
		if(close != 0){
			printf("Erro a fechar o ficheiro\n");
		}

		printf("File sucessfully received\n");

		//Espera por controlo END
		int end = receiveControlPacket(END_PACKET);
		if(end != 0){
			return -1;
		}

		return 0;
}

int sendControlPacket(unsigned char controlFlag){
	if(controlFlag ==  START_PACKET){
		printf("Sending START\n");
	}else if(controlFlag == END_PACKET){
		printf("Sending END\n");
	}

	//Creates a new Control packet
	char aux[16];
	sprintf(aux, "%d", file.filelength);
	unsigned int size = 5 + strlen(aux) + file.fileNameLength;
	unsigned char package[size];

	package[0] = controlFlag;
  package[1] = T_SIZE; // 0 - tamanho do ficheiro
  package[2] = strlen(aux);
	unsigned i =0, j = 3;
  for(; i < strlen(aux); i++,j++){
    package[j] = aux[i];
  }
  package[j] = T_FILENAME; // 1- nome do ficheiro
  j += 1;
  package[j] = file.fileNameLength;

  j +=1;
  for(i=0; i< file.fileNameLength; i++, j++){
  	package[j] = file.fileName[i];
  }

	int res = llwrite(app.fileDescriptor, package, size);
	if(res == -1){
		printf("Erro no envio do pacote de controlo\n");
		return -1;
	}else{
		printf("Enviou pacote de controlo\n");
	}
	return 0;
}

int receiveControlPacket(unsigned char controlFlag){
  unsigned char *packet = malloc(MAX_SIZE*2);
	unsigned int res = llread(app.fileDescriptor, packet);

	if(res == -1){
		printf("Erro a ler package\n");
		return -1;
	}

	if(packet[0] == START_PACKET){
		printf("START packet received\n");
	}else if(packet[0] == END_PACKET){
		printf("END packet received\n");
	}

	int l;
	if(packet[1] == T_SIZE){
		//Aloca o numero de octetos necessários para guardar length
		l = packet[2];
		char *file_length = malloc(l);
		memcpy(file_length, &packet[3], l);
		file.filelength = atoi(file_length);
		free(file_length);
	}
	int newpos = 3+l;
	if(packet[newpos] == T_FILENAME){
		newpos++;
		l = packet[newpos];

		newpos++;
		char name[l];
		int p;
		for(p = 0; p < l; p++){
			name[p] = packet[newpos + p];
		}
		file.fileName = malloc(sizeof(char) * l);
		snprintf(file.fileName, l+1, "%s", name);
	}

	free(packet);
	return 0;
}

int closeAppError(){
	  if(sendControl(app.fileDescriptor, DISC_CONTROL) == -1){ printf("Erro no envio \n");}
		return 0;
}

int closeApplicationLayer(){
  if(app.status == TRANSMITTER){
    int flag_recebeu = 0;
    if(sendControl(app.fileDescriptor, DISC_CONTROL) != 0){
			 printf("Erro a enviar DISC\n");
			 return -1;
    }else printf("Enviou DISC\n");
    setAlarm(app.numTransmissions,app.timeout);

    printf("A aguardar DISC...\n");

    int nTries = getNumTries();
    while(nTries < 3 && flag_recebeu == 0){
      int state = receiveControl(app.fileDescriptor,DISC_STATUS);
      if(state == STP){
        flag_recebeu = 1;
        printf("Recebeu DISC...A terminar\n");
        if(sendControl(app.fileDescriptor, UA_CONTROL) != 0) printf("Erro enviar\n");
      }else if(getAlarmeDispara()){
        if(sendControl(app.fileDescriptor, DISC_CONTROL) != 0) printf("Erro enviar\n");
        setAlarmeDispara(0);
      }
      nTries = getNumTries();
    }

    if(nTries == 3)
      printf("Disconexão falhada. A terminar...\n");

    closeAlarm();
  }else if(app.status == RECEIVER){
    int flag_recebeu = 0;

    printf("Waiting for DISC...\n");

    while(!flag_recebeu){
      int state = receiveControl(app.fileDescriptor, DISC_STATUS);
      if(state == STP){
        printf("Recebeu DISC. A enviar DISC...\n");
        if(sendControl(app.fileDescriptor, DISC_CONTROL) == -1){ printf("Erro no envio \n");}
        flag_recebeu = 1;
      }
    }

    setAlarm(app.numTransmissions,app.timeout);
    flag_recebeu = 0;
    int nTries = getNumTries();
    while(nTries < 3 && !flag_recebeu){
      int state = receiveControl(app.fileDescriptor, UA_STATUS);
      if(state == STP){
        printf("Recebeu UA. A terminar...\n");
        flag_recebeu = 1;
      }else if(getAlarmeDispara()){
        if(sendControl(app.fileDescriptor, DISC_CONTROL) == -1){ printf("Erro no envio \n");}
        setAlarmeDispara(0);
      }
    }
    closeAlarm();
  }

  return 0;
}
