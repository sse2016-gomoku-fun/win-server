#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

#define BOARD_SIZE 20
#define BLACK      1
#define WHITE      2
#define WIN_FLAG   5

struct globalArgs_t {
    int port;
    char *mapFile;
} globalArgs;

static const char *optString = "p:m:h";

char board[BOARD_SIZE][BOARD_SIZE] = {0};
char buffer[MAXBYTE] = {0};
int turn = BLACK;
int row, col;

SOCKET servSock, blackSock, whiteSock;

/*
 * Ip Utils
 */
 
BOOL isPort(const int port)
{
    return (port >= 0 && port <= 65535);
}


char *getIp()
{
    PHOSTENT hostinfo;
    char name[255];
    char* ip;
    if(gethostname(name, sizeof(name)) == 0)
    {
        if((hostinfo = gethostbyname(name)) != NULL)
        {
            ip = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
            return ip;
        }
    }
    return NULL;
} 

void sendTo(SOCKET *sock, const char *message)
{
    send(*sock, message, strlen(message)+sizeof(char), NULL);
    Sleep(100);
}

void retry(SOCKET *sock)
{
    sendTo(sock, "READY\n");
}

BOOL isWin(int x, int y)
{
    int count;
    int i, j;
    
    // Judge row
    count = 1;
    i = x - 1;
    while (i >= 0 && board[x][y] == board[i][y])
    {
        ++count;
        --i;
    }
    
    i = x + 1;
    while (i < BOARD_SIZE && board[x][y] == board[i][y])
    {
        ++count;
        ++i;
    }
    
    if (count >= WIN_FLAG) return TRUE;
    
    // Judge col
    count = 1;
    j = y - 1;
    while (j >= 0 && board[x][y] == board[x][j])
    {
        ++count;
        --j;
    }
    
    j = y + 1;
    while (j < BOARD_SIZE && board[x][y] == board[x][j])
    {
        ++count;
        ++j;
    }
    
    if (count >= WIN_FLAG) return TRUE;
    
    // Judge left oblique
    count = 1;
    i = x - 1;
    j = y - 1;
    while (i >= 0 && j >= 0 && board[x][y] == board[i][j])
    {
        ++count;
        --i;
        --j;
    }
    
    i = x + 1;
    j = y + 1;
    while (i < BOARD_SIZE && j < BOARD_SIZE && board[x][y] == board[i][j])
    {
        ++count;
        ++i;
        ++j;
    }
    
    if (count >= WIN_FLAG) return TRUE;
    
    // Judge right oblique
    count = 1;
    i = x - 1;
    j = y + 1;
    while (i >= 0 && j < BOARD_SIZE && board[x][y] == board[i][j])
    {
        ++count;
        --i;
        ++j;
    }
    
    i = x + 1;
    j = y - 1;
    while (i < BOARD_SIZE && j >= 0 && board[x][y] == board[i][j])
    {
        ++count;
        ++i;
        --j;
    }
    
    if (count >= WIN_FLAG) return TRUE;
    
    return FALSE;
}

void handle(SOCKET *me, int meFlag, SOCKET *other, int otherFlag)
{
    memset(buffer, 0, sizeof(buffer));
    recv(*me, buffer, MAXBYTE, NULL);
    sscanf(buffer, "%d %d\n", &row, &col);
    
    // Judge if legal
    if (board[row][col] != 0)
    {
        retry(me);
        return;
    }
    
    board[row][col] = meFlag;
    
    switch (meFlag)
    {
        case BLACK:
            printf("BLACK step at (%d, %d)\n", row, col);
            break;
        case WHITE:
            printf("WHITE step at (%d, %d)\n", row, col);
            break;
    }
    
    // Forward
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "TURN %d %d\n", row, col);
    sendTo(other, buffer);
    
    // Judge result
    if (isWin(row, col))
    {
        sendTo(me, "WIN\n");
        sendTo(other, "LOSE\n");
        if (BLACK == meFlag)
        {
            printf("BLACK win!\n");
        }
        else
        {
            printf("WHITE win!\n");
        }
        return;
    }
    
    sendTo(other, "READY\n");
    
    turn = otherFlag;
}

