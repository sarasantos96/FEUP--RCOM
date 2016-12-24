#include "connection.h"

int startConnection(urlData * url, FTP * ftp){
    int	sockfd;
	struct	sockaddr_in server_addr;

	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(url->hostIp);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("socket()");
        return -1;
    }
	/*connect to the server*/
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("connect()");
		return -1;
	}

    ftp->fdSocket = sockfd;

    return 0;
}

int getControl(FTP * ftp, urlData * url, FTP * receiverFtp){
    char buffer[1000];
    read(ftp->fdSocket, buffer, 1000);

    if(sendAndReceiveControl(USER, ftp, receiverFtp, url) != 0)
        return -1;

    if(sendAndReceiveControl(PASS, ftp, receiverFtp, url) != 0)
        return -1;

    if(sendAndReceiveControl(PASSIVE, ftp, receiverFtp, url) != 0)
        return -1;

    return 0;
}

int sendAndReceiveControl(int cmd, FTP * ftp, FTP * ftpReceiver, urlData * url){
    char command[256];

    switch(cmd){
        case USER:
            strcpy(command, "user \0");
            strcat(command, url->user);
            break;
        case PASS:
            strcpy(command, "pass \0");
            strcat(command, url->password);
            break;
        case PASSIVE:
            strcpy(command, "pasv \0");
            break;
        default:
            break;
    }

    
    strcat(command, "\n");
    if(write(ftp->fdSocket, command, strlen(command)) < 0){
        printf("Error writing to socket\n");
        return -1;
    }
    sleep(1);

    if(cmd == PASSIVE){
        if(receivePassiveAnswer(ftp) == 0){
            ftpReceiver->port = ftp->passiveAnswer[4] * 256 + ftp->passiveAnswer[5];
            memset(ftpReceiver->ip, 0, 256);
            sprintf(ftpReceiver->ip, "%d.%d.%d.%d", ftp->passiveAnswer[0],
                ftp->passiveAnswer[1], ftp->passiveAnswer[2], ftp->passiveAnswer[3]);
            
        }
    }else{
        char answer[256] = "";
        if(read(ftp->fdSocket, answer, 256) <= 0){
            printf("Error reading from socket\n");
            return -1;
        }
    }
    return 0;
}

int receivePassiveAnswer(FTP * ftp){
    char passiveAnswer[256];
    if(read(ftp->fdSocket, passiveAnswer, 256) <= 0){
        printf("Error reading from socket\n");
          return -1;
    }else{
        int r = sscanf(passiveAnswer, "%*[^(](%d,%d,%d,%d,%d,%d)\n", &(ftp->passiveAnswer[0]),
                &(ftp->passiveAnswer[1]), &(ftp->passiveAnswer[2]), &(ftp->passiveAnswer[3]),
                &(ftp->passiveAnswer[4]), &(ftp->passiveAnswer[5]));
       if(r != 6){
            printf("Error reading answer\n");
              return -1;
        }
    }
    return 0;
}

int startReceiverCon(urlData * urlReceiver, FTP * ftpReceiver){
  int	sockfd;
  struct	sockaddr_in server_addr;
  char * host_ip;

  urlReceiver->h = gethostbyname(ftpReceiver->ip);

  if(urlReceiver->h == NULL){
      printf("Could not find host\n");
      return -1;
  }

  host_ip = inet_ntoa(*((struct in_addr *)urlReceiver->h->h_addr));

  /*server address handling*/
  bzero((char*)&server_addr,sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(host_ip);	/*32 bit Internet address network byte ordered*/
  server_addr.sin_port = htons(ftpReceiver->port);		/*server TCP port must be network byte ordered */

  /*open an TCP socket*/
  sockfd = socket(AF_INET,SOCK_STREAM,0);
  if (sockfd < 0) {
        perror("socket()");
        return -1;
      }
  /*connect to the server*/
      if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
          perror("connect()");
          return -1;
  }
  ftpReceiver->fdSocket = sockfd;
  ftpReceiver->fdDataSocket = sockfd;

  return 0;
}

void getName(char * url, char ** filename){
    char * temp = url;
    int i = 0;

    while(temp != NULL){
        if(i > 0)
            *filename = temp + 1;
        else
            *filename = temp;
        temp = strchr(*filename, '/');
        i++;
    }
}

int receiveFile(urlData * url, FTP * ftp, FTP * ftpReceiver){
        write(ftp->fdSocket, "TYPE L 8\r\n", strlen("TYPE L 8\r\n"));

    char command[256] = "";
    strcpy(command, "retr ");
    strcat(command, url->urlPath);
    strcat(command, "\n");

    if(write(ftp->fdSocket, command, strlen(command)) < 0){
      printf("Error sending command\n");
      return -1;
    }

    char * filename;
    FILE* file;
    int res;
    char temp[256];

    getName(url->urlPath, &filename);
    if(!(file = fopen(filename, "wb"))){
        printf("Error opening file\n");
        return -1;
    }

    sprintf(temp,"%d",ftpReceiver->fdDataSocket);

    char buf[1024];
    while((res = read(ftpReceiver->fdDataSocket, buf, sizeof(buf)))){
        if(res < 0){
            printf("Error reading\n");
            return -1;
        }
        if((res = fwrite(buf, res, 1, file)) < 0){
            printf("Error writing to file\n");
            return -1;
        }
    }
    fclose(file);
    return 0;
}

int closeConnection(FTP * ftp, FTP * ftpReceiver){
    int res;
    res = close(ftpReceiver->fdSocket);
    if(res < 0){
        printf("Error closing FTP receiver\n");
        return -1;
    }

    res = close(ftp->fdSocket);
    if(res < 0){
        printf("Error closing FTP\n");
        return -1;
    }
    return 0;
}
