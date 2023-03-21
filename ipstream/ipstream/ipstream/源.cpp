using namespace std;
#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<time.h>
#pragma warning(disable:4996)
#pragma comment(lib,"WS2_32.lib")
#define BUFFER_SIZE 65535
#define IO_RCVALL _WSAIOW(IOC_VENDOR,1) //��������Ϊ����ģʽ

//IP ͷ���
struct ipHeader
{
	unsigned char h_lenver;
	unsigned char tos;
	unsigned short total_len;
	unsigned short ident;
	unsigned short frag_and_counts;
	unsigned char timeToLive; //������
	unsigned char protocol; //Э��
	unsigned short checksum; //У���
	unsigned int sourceip; //�������� IP
	unsigned int destip; //Ŀ������ IP
	unsigned int op_pad; //ѡ������
};
//������ṹ
struct ipNode
{
	unsigned int sourceip; //�������� IP
	unsigned int destip; //Ŀ������ IP
	unsigned char protocol; //Э��
	int count; //����
	struct ipNode* next;
};
struct ipNode* pHead;
struct ipNode* ptail;
//����Э������
char* getProtocol(BYTE Protocol)
{
	switch (Protocol)
	{
	case 1:
		return "ICMP";
	case 2:
		return "IGMP";
	case 4:
		return "IP IN IP";
	case 6:
		return "TCP";
	case 8:
		return "EGP";
	case 17:
		return "UDP";
	case 41:
		return "IPV6";
	case 46:
		return "RSVP";
	case 89:
		return "OSPF";
	default:
		return "UNKNOWN";
	}
}