void startSock()
{
    // Initial DLL
    WSADATA wsaData;
    WSAStartup( MAKEWORD(2, 2), &wsaData);
}

void initMap()
{
    FILE *fp;
    if ((fp = fopen(globalArgs.mapFile, "r")) == NULL)
    {
        printf("Map file [%s] does not exist!\n", globalArgs.mapFile);
        exit(1);
    }
    int x, y;
    while (fscanf(fp, "%d%d\n", &x, &y) != EOF)
    {
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "PLACE %d %d\n", x, y);
        sendTo(&blackSock, buffer);
        sendTo(&whiteSock, buffer);
        
        if (BLACK == turn) turn = WHITE;
        else turn = BLACK;
    }
}

void initSock(int port)
{
    // Open socket
    servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    // Bind socket
    struct sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin_family = PF_INET;  // Use ipv4 address 
    sockAddr.sin_addr.s_addr = inet_addr("0.0.0.0");  // Bind to LAN
    sockAddr.sin_port = htons(port);  // Bind port
    bind(servSock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR));
    
    // Listen
    listen(servSock, 20);
    
    char *ip = getIp();
    if (NULL != ip)
    {
        printf("Listening on %s:%d\n", ip, port);
    }
    else
    {
        printf("Listening...\n");
    }
    
    int nSize;
    
    // Connect to BLACK
    SOCKADDR blackAddr;
    nSize = sizeof(SOCKADDR);
    blackSock = accept(servSock, (SOCKADDR*)&blackAddr, &nSize);
    printf("Client Connected\n");
    
    // Connect to WHITE
    SOCKADDR whiteAddr;
    nSize = sizeof(SOCKADDR);
    whiteSock = accept(servSock, (SOCKADDR*)&whiteAddr, &nSize);
    printf("Client Connected\n");
    
    // Initial BLACK
    sendTo(&blackSock, "START\n");
    
    // Initial WHITE
    sendTo(&whiteSock, "START\n");
    
    if (NULL != globalArgs.mapFile) initMap();
    
    if (BLACK == turn) sendTo(&blackSock, "READY\n");
    else sendTo(&whiteSock, "READY\n");
}

void closeSock()
{
    // Close socket
    closesocket(blackSock);
    closesocket(whiteSock);
    closesocket(servSock);
    
    // Close DLL
    WSACleanup();
}

void loop()
{   
    while (TRUE)
    {
        switch (turn)
        {
            case BLACK:
                handle(&blackSock, BLACK, &whiteSock, WHITE);
                break;
            case WHITE:
                handle(&whiteSock, WHITE, &blackSock, BLACK);
                break;
        }
    }
}

void display_usage(char *exe)
{
    printf("Usage: %s [OPTIONS] \n", exe);
    printf("  -p port           Server port\n");
}

void initArgs(int argc, char *argv[])
{
    int opt = 0;
    globalArgs.port = 23333;
    globalArgs.mapFile = NULL;
    
    opt = getopt(argc, argv, optString);
    while (opt != -1)
    {
        switch (opt)
        {
            case 'p':
                globalArgs.port = atoi(optarg);
                break;
            case 'm':
                globalArgs.mapFile = optarg;
                break;
            case 'h':
                display_usage(argv[0]);
                exit(0);
                break;
            default:
                // Illegal!
                break;
        }
        
        opt = getopt(argc, argv, optString);
    }
    
    if (NULL != globalArgs.mapFile && access(globalArgs.mapFile, F_OK ) == -1)
    {
        printf("Map file is invalid! The game will start without map.\n");
        globalArgs.mapFile = NULL;
    }
}

int main(int argc, char *argv[]){
    
    startSock();
    
    initArgs(argc, argv);
    
    initSock(globalArgs.port);
    
    loop();
    closeSock();
    
    return 0;
}
