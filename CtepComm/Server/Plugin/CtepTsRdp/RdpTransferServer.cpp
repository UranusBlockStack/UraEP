// RdpTransferServer.cpp : ���� DLL Ӧ�ó���ĵ���������


#include "stdafx.h"
#include "CommonInclude/CommonImpl.h"
#include "RdpChannelServer.h"
#include "RdpTS.h"

using namespace std;
#include <Atlcoll.h>

#define  MONITOR_WND_CLASS_BASE_NAME "CTepSessionMonitorClass"
#pragma comment(lib,"Wtsapi32.lib")

static HANDLE    h_SessionMonitorThread = NULL;
HWND h_MainWnd = NULL;


CAtlMap<DWORD, CTransferChannel* > g_mapSessionData;
CMyCriticalSection lckMap;

void AddNewSession(DWORD dwSessionId)
{
	CTransferChannel* pTransChn = nullptr;
	LOCK(&lckMap);
	auto pair = g_mapSessionData.Lookup(dwSessionId);
	if ( pair)
	{
		pTransChn = pair->m_value;
	}
	if ( pTransChn)
		return ;

	WCHAR *userName = nullptr;  //�Ự�û���
	DWORD len = 0;

	//��Ҫ��ȡ�Ự�û���,ͨ�Ų�ȥ����,����һ���ṹ
	if ( WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, dwSessionId, WTSUserName,&userName, &len))
	{
		if( wcslen(userName))
		{
			//��RDPͨ��
			HANDLE hChannel =  CRdpChannelServer::RdpChannelOpen(dwSessionId);
			if ( hChannel == NULL )
				return;

			ReadWritePacket *m_Rwpacket = m_piCallBack->AllocatePacket(CTEPGetInterfaceTransServer());
			ASSERT(m_Rwpacket && m_Rwpacket->buff.size == 0 && m_Rwpacket->ol.InternalHigh == 0);
			StListenData* pListenData = (StListenData*)m_Rwpacket->buff.buff;

			m_Rwpacket->opType = OP_Listen;
			m_Rwpacket->hFile = hChannel;
			pListenData->hRdpChannel = hChannel;
			pListenData->dwMagic = 0x12344321;
			pListenData->dwSessionId = dwSessionId;

			g_mapSessionData[dwSessionId] = (CTransferChannel*)-1;
			ASSERT(m_Rwpacket && m_Rwpacket->buff.size == 0 && m_Rwpacket->ol.InternalHigh == 0);
			if( PostQueuedCompletionStatus(m_piCallBack->GetCompletePort(), 0, 0, &m_Rwpacket->ol) == FALSE )
			{
				CRdpChannelServer::RdpChannelClose(hChannel);
				ASSERT(0);
			}
			//m_piCallBack->InsertPendingAccept(m_Rwpacket);
			WTSFreeMemory(userName);
		}
	}
}

void DeleteSession(DWORD dwSessionId)
{
	CTransferChannel* pTransChn = nullptr;
	LOCK(&lckMap);
	auto pair = g_mapSessionData.Lookup(dwSessionId);
	if ( pair)
	{
		pTransChn = pair->m_value;
		if ( pTransChn && pTransChn != (CTransferChannel*)-1)
		{
			HANDLE hFile = pTransChn->hFile;
			pTransChn->hFile = INVALID_HANDLE_VALUE;
			WTSVirtualChannelClose(hFile);
		}
		g_mapSessionData.RemoveKey(dwSessionId);
	}
}

static void SessionChangeDispatch(WPARAM wParam, LPARAM lParam)
{
	//wParam ���ݵ��ǻỰ״̬�ı��¼����ͣ�lParam���ݵ��ǻỰID
	switch(wParam)
	{
	case WTS_CONSOLE_CONNECT:
		break;
	case WTS_CONSOLE_DISCONNECT:
		break;

	case WTS_REMOTE_CONNECT:
		OutputDebugString(L"WTS_REMOTE_CONNECT");
		AddNewSession(DWORD(lParam));
		break;

	case WTS_REMOTE_DISCONNECT:
		OutputDebugString(L"WTS_REMOTE_DISCONNECT");
		DeleteSession(DWORD(lParam));
		break;

	case WTS_SESSION_LOGON:
		OutputDebugString(L"WTS_SESSION_LOGIN");
		AddNewSession(DWORD(lParam));
		break;

	case WTS_SESSION_LOGOFF:
		OutputDebugString(L"WTS_SESSION_LOGFF");
		DeleteSession(DWORD(lParam));
		break;
	}
}


