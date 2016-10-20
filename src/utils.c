#include "utils.h"

char *socketArg;
char socketBuffer[SOCKET_MAX_BYTE];

void initSocketBuffer()
{
    memset(socketBuffer, 0, sizeof(socketBuffer));
}

void addToSocketBuffer(const char *buffer)
{
    strcat(socketBuffer, buffer);
}

int hasCommand(char divider)
{
    int i, size, total;
    char *ptr;

    ptr = strchr(socketBuffer, divider);

    size = ptr - socketBuffer;
    total = strlen(socketBuffer);

    if (NULL != socketArg) free(socketArg);
    socketArg = (char *) malloc(size + 1);

    if (ptr)
    {

        memcpy(socketArg, socketBuffer, size);
        socketArg[size] = '\0';

        for (i = 0; i < total - size; i++) {
            socketBuffer[i] = socketBuffer[i + size + 1];
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
