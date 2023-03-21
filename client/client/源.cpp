#include <WinSock2.h>
#include <stdio.h>
#pragma comment(lib,"Ws2_32.lib")

#define MAX_BUF_SIZE 65535
char ClientBuf[MAX_BUF_SIZE];
const char START_CMD[] = "START";
const char GETCURTIME_CMD[] = "GET CUR TIME";


//�û�����������ʾ��ӡ
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


	//������ִ���Ƿ���ȷ
	if (argc != 3) {
		printf("Worng format!\r\nCorrect usage: Client.exe <TCP Server IP> <TCP Server Port>");
		return 1;
	}

	//������ת��Ϊip��tcp�˿ں�
	u_long ServerIP = inet_addr(argv[1]);
	u_short ServerTCPPort = (u_short)atoi(argv[2]);

	//��ʼ��winsock
	if ((RetValue = WSAStartup(MAKEWORD(2, 2), &wsadata)) != 0) {
		printf("WSAStartup() failed with error %d\n",RetValue);
		return 2;
	}
	
	//��ʼ��tcpsocket��ʽ�׽���
	if ((cTCPSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		printf("WSASocket() for cTCPSocket failed with error %d\n", WSAGetLastError());
		return 3;
	}
	//��ʼ��udpsocket���ݱ��׽���
	if ((cUDPSocket = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		printf("WSASocket() for cUDPSocket failed with error %d\n", WSAGetLastError());
		return 4;
	}

	TCPServer.sin_family = AF_INET;
	TCPServer.sin_port = htons(ServerTCPPort);
	TCPServer.sin_addr.S_un.S_addr = ServerIP;

	//WSAConnect ����������������׽���Ӧ�ó��������
	if ((RetValue = WSAConnect(cTCPSocket, (sockaddr*)&TCPServer, sizeof(TCPServer), NULL, NULL, NULL, NULL)) == SOCKET_ERROR)
	{
		printf("WSAConnect() failed for cTCPSocket with error %d\n", WSAGetLastError());
		printf("Can't connect to server.\n");
		return 5;
	}

	//�������Է�����������
	BytesReceived = recv(cTCPSocket, ClientBuf, sizeof(ClientBuf), 0);
	if (BytesReceived == 0 || BytesReceived == SOCKET_ERROR) {
		printf("Server refused the connection or recv failed\r\n");
		return 6;
	}
	memcpy(portnum, ClientBuf, sizeof(portnum));
	ServerUDPPort = (u_short)atoi(portnum);

	//�����������͵��Ƿ�Ϊ��ʼ����
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
		//��ȡ�û�����
		fgets(UserChoice, sizeof(UserChoice), stdin);
		switch (UserChoice[0])
		{
		case'1':
			//���͵�ǰʱ�������������
			memset(ClientBuf, '\0', sizeof(ClientBuf));
			sprintf(ClientBuf, "%s", GETCURTIME_CMD);
			if ((BytesSent = send(cTCPSocket, ClientBuf, strlen(ClientBuf), 0)) == SOCKET_ERROR) {
				printf("send() failed for cTCPSocket with error %d\n", WSAGetLastError());
				printf("Can not send command to server by TCP.Maybe Server is down.\n");
				return 7;
			}

			memset(ClientBuf, '\0', sizeof(ClientBuf));
			//�������Է������ĵ�ǰʱ��
			if ((BytesReceived = recv(cTCPSocket, ClientBuf, sizeof(ClientBuf), 0)) == SOCKET_ERROR) {
				printf("recv() failed for cTCPSocket with error %d\n", WSAGetLastError());
				printf("Can not get server current systime.Maybe Server is down.\n");
				return 8;
			}
			printf("Server Current Time: %s\r\n", ClientBuf);
			break;
		case'2':

			//��ȡ������ı���Ϣ
			memset(ClientBuf, '\0', sizeof(ClientBuf));
			printf("�����������ı���Ϣ�����س����󽫷�����server.\r\n");
			gets_s(ClientBuf, sizeof(ClientBuf));

			//�����ı���Ϣ��������
			if ((BytesSent = sendto(cUDPSocket,ClientBuf,strlen(ClientBuf),0,(sockaddr *)&UDPServer,sizeof(UDPServer))) == SOCKET_ERROR) {
				printf("sendto() failed for cUDPSocket with error %d\n", WSAGetLastError());
				printf("Can not send message by UDP.Maybe Server is down.\n");
				return 9;
			}

			//�������Է��������ı���Ϣ����
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

			//�ر�socket�˳�����
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