//������Ϣ������
static LRESULT CALLBACK SessionMonitorWndProc( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_WTSSESSION_CHANGE:
		SessionChangeDispatch(wParam, lParam);
		return 0;

	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;

	case WM_QUIT:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}




BOOL RegisterMonitorWndClass()
{
	WCHAR name_buf[512];
	WNDCLASSEX wc;
	swprintf_s(name_buf, L"%s-%d", MONITOR_WND_CLASS_BASE_NAME, GetCurrentProcessId());

	memset((LPVOID)&wc, 0, sizeof(WNDCLASSEX) );
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_VREDRAW | CS_HREDRAW ;
	wc.lpfnWndProc   = SessionMonitorWndProc;
	wc.hInstance     = GetModuleHandle( NULL );
	wc.lpszMenuName  = name_buf;
	wc.lpszClassName = name_buf;

	if( !RegisterClassEx(&wc))
	{
		return FALSE;
	}
	return TRUE;
}
HWND CreateMonitorWindows()
{
	HWND hWnd;
	WCHAR name_buf[512];
	swprintf_s(name_buf, L"%s-%d",MONITOR_WND_CLASS_BASE_NAME,GetCurrentProcessId());
	hWnd = CreateWindow(name_buf, name_buf, WS_OVERLAPPED, 
		0, 0, 0, 0, NULL, NULL, GetModuleHandle( NULL ), NULL );

	if( NULL == hWnd )
		return NULL;

	UpdateWindow(hWnd);
	return hWnd;

}
static DWORD WINAPI SessionMonitorThread( LPVOID param)
{
	if ( !RegisterMonitorWndClass())//ע��һ�����ڼ��ӵĴ�����
		goto End;

	//����һ��������
	h_MainWnd = CreateMonitorWindows();
	if( !h_MainWnd)
		goto End;

	//�ȴ�Terminal Server������Ȼ����WTSRegisterSessionNotificationע��Ự֪ͨ��Ϣ
	//�����RDS��������ǰ����WTSRegisterSessionNotification�������᷵��RPC_S_INVALID_BINDING����
	HANDLE hTermSrvReadyEvent = OpenEvent(SYNCHRONIZE, FALSE, _T("Global\\TermSrvReadyEvent"));
	if( hTermSrvReadyEvent == NULL )
		goto End;

	if( WTSRegisterSessionNotification(h_MainWnd, NOTIFY_FOR_ALL_SESSIONS) == FALSE )
		goto End;

	*(BOOL*)param = TRUE;

	MSG msg;
	while( ::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg);
	}

End:
	CloseHandle(h_MainWnd);
	CloseHandle(hTermSrvReadyEvent);

	return 0;
}



BOOL CtepTsRdpMainThreadInit()
{
	if( h_SessionMonitorThread != NULL )
		return TRUE;

	//���������߳�
	volatile BOOL bMonitorSucess = FALSE;
	//h_SessionMonitorThread = (HANDLE)_beginthread(SessionMonitorThread, 0, (LPVOID)&bMonitorSucess);
	h_SessionMonitorThread = CreateThread(NULL, 0, SessionMonitorThread, (LPVOID)&bMonitorSucess, 0, 0);

	int iCount =0;
	while ( (++iCount< 50) && (bMonitorSucess == FALSE))
	{
		Sleep(1);
	}

	if( bMonitorSucess == FALSE )
	{
		CtepTsRdpMainThreadClose();
		return FALSE;
	}
	else
		return TRUE;
}

void CtepTsRdpMainThreadClose()
{
	if ( IsWindow(h_MainWnd))
	{
		DestroyWindow(h_MainWnd);
		h_MainWnd = NULL;
	}
	if(h_SessionMonitorThread != NULL )
	{
		DWORD dwWait = WaitForSingleObject(h_SessionMonitorThread, 100);
		if ( dwWait == WAIT_TIMEOUT)
		{
			TerminateThread(h_SessionMonitorThread, 1);
		}
		
		h_SessionMonitorThread = NULL;
	}
}