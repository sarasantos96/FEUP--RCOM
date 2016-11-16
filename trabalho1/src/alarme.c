#include "alarme.h"

int alarmeDispara = 0;
int tries;
int tentativas=0;
int time;

void handler(int signal){
  if(signal != SIGALRM) return;

  alarmeDispara = 1;
  tentativas++;
  printf("%d transmissao falhada. Retransmitir\n", tentativas);

  alarm(time);
}

void setAlarm(int nTries, int timeout){
  tries = nTries;
  time = timeout;
  struct sigaction a;
  tentativas = 0;
  a.sa_handler = handler;
  sigemptyset(&a.sa_mask);
  a.sa_flags = 0;

  sigaction(SIGALRM, &a, NULL);

  alarmeDispara = 0;

  alarm(time);
}

void closeAlarm(){
  struct sigaction a;
  a.sa_handler = NULL;
  sigemptyset(&a.sa_mask);
  a.sa_flags = 0;

  sigaction(SIGALRM, &a, NULL);

  alarm(0);
}

int getNumTries(){
  return tentativas;
}

int getAlarmeDispara(){
  return alarmeDispara;
}

void setAlarmeDispara(int value){
  alarmeDispara = value;
}
