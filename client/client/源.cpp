#include <WinSock2.h>
#include <stdio.h>
#pragma comment(lib,"Ws2_32.lib")

#define MAX_BUF_SIZE 65535
char ClientBuf[MAX_BUF_SIZE];
const char START_CMD[] = "START";
const char GETCURTIME_CMD[] = "GET CUR TIME";


//用户操作界面显示打印
void UserPrompt() {
	printf("Input the corresponding Num to select what you want the program to do\r\n\t1. Get current time(TCP)\r\n\t2. Echo Mode(UDP)\r\n\t3. Exit the program\r\n\Enter Your choice: \r\n");

}

int main(int argc, char* argv[]) {
	unsigned short ServerUDPPort;

	SOCKET cTCPSocket, cUDPSocket;
	WSADATA wsadata;
	SOCKADDR_IN TCPServer, UDPServer, RecvFrom;
	int RecvFromLength = sizeof(RecvFrom);

	char UserChoice[5];
	char portnum[5];
	unsigned long BytesReceived, BytesSent;
	int RetValue;


	//检查带参执行是否正确
	if (argc != 3) {
		printf("Worng format!\r\nCorrect usage: Client.exe <TCP Server IP> <TCP Server Port>");
		return 1;
	}

	//将参数转换为ip和tcp端口号
	u_long ServerIP = inet_addr(argv[1]);
	u_short ServerTCPPort = (u_short)atoi(argv[2]);

	//初始化winsock
	if ((RetValue = WSAStartup(MAKEWORD(2, 2), &wsadata)) != 0) {
		printf("WSAStartup() failed with error %d\n",RetValue);
		return 2;
	}
	
	//初始化tcpsocket流式套接字
	if ((cTCPSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		printf("WSASocket() for cTCPSocket failed with error %d\n", WSAGetLastError());
		return 3;
	}
	//初始化udpsocket数据报套接字
	if ((cUDPSocket = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		printf("WSASocket() for cUDPSocket failed with error %d\n", WSAGetLastError());
		return 4;
	}

	TCPServer.sin_family = AF_INET;
	TCPServer.sin_port = htons(ServerTCPPort);
	TCPServer.sin_addr.S_un.S_addr = ServerIP;

	//WSAConnect 函数建立与服务器套接字应用程序的连接
	if ((RetValue = WSAConnect(cTCPSocket, (sockaddr*)&TCPServer, sizeof(TCPServer), NULL, NULL, NULL, NULL)) == SOCKET_ERROR)
	{
		printf("WSAConnect() failed for cTCPSocket with error %d\n", WSAGetLastError());
		printf("Can't connect to server.\n");
		return 5;
	}

	//接收来自服务器的连接
	BytesReceived = recv(cTCPSocket, ClientBuf, sizeof(ClientBuf), 0);
	if (BytesReceived == 0 || BytesReceived == SOCKET_ERROR) {
		printf("Server refused the connection or recv failed\r\n");
		return 6;
	}
	memcpy(portnum, ClientBuf, sizeof(portnum));
	ServerUDPPort = (u_short)atoi(portnum);

	//检查服务器发送的是否为开始命令
	if (strcmp(START_CMD, ClientBuf + 5) != 0) {
		printf("Server did not return right beginning indicator\r\n");
		return 6;
	}
	else {
		printf("OK, NOW the server is ready for your service!\r\n");
	}

	UDPServer.sin_family = AF_INET; 
	UDPServer.sin_port = htons(ServerUDPPort);
	UDPServer.sin_addr.S_un.S_addr = ServerIP;

	while (TRUE)
	{
		UserPrompt();
		//获取用户输入
		fgets(UserChoice, sizeof(UserChoice), stdin);
		switch (UserChoice[0])
		{
		case'1':
			//发送当前时间命令给服务器
			memset(ClientBuf, '\0', sizeof(ClientBuf));
			sprintf(ClientBuf, "%s", GETCURTIME_CMD);
			if ((BytesSent = send(cTCPSocket, ClientBuf, strlen(ClientBuf), 0)) == SOCKET_ERROR) {
				printf("send() failed for cTCPSocket with error %d\n", WSAGetLastError());
				printf("Can not send command to server by TCP.Maybe Server is down.\n");
				return 7;
			}

			memset(ClientBuf, '\0', sizeof(ClientBuf));
			//接收来自服务器的当前时间
			if ((BytesReceived = recv(cTCPSocket, ClientBuf, sizeof(ClientBuf), 0)) == SOCKET_ERROR) {
				printf("recv() failed for cTCPSocket with error %d\n", WSAGetLastError());
				printf("Can not get server current systime.Maybe Server is down.\n");
				return 8;
			}
			printf("Server Current Time: %s\r\n", ClientBuf);
			break;
		case'2':

			//获取输入的文本信息
			memset(ClientBuf, '\0', sizeof(ClientBuf));
			printf("请输入任意文本信息，按回车键后将发送至server.\r\n");
			gets_s(ClientBuf, sizeof(ClientBuf));

			//发送文本信息给服务器
			if ((BytesSent = sendto(cUDPSocket,ClientBuf,strlen(ClientBuf),0,(sockaddr *)&UDPServer,sizeof(UDPServer))) == SOCKET_ERROR) {
				printf("sendto() failed for cUDPSocket with error %d\n", WSAGetLastError());
				printf("Can not send message by UDP.Maybe Server is down.\n");
				return 9;
			}

			//接收来自服务器的文本信息回显
			memset(ClientBuf,'\0',sizeof(ClientBuf));
			if ((BytesReceived = recvfrom(cUDPSocket, ClientBuf, sizeof(ClientBuf), 0, (sockaddr *)&RecvFrom, &RecvFromLength)) == SOCKET_ERROR) {
				printf("recvfrom () failed for cUDPSocket with error %d\n", WSAGetLastError());
				printf("Can't get Echo Reply from server by UDP.Maybe Server is down.\n");
				return 10;
			}
			if (UDPServer.sin_addr.S_un.S_addr == RecvFrom.sin_addr.S_un.S_addr && UDPServer.sin_port == RecvFrom.sin_port) {
				printf("Get Echo From Server : %s\r\n", ClientBuf);
			}
			else
			{
				printf("NO Echo From Server\r\n");
			}
			break;
			
		case'3':

			//关闭socket退出程序
			closesocket(cTCPSocket);
			closesocket(cUDPSocket);
			WSACleanup();
			printf("program exit\r\n");
			return 10;
		default:
			printf("Wrong choice,should be 1,2 or 3\r\n");
		}
	}
	return 11;
}