#ifndef ALARME_H
#define ALARME_H

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

extern int alarmeDispara;

void handler(int signal);
void setAlarm(int nTries, int timeout);
void closeAlarm();
int getNumTries();
int getAlarmeDispara();
void setAlarmeDispara(int value);

#endif
