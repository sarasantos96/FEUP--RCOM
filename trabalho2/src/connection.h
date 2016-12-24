#include "url.h"

typedef struct {
    int fdSocket;
    int fdDataSocket;
    int passiveAnswer[6];
    int port;
    char ip[256];
} FTP;

int startConnection(urlData * url, FTP * ftp);
int getControl(FTP * ftp, urlData * url, FTP * receiverFtp);
int receivePassiveAnswer(FTP * ftp);
int sendAndReceiveControl(int cmd, FTP * ftp, FTP * ftpReceiver, urlData * url);
int startReceiverCon(urlData * urlReceiver, FTP * ftpReceiver);
int receiveFile(urlData * url, FTP * ftp, FTP * ftpReceiver);
int closeConnection(FTP * ftp, FTP * ftpReceiver);