//�Ѳ���� IP ����������
void addNode(ipHeader pt)
{
	//�����һ����
	if (pHead == NULL)
	{
		ptail = new ipNode;
		ptail->sourceip = pt.sourceip;
		ptail->destip = pt.destip;
		ptail->protocol = pt.protocol;
		ptail->count = 1;
		pHead = ptail;
		ptail->next = NULL;
	}
	else
	{
		//�����֮ǰ�ȼ���ǲ�������ͬĿ�ĵ�ַ,Դ��ַ��Э��� IP �������;
		//�������,�����Ǹ����ļ�����һ;���������,��������ĩβ����
		ipNode* pTemp = pHead;
		while (pTemp)
		{
			if ((pTemp->sourceip == pt.sourceip) && (pTemp->destip == pt.destip)
				&& (pTemp->protocol == pt.protocol))
			{
				pTemp->count++;
				break;
			}
			pTemp = pTemp->next;
		}
		//������ĩβ�����
		if (pTemp == NULL)
		{
			pTemp = new ipNode;
			pTemp->sourceip = pt.sourceip;
			pTemp->destip = pt.destip;
			pTemp->protocol = pt.protocol;
			pTemp->count = 1;
			ptail->next = pTemp;
			ptail = ptail->next;
			ptail->next = NULL;
		}
	}
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cout << "please input command like this:IPMonitor.exe duration_time" << endl;
		return -1;
	}
	int second = atoi(argv[1]);
	//��ʼ��SOCKET���绷��,���ʧ��,�����˳�
	WSADATA wsData;
	if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0)
	{
		cout << "WSASTART failed";
		return -1;
	}
	//ɸѡ��Ҫץ��Э������
	short InternetProtocol = 0;
	cout << "��������Ҫɸѡ��Э������:" << endl;
	cout << "0.All" << endl;
	cout << "1.ICMP" << endl;
	cout << "2.IGMP" << endl;
	cout << "3.IP IN IP" << endl;
	cout << "6.TCP" << endl;
	cout << "8.EGP" << endl;
	cout << "17.UDP" << endl;
	cout << "41.IPv6" << endl;
	cout << "46.RSVP" << endl;
	cout << "89.OSPF" << endl;
	cin >> InternetProtocol;
	SOCKET sock; //����ԭʼ socket
	if ((sock = socket(AF_INET, SOCK_RAW, InternetProtocol)) == INVALID_SOCKET)
	{
		cout << "creat socket failed";//����ʧ��
		return -1;
	}
	BOOL flag = TRUE;
	unsigned long dwTemp = 1;
	ioctlsocket(sock, FIONBIO, &dwTemp);//���÷�����ģʽ
	char hostName[128];
	if (gethostname(hostName, 100) == SOCKET_ERROR)
	{
		cout << "gethostname failed";
		return -1;
	}
	//��ñ��� IP ��ַ
	hostent* pHostIP;
	if (!(pHostIP = gethostbyname(hostName)))
	{
		cout << "gethostbyname failed";
		return -1;
	}
	int i;
	int num;
	cout << "==" << second << "==" << endl;
	cout << "==���ʱ��Ϊ��" << second << "��==" << endl;
	//ö�ٵ����ϵ���������
	for (i = 0;; i++)
	{
		if (pHostIP->h_addr_list[i] != NULL) {
			in_addr ip = *(in_addr*)pHostIP->h_addr_list[i];
			cout << "==[cardSn:" << i << "]" << (int)ip.S_un.S_un_b.s_b1 << "." << (int)ip.S_un.S_un_b.s_b2 << "." << (int)ip.S_un.S_un_b.s_b3 << "." << (int)ip.S_un.S_un_b.s_b4 << "==" << endl;

		}
		else {
			break;
		}
	}
	cout << "==������������ţ�0-" << i - 1 << "):" << endl;
	cin >> num;//ɸѡ�������
	//��� SOCKADDR_IN
	sockaddr_in addr_in;
	addr_in.sin_addr = *(in_addr*)pHostIP->h_addr_list[num];
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(8000);
	//��ԭʼ socket �󶨵�����������
	if (bind(sock, (PSOCKADDR)&addr_in, sizeof(addr_in)) == SOCKET_ERROR)
	{
		cout << "bind failed";
		return -1;
	}//if

	//���� sock_raw Ϊ SIO_RCVALL,�Ա�������е� IP ��
	DWORD dwValue = 1;
	DWORD dwBufferLen[20];
	DWORD dwBufferInLen = 1;
	DWORD dwBytesReturned = 0;
	if (WSAIoctl(sock, IO_RCVALL, &dwBufferInLen, sizeof(dwBufferInLen),
		&dwBufferLen, sizeof(dwBufferLen), &dwBytesReturned, NULL, NULL) == SOCKET_ERROR)
	{
		cout << "ioctlsocker failed";
		return -1;
	}
	char buffer[BUFFER_SIZE]; //���û�����
	time_t beg;//ѭ����ӡ��ʼʱ��
	time_t end;//ѭ����ӡ����ʱ��
	time_t temp;//ÿ��ѭ����ӡ����ʱʱ��
	cout << endl;
	cout << endl;
	cout << "==����IP��" << inet_ntoa(*((struct in_addr*)pHostIP->h_addr_list[num])) << "==" << endl;
	cout << "==��ʼ����..." << endl;
	time(&beg);//��ȡbeg�Ŀ�ʼʱ��
	time(&temp);//��ʼ��temp��ǰʱ��
	int j = 0;
	//ѭ����ʼ
	while (true)
	{
		//ÿ��ѭ����ʼʱ����endʱ������»�ȡ
		time(&end);
		//����ʱ����жϣ��Ƿ񳬹����ʱ�䣬�����ͽ���ѭ��
		if (end - beg >= second)
		{
			break; //�ﵽ�趨ʱ��,�˳�
		}
		//�ж��Ƿ���յ���ip������
		int size = recv(sock, buffer, BUFFER_SIZE, 0);
		if (size > 0)
		{
			ipHeader p = *(ipHeader*)buffer;
			if ((addr_in.sin_addr).S_un.S_addr == p.sourceip || (addr_in.sin_addr).S_un.S_addr == p.destip)
			{
				addNode(p); //�Ѳ��񵽵İ����뵽����
			}
		}

		ipNode* pt = pHead;
		//�˴����ʱ��-�ϴ����ʱ���Ƿ����1��
		if ((end - temp) >= 1) {
			//����
			system("cls");
			j++;
			//���ip�������Ϣ
			cout << "==����IP��" << inet_ntoa(*((struct in_addr*)pHostIP->h_addr_list[num])) << "==" << j << endl;
			while (pt) {
				cout << "SourceIP:" << inet_ntoa(*(in_addr*)&(pt->sourceip)) << ";";
				cout << "DestIP:" << inet_ntoa(*(in_addr*)&(pt->destip)) << ";";
				cout << "Protocol:" << getProtocol(pt->protocol) << ";";
				cout << "PktSta:" << pt->count << endl;
				pt = pt->next;

			}
			//��ȡtemp���˴������ʱ��
			time(&temp);
		}
	}
	return 0;
}
