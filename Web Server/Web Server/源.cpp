
#include <stdio.h>
#include <WinSock2.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma pack(1)	//结构体类型在存储时按1字节对齐 5
char WEB_ROOT[MAX_PATH] = { 0 };	//web根目录
volatile unsigned int TcpClientCount = 0;	//连接的浏览器数
const int MAX_CLIENT = 100;	//设置支持的最大并发连接数
const int MAX_BUF_SIZE = 1024;
#define HTTPPORT	9000	//web server程序监听端口号
#define METHOD_GET	0	//http协议的GET方法
#define METHOD_HEAD	1	//http协议的HEAD方法
//http协议响应消息“状态行"常量
#define	HTTP_STATUS_OK	"200	OK"
#define	HTTP_STATUS_CREATED	"201	Created"
#define	HTTP_STATUS_ACCEPTED	"202	Accepted"
#define	HTTP_STATUS_NOCONTENT	"204	No Content"
#define	HTTP_STATUS_MOVEDPERM	"301	Moved Permanently"
#define	HTTP_STATUS_MOVEDTEMP	"302	Moved Temporarily"
#define	HTTP_STATUS_NOTMODIFIED	"304	Not Modified"
#define	HTTP_STATUS_BADREQUEST	"400	Bad Request"
#define	HTTP_STATUS_UNAUTHORIZED	"401	Unauthorized"
#define	HTTP_STATUS_FORBIDDEN	"403	Forbidden"
#define	HTTP_STATUS_NOTFOUND	"404	File can not found"
#define	HTTP_STATUS_SERVERERROR	"500	Internal Server Error"
#define	HTTP_STATUS_NOTIMPLEMENTED	"501	Not Implemented"
#define	HTTP_STATUS_BADGATEWAY	"502	Bad Gateway"
#define	HTTP_STATUS_UNAVAILABLE	"503	Service Unavailable"

//格林威治时间的星期转换
char* week[] = {
"Sun,",
"Mon,",
"Tue,",
"Wed,",
"Thu,",
"Fri,",
"Sat,",
};
//格林威治时间的月份转换
char* month[] = {
"Jan",
"Feb",
"Mar",
"Apr",
"May",
"Jun",
"Jul",
"Aug",
"Sep",
"Oct",
"Nov",
"Dec",
};
//文件后缀名与文档类型（Content-Type)之间的对应关系数组
char* typeMap[70][2] = {
	{".doc","application/msword"},
	{".bin","application/octet-stream"},
	{".dll","application/octet-stream"},
	{".exe","application/octet-stream"},
	{".pdf","application/pdf"},
	{".ai","application/postscript"},
	{".eps","application/postscript"},
	{".ps","application/postscript"},
	{".rtf","application/rtf"},
	{".fdf","application/vnd.fdf"},
	{".arj", "application/x-arj"},
	{".gz","application/x-gzip"},
	{".class","application/x-java-class"},
	{".js","application/x-javascript"},
	{".lzh","application/x-lzh"},
	{".lnk","application/x-ms-shortcut"},
	{".tar","application/x-tar"},
	{".hlp","application/x-winhelp"},
	{".cert","application/x-x509-ca-cert"},
	{".zip","application/zip"},
	{".cab","application/x-compressed"},
	{".arj","application/x-compressed"},
	{".aif","audio/aiff"},
	{".aifc","audio/aiff"},
	{".aiff","audio/aiff"},
	{".au","audio/basic"},
	{".snd","audio/basic"},
	{".mid","audio/midi"},
	{".rmi","audio/midi"},
	{".mp3","audio/mpeg"},
	{".vox","audio/voxware"},
	{".wav","audio/wav"},
	{".ra","audio/x-pn-realaudio"},
	{".ram","audio/x-pn-realaudio"},
	{".bmp","image/bmp"},
	{".gif","image/gif"},
	{".jpeg","image/jpeg"},//
	{".jpg","image/jpeg"}, //
	{".tif","image/tiff"},
	{".tiff","image/tiff"},
	{".xbm","image/xbm"},
		{".wrl","model/vrml"},
		{".htm","text/html"}, //
		{".html","text/html"}, //
		{".c","text/plain"},
		{".cpp","text/plain"},
		{".def","text/plain"},
		{".h","text/plain"},	{".txt","text/plain"},
	{".rtx","text/richtext"},
	{".rtf","text/richtext"},
	{".java","text/x-java-source"},
	{".css","text/css"},
	{".mpeg","video/mpeg"},
	{".mpg","video/mpeg"},
	{".mpe","video/mpeg"},
	{".avi","video/msvideo"},
	{".mov","video/quicktime"},
	{".qt","video/quicktime"},
	{".shtml","wwwserver/html-ssi"},
	{".asa","wwwserver/isapi"},
{".asp","wwwserver/isapi"},
{".cfm","wwwserver/isapi"},
{".dbm","wwwserver/isapi"},
{".isa","wwwserver/isapi"},
{".plx","wwwserver/isapi"},
{".url","wwwserver/isapi"},
{".cgi","wwwserver/isapi"},
{".php","wwwserver/isapi"},
{".wcgi","wwwserver/isapi"}
};
//传递给TCP线程的结构体参数
struct TcpThreadParam
{
	SOCKET socket;	//Browser端套接字
	sockaddr_in addr;	//Browser端地址
};

