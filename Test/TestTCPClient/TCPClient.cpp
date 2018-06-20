//////////////////////////////////////////////////////////
// TCPClient.cpp�ļ�

#include "assert.h"
#include "initSock.h"
#include <stdio.h>
CInitSock initSock;		// ��ʼ��Winsock��

SOCKET s;
long volatile nSendAll = 0;
long volatile nRecvAll = 0;

#include "CommonInclude/Tools/FastQueue.h"

struct T
{
	T* pNext;
};
MutliPriorityQueue<T> queue;


DWORD WINAPI ThreadProc(__in  LPVOID lpParameter)
{
	queue.IsUsed();
	T a;
	queue.Push(&a, 1);
	queue.Pop();
	queue.Lock();
	queue.Unlock();
	queue.SetUsed(TRUE);
	return 0;
}


int main(int nArg, CHAR* Arg[])
{
	int iRet;
	long port;

	// ��дԶ�̵�ַ��Ϣ
	sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(4567);
	// ע�⣬����Ҫ��д����������TCPServer�������ڻ�����IP��ַ
	// �����ļ����û��������ֱ��ʹ��127.0.0.1����
	servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");


	if ( nArg >= 2)
	{
		servAddr.sin_addr.S_un.S_addr = inet_addr(Arg[1]);
	}
	if ( nArg >= 3)
	{
		port = strtol(Arg[2], 0, 10);
		if ( port > 0 && port < 65535)
		{
			servAddr.sin_port = htons((USHORT)port);
		}
	}

	HANDLE hEvt = CreateEvent(0, TRUE, FALSE, "TestTcpClient.Limb.1");
	assert(hEvt);
	DWORD dwErr = GetLastError();
	if ( dwErr !=  ERROR_ALREADY_EXISTS)
	{
		Sleep(12000);
		SetEvent(hEvt);
		Sleep(1);
	}
	else
		WaitForSingleObject(hEvt, 15000);

	DWORD dwTimeTick = GetTickCount();

	for ( int j = 0; j < 200; j++)
	{
		// �����׽���
		s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(s == INVALID_SOCKET)
		{
			printf(" Failed socket() \n");
			ASSERT(0);
			return 0;
		}

		unsigned   long   sendbufsize = 0;
		int size = sizeof(sendbufsize);
		iRet = getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&sendbufsize, &size);
		sendbufsize = 64*1024;
		iRet = setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&sendbufsize, size);

		// Ҳ�������������bind������һ�����ص�ַ
		// ����ϵͳ�����Զ�����

		if(::connect(s, (sockaddr*)&servAddr, sizeof(servAddr)) == -1)
		{
			printf(" Failed connect() \n");
			ASSERT(0);
			return 0;
		}

		for (int x=0; x<100; x++)
		{
			//��������
			nSendAll = 0;
			char SendBuff[] = "[test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str][test_Str]";
			for (int i = 0; i<5 && nSendAll < 3400; i++)
			{
				DWORD dwWantSend = rand()%2999 + 1;
				int nSend = ::send(s, SendBuff, dwWantSend, 0);
				assert(nSend == dwWantSend);
				nSendAll += nSend;
				//Sleep(0);
			}

#define TIMES			10
			// ��������
			nRecvAll = 0;
			while ( nRecvAll < nSendAll*TIMES)
			{
				char buff[3001];
				int nRecv = ::recv(s, buff, 3000, 0);
				if(nRecv > 0)
				{
					buff[nRecv] = '\0';
					//printf(" ���յ�����(%d-%d)��[%s]\n", j, nRecv, buff);
					nRecvAll += nRecv;
				}
				else
				{
					printf("error!!! Err:%d", GetLastError());
					ASSERT(0);
					nRecvAll = nSendAll = 0;
					break;
				}
			}

			if ( nSendAll == 0)
				break;

			if ( nRecvAll != nSendAll*TIMES)
			{
				printf("nRecvAll:%d == nSendAll:(%d)\n", nRecvAll, nSendAll);
				Sleep(1);
				assert(nRecvAll == nSendAll*TIMES);
			}
		}

		
		// �ر��׽���
		::closesocket(s);
	}

	DWORD dwNewTick = GetTickCount();

	printf("\nAll Time:[%8d]\n", dwNewTick - dwTimeTick);
	system("pause");
	return 0;
}









