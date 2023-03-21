using namespace std;
#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<time.h>
#pragma warning(disable:4996)
#pragma comment(lib,"WS2_32.lib")
#define BUFFER_SIZE 65535
#define IO_RCVALL _WSAIOW(IOC_VENDOR,1) //设置网卡为混杂模式

//IP 头结点
struct ipHeader
{
	unsigned char h_lenver;
	unsigned char tos;
	unsigned short total_len;
	unsigned short ident;
	unsigned short frag_and_counts;
	unsigned char timeToLive; //生命期
	unsigned char protocol; //协议
	unsigned short checksum; //校验和
	unsigned int sourceip; //本地主机 IP
	unsigned int destip; //目标主机 IP
	unsigned int op_pad; //选项和填充
};
//链表结点结构
struct ipNode
{
	unsigned int sourceip; //本地主机 IP
	unsigned int destip; //目标主机 IP
	unsigned char protocol; //协议
	int count; //计数
	struct ipNode* next;
};
struct ipNode* pHead;
struct ipNode* ptail;
//解析协议类型
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

//把捕获的 IP 包插入链表
void addNode(ipHeader pt)
{
	//插入第一个包
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
		//插入包之前先检查是不是有相同目的地址,源地址和协议的 IP 包插入过;
		//如果存在,则让那个结点的记数加一;如果不存在,则在链表末尾插入
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
		//在链表末尾插入包
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
	//初始化SOCKET网络环境,如果失败,程序退出
	WSADATA wsData;
	if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0)
	{
		cout << "WSASTART failed";
		return -1;
	}
	//筛选想要抓的协议类型
	short InternetProtocol = 0;
	cout << "请输入想要筛选的协议类型:" << endl;
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
	SOCKET sock; //建立原始 socket
	if ((sock = socket(AF_INET, SOCK_RAW, InternetProtocol)) == INVALID_SOCKET)
	{
		cout << "creat socket failed";//创建失败
		return -1;
	}
	BOOL flag = TRUE;
	unsigned long dwTemp = 1;
	ioctlsocket(sock, FIONBIO, &dwTemp);//设置非阻塞模式
	char hostName[128];
	if (gethostname(hostName, 100) == SOCKET_ERROR)
	{
		cout << "gethostname failed";
		return -1;
	}
	//获得本地 IP 地址
	hostent* pHostIP;
	if (!(pHostIP = gethostbyname(hostName)))
	{
		cout << "gethostbyname failed";
		return -1;
	}
	int i;
	int num;
	cout << "==" << second << "==" << endl;
	cout << "==监控时间为：" << second << "秒==" << endl;
	//枚举电脑上的所有网卡
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
	cout << "==请输入网卡序号（0-" << i - 1 << "):" << endl;
	cin >> num;//筛选网卡序号
	//填充 SOCKADDR_IN
	sockaddr_in addr_in;
	addr_in.sin_addr = *(in_addr*)pHostIP->h_addr_list[num];
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(8000);
	//把原始 socket 绑定到本地网卡上
	if (bind(sock, (PSOCKADDR)&addr_in, sizeof(addr_in)) == SOCKET_ERROR)
	{
		cout << "bind failed";
		return -1;
	}//if

	//设置 sock_raw 为 SIO_RCVALL,以便接收所有的 IP 包
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
	char buffer[BUFFER_SIZE]; //设置缓冲区
	time_t beg;//循环打印开始时间
	time_t end;//循环打印结束时间
	time_t temp;//每次循环打印的临时时间
	cout << endl;
	cout << endl;
	cout << "==本机IP：" << inet_ntoa(*((struct in_addr*)pHostIP->h_addr_list[num])) << "==" << endl;
	cout << "==开始捕获..." << endl;
	time(&beg);//获取beg的开始时间
	time(&temp);//初始化temp当前时间
	int j = 0;
	//循环开始
	while (true)
	{
		//每次循环开始时进行end时间的重新获取
		time(&end);
		//进行时间的判断，是否超过监控时间，超过就结束循环
		if (end - beg >= second)
		{
			break; //达到设定时间,退出
		}
		//判断是否接收到了ip流量包
		int size = recv(sock, buffer, BUFFER_SIZE, 0);
		if (size > 0)
		{
			ipHeader p = *(ipHeader*)buffer;
			if ((addr_in.sin_addr).S_un.S_addr == p.sourceip || (addr_in.sin_addr).S_un.S_addr == p.destip)
			{
				addNode(p); //把捕获到的包加入到链表
			}
		}

		ipNode* pt = pHead;
		//此次输出时间-上次输出时间是否大于1秒
		if ((end - temp) >= 1) {
			//清屏
			system("cls");
			j++;
			//输出ip包相关信息
			cout << "==本机IP：" << inet_ntoa(*((struct in_addr*)pHostIP->h_addr_list[num])) << "==" << j << endl;
			while (pt) {
				cout << "SourceIP:" << inet_ntoa(*(in_addr*)&(pt->sourceip)) << ";";
				cout << "DestIP:" << inet_ntoa(*(in_addr*)&(pt->destip)) << ";";
				cout << "Protocol:" << getProtocol(pt->protocol) << ";";
				cout << "PktSta:" << pt->count << endl;
				pt = pt->next;

			}
			//获取temp，此次输出的时间
			time(&temp);
		}
	}
	return 0;
}