DWORD WINAPI TcpServeThread(LPVOID lpParam);//TCP线程的线程函数声明139
typedef struct REQUEST
{
	int	nMethod;	// 请求的使用方法：GET或HEAD
	HANDLE	hFile;	// 请求连接的文件
	char	szFileName[MAX_PATH]; // 文件的相对路径
	char	postfix[10];	// 存储扩展名
	int	fileSize;
	char	StatuCodeReason[100]; // 头部的status cod以及reason-phrase
}REQUEST, * PREQUEST;
//分析WEB浏览器发送的http请求
int Analyze(REQUEST* pReq, const char* pBuf);
//构造http协议响应消息中的“状态行、头部行和空行”
int MakeResHeader(REQUEST* pReq, char* resHeader);
//发送http协议响应消息中的“响应数据"，比如GET方法的响应数据通常是一个html页面
void SendFile(REQUEST* pReq, SOCKET skt);
//判断请求的文件是否存在
int FileExist(REQUEST* pReq);
//根据http请求中URL资源的后缀名，获取http响应消息“头部行”中的Content-Type字段
void GetContenType(REQUEST* pReq, char* type);

int	main(int argc, char* argv[])
{
	//初始化winsock2环境
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("初始化winsock 2失败！错误码=%d\r\n", WSAGetLastError());
		return -1;
	}
	//创建用于侦听的TCP Server Socket
	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("创建监听套接字失败！错误码=%d\r\n", WSAGetLastError());
	}
	//获取TCP监听端口号
	u_short ListenPort = HTTPPORT;
	if (argc >= 2) //argv[1]传递端口号
	{
		ListenPort = (u_short)atoi(argv[1]);
	}
	if (argc >= 3) //argv[2]传递web server根目录
	{
		strcpy(WEB_ROOT, argv[2]);
	}
	else
	{
		printf("请设定WEB服务器的根目录\r\n");
		scanf("%[^\r\n]", WEB_ROOT);
	}
	//获取本机名
	char hostname[256];
	gethostname(hostname, sizeof(hostname));
	//获取本地IP地址
	hostent* pHostent = gethostbyname(hostname);
	//填充本地TCP Socket地址结构
	SOCKADDR_IN ListenAddr;
	memset(&ListenAddr, 0, sizeof(SOCKADDR_IN));
	ListenAddr.sin_family = AF_INET;
	ListenAddr.sin_port = htons(ListenPort);
	//可枚举本机所有IP地址，再指定IP地址
	//*(in_addr*)pHostent->h_addr_list[0];
	ListenAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	//绑定TCP端口，最易出错的地方！！！
	if (bind(ListenSocket, (sockaddr*)&ListenAddr, sizeof(ListenAddr)) == SOCKET_ERROR)
	{
		printf("绑定（监听）套接字失败！错误码=%d\r\n", WSAGetLastError());
		return -1;
	}
	//监听
	if ((listen(ListenSocket, SOMAXCONN)) == SOCKET_ERROR)
	{
		printf("监听失败！错误码=%d\r\n", WSAGetLastError());
		return -1;
	}
	printf("启动WEB服务器，监听端口为：%d\r\n", ListenPort);
	SOCKET TcpSocket;
	SOCKADDR_IN TcpClientAddr;
	while (TRUE)
	{
		//接受WEB浏览器连接请求
		int iSockAddrLen = sizeof(sockaddr);
		if ((TcpSocket = accept(ListenSocket, (sockaddr*)&TcpClientAddr,
			&iSockAddrLen)) == SOCKET_ERROR)
		{
			printf("接受WEB浏览器连接失败！错误码=%d\r\n", WSAGetLastError());

			return -1;
		}
		if (TcpClientCount >= MAX_CLIENT)//TCP线程数达到上限，停止接受新的Client
		{
			closesocket(TcpSocket);
			printf("由于服务器连接数达到上限（%d),来自WEB浏览器[%s:%d]的连接请求被拒绝!\r\n", MAX_CLIENT, inet_ntoa(TcpClientAddr.sin_addr), ntohs(TcpClientAddr.sin_port));
			continue;
		}
		printf("接收到来自WEB浏览器[%s:%d]的连接请求！\r\n", inet_ntoa(TcpClientAddr.sin_addr),
			ntohs(TcpClientAddr.sin_port));
		TcpThreadParam Param;
		Param.socket = TcpSocket;
		Param.addr = TcpClientAddr;
		//创建TCP服务线程。一请求一Thread模式。
		DWORD dwThreadId;
		CreateThread(NULL, 0, TcpServeThread, &Param, 0, &dwThreadId);
		InterlockedIncrement(&TcpClientCount);//为什么不是TcpClientCount+

		printf("服务器当前连接数为：%d\r\n", TcpClientCount);
	}
	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}

