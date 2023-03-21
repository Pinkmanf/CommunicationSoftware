#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

DWORD dwBus = 0;
#define SEND 0xf0000000
#define ACK 0x0f000000
#define CONF 0x00f00000

//发送线程函数
DWORD WINAPI HostA(LPVOID)
{
	//初始化发送次数和重发次数
	int i = 0;
	int nSendTimes = 0;
	while (i != 10) {
	Wait:
		//检测信道是否空闲
		while (dwBus != 0) {
			Sleep(100);
		}
		Sleep(rand() % 100);
		if (dwBus != 0) {
			//回到循环开始时
			goto Wait;
		}
		//设置信道为发送状态
		dwBus = SEND + i;
		Sleep(1000);
		//信道为接收状态，表示发送数据报成功
		if (dwBus == ACK + i) {
			printf("发送第%d个数据包成功。\r\n", i + 1);
			i++;
			nSendTimes = 0;
			Sleep(1000);
		}
		else
		{
			//没有收到ack，重新发送
			nSendTimes++;
			//重发次数是否小于10
			if (nSendTimes < 10) {
				printf("未能收到第%d个数据包的ACK响应，第%d次重新发送。\r\n", i + 1, nSendTimes);
				int nSleepTime = (rand() % 3) * (int)pow(2, nSendTimes);

				Sleep(nSleepTime * 50);
			}
			else {
				printf("放弃发送第 %d个数据包。\r\n",i + 1);
				i++;
			}
		}
	}
	return 0;
}

DWORD WINAPI HostB(LPVOID) {
	do {
		//检测信道是否为发送状态
		while ((dwBus & 0xf0000000) != SEND) {
			Sleep(10);
		}
		//将信道置为接收到ack状态
		dwBus = ACK + (dwBus & 0x0000000f);
	} while (dwBus != SEND + 9);
	return 0;
}

int main(int argc,char* argv[]) {
	dwBus = 0;
	DWORD ThreadIDA = 0;
	DWORD ThreadIDB = 0;
	//创建两个发送接收线程
	CreateThread(NULL, 0, HostB, NULL, 0, &ThreadIDB);
	CreateThread(NULL, 0, HostA, NULL, 0, &ThreadIDA);
	//初始化随机函数种子
	//srand((unsigned)time(NULL));
	for (int i = 0; i < 25; i++)
	{

		//随机制造冲突
		Sleep((rand() % 20) * 100);
		dwBus = dwBus | CONF;

		Sleep(2000);
		dwBus = 0;
	}
	printf("仿真结束。\r\n");
	return 0;
}