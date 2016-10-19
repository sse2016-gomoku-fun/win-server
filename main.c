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
 * 工具类
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
	
	//判断横着的
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
	
	//计算竖着的
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
	
	//计算左斜着的
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
	
	//计算右斜着的
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
	
	//判断落子是否合法 
	if (board[row][col] != 0)
	{
		retry(me);
		return;
	}
	
	//落子 
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
	
	//转发
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "TURN %d %d\n", row, col);
    sendTo(other, buffer);
    
    //判断输赢
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
	
	//发送
	sendTo(other, "READY\n");
	
	turn = otherFlag;
}

void startSock()
{
    //初始化 DLL
    WSADATA wsaData;
    WSAStartup( MAKEWORD(2, 2), &wsaData);
}

void initMap()
{
	// 初始化棋局 
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
	}
}

void initSock(int port)
{
    //创建套接字 
    servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    //绑定套接字 
    struct sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));  //每个字节都使用0填充 
    sockAddr.sin_family = PF_INET;  //使用ipv4地址 
    sockAddr.sin_addr.s_addr = inet_addr("0.0.0.0");  //绑定IP地址 
    sockAddr.sin_port = htons(port);  //绑定端口 
    bind(servSock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR));
    
    //进入监听状态 
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
    
    //接收黑棋请求 
    SOCKADDR blackAddr;
    nSize = sizeof(SOCKADDR);
    blackSock = accept(servSock, (SOCKADDR*)&blackAddr, &nSize);
    printf("Client Connected\n");
    
    //接收白棋请求 
    SOCKADDR whiteAddr;
    nSize = sizeof(SOCKADDR);
    whiteSock = accept(servSock, (SOCKADDR*)&whiteAddr, &nSize);
    printf("Client Connected\n");
    
    //初始化黑棋
    sendTo(&blackSock, "START\n");
	
	//初始化白棋 
	sendTo(&whiteSock, "START\n");
	
	if (NULL != globalArgs.mapFile) initMap();
    
    //执黑先行 
	sendTo(&blackSock, "READY\n");
}

void closeSock()
{
    //关闭套接字 
    closesocket(blackSock);
    closesocket(whiteSock);
    closesocket(servSock);
    
    //终止 DLL 的使用 
    WSACleanup();
}

void loop()
{   
    while (TRUE)
    {
    	// 先接收下在哪里 
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
		exit(0);
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
