#include "url.h"

void getUrlInfo(char * completeUrl, urlData * url){
    //verifica o inicio do url
    if(strncmp(completeUrl, "ftp://", 6)){
        printf("Wrong url: expected ftp://...\n");
        exit(1);
    }

    char * mode = strchr(completeUrl, '@');
    if(mode == NULL){
        printf("Anonymous mode\n");
    }else{
        printf("Standard mode\n");
    }

    int userPassLength, userLength, passLength, hostLength, urlPathLength;
    char * colon = strchr(completeUrl + 6, ':');
    char * slash = strchr(completeUrl + 7, '/');

    if(slash == NULL){
        printf("Wrong url\n");
        exit(1);
    }

    if(mode == NULL){
        userPassLength = 0;
        passLength = 0;
        userLength = 0;
        hostLength = (int) (slash - completeUrl - 6);
        urlPathLength = strlen(completeUrl) - (7 + userLength + passLength + hostLength);
    }else{
        userPassLength = (int) (mode - completeUrl - 6);
        userLength = (int) (colon - completeUrl -6);
        passLength = (int) (userPassLength - userLength - 1);
        hostLength = (int) (slash - mode - 1);
        urlPathLength = strlen(completeUrl) - (9 + userLength + passLength + hostLength);
    }

    if(hostLength <= 0 || urlPathLength <= 0){
        printf("Wrong url: host and Path length can't be 0\n");
        exit(1);
    }

    
    url->urlPath = malloc(sizeof(char) * urlPathLength);

    char hostTemp[100];

    if(mode == NULL){
        url->user = malloc(sizeof(char) * strlen("anonymous"));
        strncpy(url->user, "anonymous", strlen("anonymous"));
        url->password = malloc(sizeof(char) * strlen("bill9gates@"));
        strncpy(url->password, "bill9gates@", strlen("bill9gates@"));
        strncpy(hostTemp, completeUrl + 6, hostLength);
    }else{
        url->user = malloc(sizeof(char) * userLength);
        strncpy(url->user, completeUrl + 6, userLength);
        url->password = malloc(sizeof(char) * passLength);
        strncpy(url->password, completeUrl + userLength + 7, passLength);
        strncpy(hostTemp, mode + 1, hostLength);
    }
    strncpy(url->urlPath, slash + 1, urlPathLength);
    hostTemp[hostLength] = '\0';

    if((url->h = gethostbyname(hostTemp)) == NULL){
        herror("gethostbyname");
        exit(1);
    }

    int l = strlen(inet_ntoa(*((struct in_addr *)url->h->h_addr)));
    url->hostIp = calloc(l, sizeof(char) * l);
    strncpy(url->hostIp, inet_ntoa(*((struct in_addr *)url->h->h_addr)), l);
}