DWORD WINAPI TcpServeThread(LPVOID lpParam)//TCP服务线程
{
	char buf[MAX_BUF_SIZE] = { 0 };
	char resHeader[MAX_BUF_SIZE] = { 0 };
	REQUEST httpReq = { 0 };
	//获取线程参数
	SOCKET TcpSocket = ((TcpThreadParam*)lpParam)->socket;

	SOCKADDR_IN TcpClientAddr = ((TcpThreadParam*)lpParam)->addr;
	//输出提示信息
	printf("服务WEB浏览器[%s:%d]的线程ID是：%d\r\n", inet_ntoa
	(TcpClientAddr.sin_addr),
		ntohs(TcpClientAddr.sin_port), GetCurrentThreadId());
	int TCPBytesReceived = 0;
	TCPBytesReceived = recv(TcpSocket, buf, sizeof(buf), 0);
	//TCPBytesReceived值为0表示client端已正常关闭连接
			//TCPBytesRecieved值为SOCKET_ERROR则表示socket的状态不正常,无法继续数据通  信
			//两种情况下都表明本线程的任务已结束，需要退出
	if (TCPBytesReceived == 0 || TCPBytesReceived == SOCKET_ERROR)
	{
		printf("WEB浏览器[%s:%d]已经关闭，线程ID为%d的线程退出！\r\n", inet_ntoa(TcpClientAddr.sin_addr),

			ntohs(TcpClientAddr.sin_port), GetCurrentThreadId());
	}
	else
	{
		int nRet = Analyze(&httpReq, buf);//分析http请求消息
		if (nRet)
		{
			printf("分析WEB浏览器http请求错误！\r\n");
		}
		else
		{
			MakeResHeader(&httpReq, resHeader);//构造http响应消息的状态行、头 部行和空行
			send(TcpSocket, resHeader, strlen(resHeader), 0);//发送http响应消息的状态行、头部行和空行
			if (httpReq.nMethod == METHOD_GET && httpReq.hFile != INVALID_HANDLE_VALUE)
			{
				//发送http协议GET方法请求的页面文件资源
				SendFile(&httpReq, TcpSocket);
			}
		}
	}
	InterlockedDecrement(&TcpClientCount);//为什么不是TcpClientCount--？
	closesocket(TcpSocket);
	printf("服务WEB浏览器[%s:%d]的线程（ID：%d）退出\r\n", inet_ntoa
	(TcpClientAddr.sin_addr),
		ntohs(TcpClientAddr.sin_port), GetCurrentThreadId());
	return 0;
}

int Analyze(PREQUEST pReq, const char* pBuf)// 分析http协议请求消息
{
	char szSeps[] = " \r\n";
	char* cpToken;
	if (strstr((const char*)pBuf, "..") != NULL)// 防止非法请求
	{
		strcpy(pReq->StatuCodeReason, HTTP_STATUS_BADREQUEST);
		return 1;
	}
	// 判断http协议请求消息中的“方法”	
	cpToken = strtok((char*)pBuf, szSeps);	// 缓存中字符串分解为一组标记串。


	if (!_stricmp(cpToken, "GET"))	// GET方法
	{
		pReq->nMethod = METHOD_GET;
	}
	else if (!_stricmp(cpToken, "HEAD"))	// HEAD方法
	{
		pReq->nMethod = METHOD_HEAD;
	}
	else
	{
		strcpy(pReq->StatuCodeReason, HTTP_STATUS_NOTIMPLEMENTED);
		return 1;
	}
	// 获取http协议请求消息中的“URL”
	cpToken = strtok(NULL, szSeps);
	if (cpToken == NULL)
	{
		strcpy(pReq->StatuCodeReason, HTTP_STATUS_BADREQUEST);
		return 1;
	}
	//获取WEB根目录
	strcpy(pReq->szFileName, WEB_ROOT);
	if (strlen(cpToken) > 1)
	{
		//把请求资源文件名拼接到根目录后，形成资源文件的本地完整路径
		//注意http请求资源相对路径与本地完整路径之间的对应关系
		strcat(pReq->szFileName, cpToken);
	}
	else
	{
		//如果没有请求资源文件，指定一个默认主页文件
		strcat(pReq->szFileName, "/index.html");
	}
	return 0;
}

