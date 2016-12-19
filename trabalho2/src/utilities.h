#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct{
    char * user;
    char * password;
    struct hostent * h;
    char * urlPath;
    char * hostIp;
}urlData;

#define SERVER_PORT 21

#define USER 0
#define PASS 1
#define PASSIVE 2