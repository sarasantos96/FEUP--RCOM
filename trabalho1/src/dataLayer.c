#include "dataLayer.h"

struct linkLayer layer;
struct termios oldtio,newtio;

volatile int STOP=FALSE;

void inicializa_layer(unsigned int maxTransm, unsigned int timeout){
	strcpy(layer.port, MODEMDEVICE);
	layer.baudRate = BAUDRATE;
	layer.machineState = START;
	layer.ns = 0;
	layer.numTransmissions = maxTransm;
	layer.timeout = timeout;
}

void byte_stuffing(unsigned char *data, unsigned int i,unsigned char byte){
	data[i] = ESCAPE;
	data[i++] = byte ^ 0x20;
}

unsigned int byte_destuffing(unsigned char* destuff,unsigned char* stuff, unsigned int length){
	int i = 0, j = 0;

	for(i = 0; i < length; i++, j++){
		if(stuff[i] == ESCAPE){
			i++;
			destuff[j] = (stuff[i] ^ 0x20);
		}else{
			destuff[j] = stuff[i];
		}
	}

	return j;

}

int extractMessage(unsigned char* message,unsigned char* package, unsigned int length, unsigned char nr){
	unsigned char tempBCC2 = 0x00;
	unsigned char ns;
	int i;

	ns = (nr + 1) % 2;
	if((package[2] >> 6) == nr){
		printf("Pacote repetido.\n");
		return 1;
	}else if(package[0] != FLAG
				|| package[1] != A
				|| package[2] !=(unsigned char) (ns << 6)
				|| package[3] != (package[1]^package[2])
				|| package[length-1] != FLAG){
		printf("Cabeçalho errado.\n");
		return -1;
	}

	i = 4;
	for(; i < length-2; i++){
		message[i - 4] = package[i];
		tempBCC2 = tempBCC2 ^ (unsigned char)package[i];
	}

	if(tempBCC2 != package[length-2]){
		printf("BCC2 errado\n");
		return -1;
	}

	return 0;
}

int llopen(char* porta,int status, int perm, unsigned int maxTransm, unsigned int timeout){
	inicializa_layer(maxTransm, timeout);
	int fd;
	if(status == TRANSMITTER){
		fd = open(porta,perm);
		if (fd <0) {perror("porta"); exit(-1);}
	}else if(status == RECEIVER){
		fd = open(porta, perm);
		if (fd <0) {perror("porta"); exit(-1);}
	}

	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
	  perror("tcgetattr");
	  exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = layer.baudRate | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */
  /*
	VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
	leitura do(s) pr�ximo(s) caracter(es)
  */

	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
	  perror("tcsetattr");
	  exit(-1);
	}
	return fd;
}

int llclose(int fd){
	tcsetattr(fd,TCSANOW,&oldtio);
	return close(fd);
}

int buildData(int fd,unsigned char *package,unsigned char *data, unsigned int size, char control){
	//Creates data package
	unsigned char bcc1 = A^control;
	unsigned char bcc2 = getBCC2(data, size);

	int i = 0, j;
	package[i] = FLAG;
	i++;
	package[i] = A;
	i++;
	package[i] = control;
	i++;
	if(bcc1 == FLAG || bcc1 == ESCAPE){
		package[i] = ESCAPE;
		i++;
		package[i] = bcc1 ^ 0x20;
		i++;
	}else{
		package[i] = bcc1;
		i++;
	}

	for(j = 0; j < size; j++,i++){
		if(data[j] == FLAG || data[j] == ESCAPE){
			package[i] = ESCAPE;
			i++;
			package[i] = data[j] ^ 0x20;
		}else{
			package[i] = data[j];
		}
	}


	if(bcc2 == FLAG || bcc2 == ESCAPE){
		package[i] = ESCAPE;
		i++;
		package[i] = bcc2 ^ 0x20;
		i++;
	}else{
		package[i] = bcc2;
		i++;
	}

	package[i] = FLAG;
	i++;
	return i;
}

unsigned char getBCC2(unsigned char *buffer, unsigned int length){
	unsigned char temp = 0x00;
	int i=0;
	for(;i < length;  i++){
		temp ^= buffer[i];
	}
	return temp;
}

int isRR(char control[]){
	if(control[0] == FLAG &&
		control[1] == A &&
		control[2] == C_RR &&
		control[3] == (A ^ C_RR) &&
		control[4] == FLAG){
		return 1;
	}
	return 0;
}

int isREJ(char control[]){
	if(control[0] == FLAG &&
		control[1] == A &&
		control[2] == C_REJ &&
		control[3] == (A ^ C_REJ) &&
		control[4] == FLAG){
		return 1;
	}
	return 0;
}

