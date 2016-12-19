#include "connection.h"

int main(int argc, char * argv[]){
    if(argc != 2){
        printf("Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        printf("Usage: %s ftp://<host>/<url-path>\n", argv[0]);
        exit(1);
    }

    urlData * url = malloc(sizeof(urlData));
    urlData * urlReceiver = malloc(sizeof(urlData));

    getUrlInfo(argv[1], url);

    printf("User: %s\n", url->user);
    printf("Password: %s\n", url->password);
    printf("path: %s\n", url->urlPath);
    printf("HostIp: %s\n", url->hostIp);

    FTP ftp;
    FTP ftpReceiver;

    if(startConnection(url, &ftp) != 0){
        printf("Error connecting\n");
        exit(1);
    }

    if(getControl(&ftp, url, &ftpReceiver) != 0){
        printf("Error getting control\n");
        exit(1);
    }

    if(startReceiverCon(urlReceiver, &ftpReceiver) != 0){
        printf("Error starting receiving connection\n");
        exit(1);
    }

    if(receiveFile(url, &ftp, &ftpReceiver) != 0){
        printf("Error receiving file\n");
        exit(1);
    }

    if(closeConnection(&ftp, &ftpReceiver) != 0){
        printf("Error closing connection\n");
        exit(1);
    }

    free(url->user);
    free(url->password);
    free(url->urlPath);
    free(url);

    return 0;
}
