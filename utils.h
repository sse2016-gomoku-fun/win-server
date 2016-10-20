#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef UTILS
#define UTILS

#define TRUE 1
#define FALSE 0
#define SOCKET_MAX_BYTE 10000

extern char *socketArg;
extern char socketBuffer[SOCKET_MAX_BYTE];

void initSocketBuffer();
void addToSocketBuffer(const char *buffer);
int hasCommand(char divider);


#endif