int MakeResHeader(REQUEST* pReq, char* resHeader)//构造http协议响应消息的状态行、头部行和空行
{
	int ret = FileExist(pReq);
	char curTime[50] = { 0 };
	//本地时间
	SYSTEMTIME st;
	GetLocalTime(&st);
	//时间格式化
	sprintf(curTime, "%s %02d %s %d %02d:%02d:%02d GMT", week
		[st.wDayOfWeek], st.wDay,
		month[st.wMonth - 1], st.wYear, st.wHour, st.wMinute, st.wSecond);

	//取得请求资源文件的last-modified时间
	char last_modified[60] = { 0 };
	if (ret == 0)
	{
		//获得文件的last-modified 时间
		FILETIME ftCreate, ftAccess, ftWrite;
		SYSTEMTIME stCreate;
		FILETIME ftime;
		//获得文件的last-modified的UTC时间
		GetFileTime(pReq->hFile, &ftCreate, &ftAccess, &ftWrite);
		FileTimeToLocalFileTime(&ftWrite, &ftime);
		//UTC时间转化成本地时间
		FileTimeToSystemTime(&ftime, &stCreate);
		//时间格式化
		sprintf(last_modified, "%s %02d %s %d %02d:%02d:%02d GMT", week[stCreate.wDayOfWeek], stCreate.wDay, month[stCreate.wMonth - 1], stCreate.wYear, stCreate.wHour, st.wMinute, st.wSecond);
	}
	char ContenType[50] = { 0 };
	GetContenType(pReq, ContenType);// 取得文件的类型
	//头部行之后，切记添加一个空行"\r\n" !!!	
	sprintf(resHeader, "HTTP/1.1 %s\r\nDate: %s\r\nServer:	%s\r\nContent-Type: % s\r\n\
					Content - Length: % d\r\nLast - Modified: % s\r\n\r\n",
		pReq->StatuCodeReason,
		curTime,	// Date
		"My Http Server",	// Server
		ContenType,	// Content-Type
		pReq->fileSize,	// Content-length
		ret == 0 ? last_modified : curTime); // Last- Modified
	return 0;
}
int	FileExist(REQUEST* pReq)//判断http协议请求资源文件是否存在
{
	//CreateFile和fopen之间有何区别与联系？
	pReq->hFile = CreateFile(pReq->szFileName, GENERIC_READ,
		FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (pReq->hFile == INVALID_HANDLE_VALUE)// 如果文件不存在，则返回出错信息
	{

		strcpy(pReq->StatuCodeReason, HTTP_STATUS_NOTFOUND);
		pReq->fileSize = 0;
		return 1;
	}
	else
	{
		strcpy(pReq->StatuCodeReason, HTTP_STATUS_OK);


		pReq->fileSize = GetFileSize(pReq->hFile, NULL);
		return 0;
	}
}

void SendFile(REQUEST* pReq, SOCKET skt)//发送http协议响应消息的“响应数据”部分
{
	int n = FileExist(pReq);
	if (n)// 文件不存在，则返回
	{
		return;
	}
	char buf[2048] = { 0 };
	DWORD dwRead = 0;
	BOOL	fRet = FALSE;
	char szMsg[512] = { 0 };
	int flag = 1;
	while (1)// 读写数据直到完成
	{
		fRet = ReadFile(pReq->hFile, buf, sizeof(buf), &dwRead, NULL);//从file中读入到buffer中
		if (!fRet)
		{
			sprintf(szMsg, "%s", HTTP_STATUS_SERVERERROR);
			send(skt, szMsg, strlen(szMsg), 0); //向客户端发送出错信息
			break;
		}
		if (dwRead == 0)//完成
		{
			break;
		}
		send(skt, buf, dwRead, 0);// 将buffer内容传送给client
	}
	CloseHandle(pReq->hFile);// 关闭文件
	pReq->hFile = INVALID_HANDLE_VALUE;
}

void GetContenType(REQUEST* pReq, char* type)//根据文件后缀名构造http协议响应消息头部行中的“Content-Type”
{
	char* cpToken = strrchr(pReq->szFileName, '.');// 取得文件的后缀名
	strcpy(pReq->postfix, cpToken);
	// 遍历搜索该文件类型对应的content-type
	if (cpToken == NULL)
	{
		sprintf(type, "%s", "text/html");
		return;
	}
	int i = 0;
	for (i = 0; i < 70; i++)
	{
		if (strcmp(typeMap[i][0], pReq->postfix) == 0)
		{


			strcpy(type, typeMap[i][1]);
			break;
		}
	}
	if (i == 70) //not find
	{
		sprintf(type, "%s", "text/html");
	}
}

