#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

DWORD dwBus = 0;
#define SEND 0xf0000000
#define ACK 0x0f000000
#define CONF 0x00f00000

//�����̺߳���
DWORD WINAPI HostA(LPVOID)
{
	//��ʼ�����ʹ������ط�����
	int i = 0;
	int nSendTimes = 0;
	while (i != 10) {
	Wait:
		//����ŵ��Ƿ����
		while (dwBus != 0) {
			Sleep(100);
		}
		Sleep(rand() % 100);
		if (dwBus != 0) {
			//�ص�ѭ����ʼʱ
			goto Wait;
		}
		//�����ŵ�Ϊ����״̬
		dwBus = SEND + i;
		Sleep(1000);
		//�ŵ�Ϊ����״̬����ʾ�������ݱ��ɹ�
		if (dwBus == ACK + i) {
			printf("���͵�%d�����ݰ��ɹ���\r\n", i + 1);
			i++;
			nSendTimes = 0;
			Sleep(1000);
		}
		else
		{
			//û���յ�ack�����·���
			nSendTimes++;
			//�ط������Ƿ�С��10
			if (nSendTimes < 10) {
				printf("δ���յ���%d�����ݰ���ACK��Ӧ����%d�����·��͡�\r\n", i + 1, nSendTimes);
				int nSleepTime = (rand() % 3) * (int)pow(2, nSendTimes);

				Sleep(nSleepTime * 50);
			}
			else {
				printf("�������͵� %d�����ݰ���\r\n",i + 1);
				i++;
			}
		}
	}
	return 0;
}

DWORD WINAPI HostB(LPVOID) {
	do {
		//����ŵ��Ƿ�Ϊ����״̬
		while ((dwBus & 0xf0000000) != SEND) {
			Sleep(10);
		}
		//���ŵ���Ϊ���յ�ack״̬
		dwBus = ACK + (dwBus & 0x0000000f);
	} while (dwBus != SEND + 9);
	return 0;
}

int main(int argc,char* argv[]) {
	dwBus = 0;
	DWORD ThreadIDA = 0;
	DWORD ThreadIDB = 0;
	//�����������ͽ����߳�
	CreateThread(NULL, 0, HostB, NULL, 0, &ThreadIDB);
	CreateThread(NULL, 0, HostA, NULL, 0, &ThreadIDA);
	//��ʼ�������������
	//srand((unsigned)time(NULL));
	for (int i = 0; i < 25; i++)
	{

		//��������ͻ
		Sleep((rand() % 20) * 100);
		dwBus = dwBus | CONF;

		Sleep(2000);
		dwBus = 0;
	}
	printf("���������\r\n");
	return 0;
}