int llwrite(int fd,unsigned char* buffer, unsigned int length){
	static unsigned char ns = 0;
	int flag_recebeu = 0;
	unsigned char package[MAX_SIZE * 2];
	int size = buildData(fd, package, buffer, length, (ns << 6));

	int res = write(fd, package, size);
	sleep(1);
	if(res == 0 || res == -1){
		printf("Erro a enviar dados\n");
		return -1;
	}
	setAlarm(layer.numTransmissions, layer.timeout);

	int nTries = getNumTries();

	while(!flag_recebeu && nTries < layer.numTransmissions){
		receiveControl(fd, RR_STATUS);

		if(layer.machineState == STP){
			printf("Recebeu RR\n");
			flag_recebeu = 1;
			closeAlarm();
		}else if(getAlarmeDispara()){
			printf("Alarme disparou\n");
			res = write(fd, package, size);
			sleep(1);
			if(res == 0 || res == -1){
				printf("Erro a enviar dados\n");
			}
			setAlarmeDispara(0);
		}
	//	tcflush(fd, TCIOFLUSH);
		nTries = getNumTries();
	}

	ns = (ns + 1) % 2;

	if(nTries ==  layer.numTransmissions){
		printf("Conexão Falhada. Terminar...\n");
		return -1;
	}

	return 0;
}

int llread(int fd,unsigned char *message){
	static unsigned char nr = 1;
	unsigned char buf[MAX_SIZE * 2];
	int flag_recebeu = 0;
	int size, destuffedSize, m;
	while(!flag_recebeu){
		size = receivePackage(fd, buf);
		if(size == -1){
			return size;
		}else if(size == -2){
			printf("Recebeu DISC\n");
			return -2;
		}

		unsigned char destuffedBuf[MAX_SIZE];
		destuffedSize = byte_destuffing(destuffedBuf, buf, size);

		m = extractMessage(message, destuffedBuf, destuffedSize, nr);

		if(m == 0){
			flag_recebeu = 1;
			printf("enviou RR\n");
			sendControl(fd, RR_CONTROL);
			nr = (nr + 1) % 2;
		}else if(m == 1){
			printf("enviou RR rep\n");
			sendControl(fd, REJ_CONTROL);
			return -1;
		}else if(m == -1){
			sendControl(fd, REJ_CONTROL);
			return -1;
		}
	}

	return (destuffedSize - 6); //retorna tamanho da mensagem: tamanho total - flags
}

int receivePackage(int fd,unsigned char* buf){
	enum MachineStates state = STR;
	int i = 0, res;
	unsigned char byte;

	while(state != ST){
		res = read(fd, &byte, 1);
		if(res == -1 || res == 0){
			printf("Erro na leitura do pacote\n");
			return -1;
		}

		switch (state) {
			case STR:
				if(byte == FLAG){
					buf[i] = byte;
					i++;
					state = FLAG_RCV;
				}
				break;
			case FLAG_RCV:
				if(byte == A){
					buf[i] = byte;
					i++;
					state = A_RCV;
				}else if(byte != FLAG){
					state = STR;
				}
				break;
			case A_RCV:
				if(byte == C_DISC){
					return -2;
				}
				if(byte != FLAG){
					buf[i] = byte;
					i++;
					state = C_RCV;
				}else{
					state = FLAG_RCV;
				}
				break;
			case C_RCV:
				if(byte != FLAG){
					buf[i] = byte;
					i++;
					state = BCC_OK;
				}else{
					state = FLAG_RCV;
				}
				break;
			case BCC_OK:
				if(byte == FLAG){
					buf[i] = byte;
					i++;
					state = ST;
				}else{
					buf[i] = byte;
					i++;
				}
				break;
			default:
				break;
		}
	}

	return i;
}

