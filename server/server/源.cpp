#include <stdio.h>
#include <time.h>
#include <WinSock2.h>

#pragma comment(lib,"Ws2_32.lib")
#pragma pack(1)

long TcpClientCount = 0;
#define MAX_CLIENT 10
#define MAX_BUF_SIZE 65535

const u_short UDPSrvPort = 2345;
const char START_CMD[] = "START";
const char GETCURTIME_CMD[] = "GET CUR TIME";

struct TcpThreadParam
{
	SOCKET socket;
	sockaddr_in addr;
};

DWORD WINAPI TcpServeThread(LPVOID lpParam);
DWORD WINAPI UdpServer(LPVOID lpParam);
int main(int argc, char* argv[]) {

	//�����Ĳ����Ƿ�������ȷ
	if (argc != 2) {
		printf("Wrong format!\nCorrect usage : Server.exe < TCP server port>");
		return -1;
	}

	//��ʼ��winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Failed to initialize the winsock 2 stack nerror code:%d\r\n", WSAGetLastError());
		return -1;
	}

	DWORD dwThreadId;
	//����udp�߳�
	CreateThread(NULL, 0, UdpServer, NULL, 0, &dwThreadId);
	//���������׽���
	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	u_short ListenPort = (u_short)atoi(argv[1]);

	//ͨ������������ȡip��ַ
	char hostname[256];
	gethostname(hostname, sizeof(hostent));
	hostent* pHostent = gethostbyname(hostname);
	SOCKADDR_IN ListenAddr;
	memset(&ListenAddr, 0, sizeof(SOCKADDR_IN));
	ListenAddr.sin_family = AF_INET;
	ListenAddr.sin_port = htons(ListenPort);
	ListenAddr.sin_addr = *(in_addr*)pHostent->h_addr_list[1];

	//���׽���
	if (bind(ListenSocket, (sockaddr*)&ListenAddr, sizeof(ListenAddr)) == SOCKET_ERROR) {
		printf("Failed to bind the ListenSocket(rnerror code: %d", WSAGetLastError());
		return -1;
	}

	//�����׽���
	if ((listen(ListenSocket, SOMAXCONN)) == SOCKET_ERROR) {
		printf("Failed to listen the ListenSocket \r\n error code: %d", WSAGetLastError());
		return -1;
	}
	printf("TCP Server Started On TCP Port:%d\r\n", ListenPort);

	SOCKET TcpSocket;
	SOCKADDR_IN TcpClientAddr;
	while (TRUE) {
		int iSockAddrLen = sizeof(sockaddr);
		//������ͻ��˵�����
		if ((TcpSocket = accept(ListenSocket, (sockaddr*)&TcpClientAddr, &iSockAddrLen)) == SOCKET_ERROR) {
			printf("Failed to accept the client TCP Socket \r\nerror code: %d", WSAGetLastError());
			return -1;
		}

		//������ӵĿͻ��˸����Ƿ�ﵽ����
		if (TcpClientCount >= MAX_CLIENT) {
			closesocket(TcpSocket);
			printf("Connection from TCp client %s:%d refused for max client numirin",
				inet_ntoa(TcpClientAddr.sin_addr), ntohs(TcpClientAddr.sin_port));
			continue;
		}

		printf("Connection from TCP client %s:%d accepted \r\n", inet_ntoa(TcpClientAddr.sin_addr), ntohs(TcpClientAddr.sin_port));

		TcpThreadParam Param;
		Param.socket = TcpSocket;
		Param.addr = TcpClientAddr;

		DWORD dwThreadId;

		//����tcp�������߳�
		::CreateThread(NULL,0,TcpServeThread,&Param,0,&dwThreadId);
		InterlockedIncrement(&TcpClientCount);

		//��ӡ��ǰ���ӵ�tcp�ͻ��˸���
		printf("Current Number of TCP Clients: %d\r\n",TcpClientCount);
	}
	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}

DWORD WINAPI TcpServeThread(LPVOID lpParam)
{
	char ServerTCPBuf[MAX_BUF_SIZE];
	SOCKET TcpSocket = ((TcpThreadParam*)lpParam)->socket;
	SOCKADDR_IN TcpClientAddr = ((TcpThreadParam*)lpParam)->addr;

	printf("Thread: %d is serving client from %s:%d\r\n", GetCurrentThreadId(), inet_ntoa(TcpClientAddr.sin_addr), ntohs(TcpClientAddr.sin_port));


	//���Ϳ�ʼ����������ͻ��ˣ��ȴ��ͻ��˲���
	sprintf(ServerTCPBuf, "%5d%s", UDPSrvPort, START_CMD);
	send(TcpSocket, ServerTCPBuf, strlen(ServerTCPBuf),0);
	printf("Waiting for command from Client(s)... \r\n");

	int TCPBytesReceived;
	time_t CurSysTime;
	while (TRUE) {
		memset(ServerTCPBuf, '\0', sizeof(ServerTCPBuf));

		//���տͻ��˻�ȡ��ǰʱ�������
		TCPBytesReceived = recv(TcpSocket, ServerTCPBuf, sizeof(ServerTCPBuf),0);

		if (TCPBytesReceived == 0 || TCPBytesReceived == SOCKET_ERROR) {
			printf("Client from %s:%d disconnected. Thread: %d is ending.\r\n", inet_ntoa(TcpClientAddr.sin_addr), ntohs(TcpClientAddr.sin_port), GetCurrentThreadId());
			break;
		}

		if (strcmp(ServerTCPBuf, GETCURTIME_CMD) != 0)
		{
			printf("Unknowm command from Client %s \r\n", inet_ntoa(TcpClientAddr.sin_addr));
			continue;
		}
		printf("Request for Current time from client %s:%d by TCP\r\n",inet_ntoa(TcpClientAddr.sin_addr), ntohs(TcpClientAddr.sin_port));
		//��ȡ��ǰʱ�䲢���͸��ͻ���
		time(&CurSysTime);
		memset(ServerTCPBuf, '\0', sizeof(ServerTCPBuf));
		strftime(ServerTCPBuf, sizeof(ServerTCPBuf), "%Y-%m-%d %H:%M:%S", localtime(&CurSysTime));
		send(TcpSocket, ServerTCPBuf, strlen(ServerTCPBuf),0);
		printf("Server Current Time: %s", ServerTCPBuf);

	}
	//�����������ر��׽���
	InterlockedDecrement(&TcpClientCount);
	closesocket(TcpSocket);
	return 0;

}


DWORD WINAPI UdpServer(LPVOID lpParam) {
	char ServerUDPBuf[MAX_BUF_SIZE];
	SOCKADDR_IN UDPClientAddr;

	SOCKET UDPSrvSocket = socket(AF_INET, SOCK_DGRAM, 0);
	//��ȡ������ip��ַ
	char hostname[256];
	gethostname(hostname, sizeof(hostname));

	hostent* pHostent = gethostbyname(hostname);

	SOCKADDR_IN UDPSrvAddr;
	memset(&UDPSrvAddr, 0, sizeof(SOCKADDR_IN));
	UDPSrvAddr.sin_family = AF_INET;
	UDPSrvAddr.sin_port = htons(UDPSrvPort);
	UDPSrvAddr.sin_addr = *(in_addr*)pHostent->h_addr_list[1];
	
	if (bind(UDPSrvSocket, (sockaddr*)&UDPSrvAddr, sizeof(UDPSrvAddr)) == SOCKET_ERROR) {
		printf("bind() failed for UDPSrvSocket\r\nerror code: %d\r\n", WSAGetLastError());
		return -1;
	}

	printf("UDP Server Started On UDP Port: %d \r\n", UDPSrvPort);

	while (TRUE) {
		memset(ServerUDPBuf, '\0', sizeof(ServerUDPBuf));

		//��ȡ���Կͻ��˷��͵�����
		int iSockAddrLen = sizeof(sockaddr);
		if ((recvfrom(UDPSrvSocket, ServerUDPBuf,sizeof(ServerUDPBuf), 0,(sockaddr*)&UDPClientAddr, &iSockAddrLen)) == SOCKET_ERROR) {

			printf("recvfrom() failed for UDPSrvSocket\r\nerror code: %d\r\n", WSAGetLastError());
			continue;

		}
		//���տͻ��˷��͵�����
		printf("\nClient Command: Echo\n\n");
		printf("\"%s\"received from %s:%d by UDP.\r\n", ServerUDPBuf, inet_ntoa(UDPClientAddr.sin_addr), ntohs(UDPClientAddr.sin_port));
		iSockAddrLen = sizeof(sockaddr);
		//���Ըո��յ������ݸ��ͻ���
		if ((sendto(UDPSrvSocket, ServerUDPBuf, strlen(ServerUDPBuf), 0, (sockaddr*)&UDPClientAddr, iSockAddrLen)) == SOCKET_ERROR) {

			printf("sendto() failed for UDPSrvSocket \r\nerror code: %d\r\n", WSAGetLastError());
			continue;

		}

		printf("Echo  \"%s \" to client %s:%d by UDP. \r\n",ServerUDPBuf, inet_ntoa(UDPClientAddr.sin_addr), ntohs(UDPClientAddr.sin_port));
	}

	return 0;

}