int sendControl(int fd, enum Control control){
	unsigned char SET[] = {FLAG, A, C_SET, A^C_SET, FLAG};
	unsigned char UA[] = {FLAG, A, C_UA, A^C_UA, FLAG};
	unsigned char DISC[] = {FLAG, A, C_DISC, A^C_DISC, FLAG};
	unsigned char RR[] = {FLAG, A, C_RR, A^C_RR, FLAG};
	unsigned char REJ[] = {FLAG, A, C_REJ, A^C_REJ, FLAG};

	int res;
	switch (control) {
		case SET_CONTROL:
			res = write(fd, SET,CONTROL_LENGTH);
			sleep(1);
			if( res == 0 || res == -1){
				printf("Erro a enviar \n");
				return -1;
			}
			break;
		case UA_CONTROL:
			res = write(fd, UA,CONTROL_LENGTH);
			sleep(1);
			if( res == 0 || res == -1){
				printf("Erro a enviar \n");
				return -1;
			}
			break;
		case DISC_CONTROL:
			res = write(fd, DISC,CONTROL_LENGTH);
			sleep(1);
			if( res == 0 || res == -1){
				printf("Erro a enviar \n");
				return -1;
			}
			break;
		case RR_CONTROL:
			res = write(fd, RR,CONTROL_LENGTH);
			sleep(1);
			if( res == 0 || res == -1){
				printf("Erro a enviar \n");
				return -1;
			}
			break;
		case REJ_CONTROL:
			res = write(fd, REJ,CONTROL_LENGTH);
			sleep(1);
			if( res == 0 || res == -1){
				printf("Erro a enviar \n");
				return -1;
			}
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

unsigned char receiveControl(int fd, int status){
	STOP = FALSE;
	layer.machineState = START;
	unsigned char set[5];
	int i = 0;
	read(fd,set+i,1);

	if(status == SET_STATUS) updateStateReceiveSET(set[i]);
	else if(status == UA_STATUS) updateStateReceiveUA(set[i]);
	else if(status == DISC_STATUS) updateStateReceiveDISC(set[i]);
	else if(status == RR_STATUS) updateStateReceiveRR(set[i]);

	i++;

	if(set[i-1] == FLAG){
		while (STOP==FALSE){
			read(fd,set+i,1);
			if(status == SET_STATUS) updateStateReceiveSET(set[i]);
			else if(status == UA_STATUS) updateStateReceiveUA(set[i]);
			else if(status == DISC_STATUS) updateStateReceiveDISC(set[i]);
			else if(status == RR_STATUS) updateStateReceiveRR(set[i]);

			i++;
			if(set[i-1] == FLAG){
				STOP = TRUE;
			}
		}
	}

	return layer.machineState;
}

void updateStateReceiveRR(unsigned char state){
	switch(state){
	case FLAG:
		if(layer.machineState == START){
			layer.machineState = FLAG;
		}else if(layer.machineState == (A ^ C_RR)){
			layer.machineState = STP;
		}else{
			layer.machineState = START;
		}
		break;
	case A:
		if(layer.machineState == FLAG){
			layer.machineState = A;
		}else{
			layer.machineState = START;
		}
		break;
	case C_RR:
		if(layer.machineState == A){
			layer.machineState = C_RR;
		}else{
			layer.machineState = START;
		}
		break;
	case (A ^ C_RR):
		if(layer.machineState == C_RR){
			layer.machineState = A ^ C_RR;
		}else{
			layer.machineState = START;
		}
		break;
	default:
		layer.machineState = START;
		break;
	}
}

void updateStateReceiveUA(unsigned char state){
	switch(state){
	case FLAG:
		if(layer.machineState == START){
			layer.machineState = FLAG;
		}else if(layer.machineState == (A ^ C_UA)){
			layer.machineState = STP;
		}else{
			layer.machineState = START;
		}
		break;
	case A:
		if(layer.machineState == FLAG){
			layer.machineState = A;
		}else{
			layer.machineState = START;
		}
		break;
	case C_UA:
		if(layer.machineState == A){
			layer.machineState = C_UA;
		}else{
			layer.machineState = START;
		}
		break;
	case (A ^ C_UA):
		if(layer.machineState == C_UA){
			layer.machineState = A ^ C_UA;
		}else{
			layer.machineState = START;
		}
		break;
	default:
		layer.machineState = START;
		break;
	}
}

void updateStateReceiveSET(unsigned char state){
	switch(state){
	case FLAG:
		if(layer.machineState == START){
			layer.machineState = FLAG;
		}else if(layer.machineState == (A ^ C_SET)){
			layer.machineState = STP;
		}else{
			layer.machineState = START;
		}
		break;
	case C_SET:
		if(layer.machineState == FLAG){
			layer.machineState = A;
		}else if(layer.machineState == A){
			layer.machineState = C_SET;
		}else{
			layer.machineState = START;
		}
		break;
	case (A ^ C_SET):
		if(layer.machineState == C_SET){
			layer.machineState = A ^ C_SET;
		}else{
			layer.machineState = START;
		}
		break;
	default:
		layer.machineState = START;
		break;
	}
}

void updateStateReceiveDISC(unsigned char state){
	switch(state){
	case FLAG:
		if(layer.machineState == START){
			layer.machineState = FLAG;
		}else if(layer.machineState == (A ^ C_DISC)){
			layer.machineState = STP;
		}else{
			layer.machineState = START;
		}
		break;
	case A:
		if(layer.machineState == FLAG){
			layer.machineState = A;
		}else{
			layer.machineState = START;
		}
		break;
	case C_DISC:
		if(layer.machineState == A){
			layer.machineState = C_DISC;
		}else{
			layer.machineState = START;
		}
		break;
	case (A ^ C_DISC):
		if(layer.machineState == C_DISC){
			layer.machineState = A ^ C_DISC;
		}else{
			layer.machineState = START;
		}
		break;
	default:
		layer.machineState = START;
		break;
	}
}
