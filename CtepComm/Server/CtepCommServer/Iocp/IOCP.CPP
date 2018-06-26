#include "stdafx.h"

#include "Interface/CTEP_Common_Struct_Ex.h"
#include "iocp.h"

#pragma comment(lib, "WS2_32.lib")

#define CTEP_DEBUG_SENDPACKET_LEVEL_OFF		0

Log4CppLib CIOCPServer::m_log("Iocp");

CIOCPServer::CIOCPServer()
{
	{
		ReadWritePacket* pBuffer = (ReadWritePacket *)::HeapAlloc(GetProcessHeap(), 
			HEAP_ZERO_MEMORY, sizeof(ReadWritePacket));
		ASSERT(pBuffer);
		pBuffer->PacketInit();
		m_queFreePacket.Initialize(pBuffer, true, true, 1);
	}
	{
		CTransferChannelEx* pContext = new CTransferChannelEx();
		ASSERT(pContext);
		m_queFreeContext.Initialize(pContext, true, true, 150);
	}
	m_quePendingAccepts.Initialize(true, true, true);
	//m_queConnectionList.Initialize(true, true, true);

	// �����ڴ��
	for (long i = m_queFreePacket.GetCount(); i < 500; i++)
	{
		ReadWritePacket* pBuffer = (ReadWritePacket *)::HeapAlloc(GetProcessHeap(), 
			HEAP_ZERO_MEMORY, sizeof(ReadWritePacket));
		if ( pBuffer)
		{
			pBuffer->PacketInit();
			m_queFreePacket.Push(pBuffer);
		}
	}

	for (long i = m_queFreeContext.GetCount(); i < 300; i++)
	{
		CTransferChannelEx* pContext = new CTransferChannelEx();
		if ( pContext)
		{
			m_queFreeContext.Push(pContext);
		}
	}

	FindMoudleInterface(CLoadModules::CtepTransServer);
	FindMoudleInterface(CLoadModules::CtepAppServer);

	// Accept����
	m_hAcceptEvent = ::WSACreateEvent();
	ASSERT(WSA_INVALID_EVENT != m_hAcceptEvent);
	m_hRepostEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	ASSERT(m_hRepostEvent);
	m_nRepostCount = 0;

	m_nMaxAccepts = 150;
	m_nMaxSends = 1000;
	m_nMaxFreeBuffers = 2000;
	m_nMaxConnections = 3000;

	m_nIocpOutStandingSend = 0;
	m_nIocpOutStandingRecv = 0;
	m_nIocpOutStandingListen = 0;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	m_dwWorkThreadCount = si.dwNumberOfProcessors*2;
	if ( m_dwWorkThreadCount < 4)
	{
		m_dwWorkThreadCount = 4;
	}
	else if ( m_dwWorkThreadCount > MAX_THREAD)
	{
		m_dwWorkThreadCount = MAX_THREAD;
	}

#ifdef _DEBUG
	//m_dwWorkThreadCount = 1;
#endif // _DEBUG

	m_hListenThread = NULL;
	m_hSyncWorkThread = NULL;
	m_hCompletion = NULL;

	m_bShutDown = FALSE;
	m_bServerStarted = FALSE;
}

CIOCPServer::~CIOCPServer()
{
	Shutdown();

 	if(m_hListenThread != NULL)
 		::CloseHandle(m_hListenThread);

	if ( m_hSyncWorkThread != nullptr)
		::CloseHandle(m_hSyncWorkThread);

	::CloseHandle(m_hRepostEvent);
	::CloseHandle(m_hAcceptEvent);

}


ReadWritePacket *CIOCPServer::AllocateBuffer(CTransferChannel* pContext, EmPacketOperationType type, int nLen/* = 0*/)
{
	ReadWritePacket *pBuffer = NULL;

	ASSERT(nLen <= CTEP_DEFAULT_BUFFER_SIZE);

	// Ϊ���������������ڴ�
	pBuffer = m_queFreePacket.Pop();
	if( !pBuffer)  // �ڴ��Ϊ�գ������µ��ڴ�
	{
		pBuffer = (ReadWritePacket *)::HeapAlloc(GetProcessHeap(), 
			HEAP_ZERO_MEMORY, sizeof(ReadWritePacket));
	}

	// ��ʼ���µĻ���������
	if( pBuffer)
	{
		pBuffer->PacketInit();
		pBuffer->opType = type;
		pBuffer->buff.size = nLen;

		if (pContext)
		{
			pBuffer->piTrans = pContext->piTrans;
			pBuffer->hFile = pContext->hFile;
			pBuffer->pTransChn = pContext;
		}

		ASSERT( (type != OP_IocpRecv && type != OP_IocpSend) || pContext);
		if ( type == OP_IocpRecv)
		{
			DWORD dwSize = 0;
			pBuffer->buff.buff = ((CTransferChannelEx*)pContext)->rbufPacket.GetBuffer(&dwSize);
			ASSERT(dwSize >= CTEP_DEFAULT_BUFFER_SIZE);
			pBuffer->buff.maxlength = dwSize;
		}
	}

	return pBuffer;
}

void CIOCPServer::ReleaseBuffer(ReadWritePacket *pBuffer)
{
	if( m_queFreePacket.GetCount() <= m_nMaxFreeBuffers)	// ��Ҫ�ͷŵ��ڴ����ӵ������б���
	{
#if _DEBUG
		pBuffer->buff.buff = (char*)11;
#endif
		m_queFreePacket.Push(pBuffer);
	}
	else	// �Ѿ��ﵽ���ֵ���������ͷ��ڴ�
	{
		::HeapFree(::GetProcessHeap(), 0, pBuffer);
	}
}
void CIOCPServer::ReleaseContext(CTransferChannelEx *pContext)
{
	ASSERT(pContext->bClosing);
	m_queFreeContext.Push(pContext);

	while ( m_queFreeContext.GetCount() > m_nMaxConnections / 5 + 150)	// ��Ҫ�ͷŵ��ڴ����ӵ������б���
	{
		// �Ѿ��ﵽ���ֵ���������ͷ��ڴ�
		CTransferChannelEx *pDelete = m_queFreeContext.Pop();
		delete pDelete;
	}
}

#ifdef _DEBUG
DWORD DebugGetBagSize(char* pbuf, DWORD dwSize, void* pParam)
{
	UNREFERENCED_PARAMETER(pParam);
	if ( dwSize >= 4)
	{
		CTEPPacket_Header* pPacket = (CTEPPacket_Header*)pbuf;
		ASSERT(pPacket->magic == 'E');
		ASSERT(pPacket->PacketLength <= CTEP_DEFAULT_BUFFER_SIZE);
		ASSERT((pPacket->Type&CTEP_PACKET_CONTENT_MASK) <= 0x70);
		ASSERT((pPacket->Type&CTEP_PACKET_SEGMENT_MASK) <= 0x03);
	}
	if ( dwSize < 2)
	{
		return 0;
	}
	//ASSERT(dwSize >= 2);
	return *((USHORT*)pbuf);
}
#endif // _DEBUG

CTransferChannelEx *CIOCPServer::AllocateContext(HANDLE s, ICTEPTransferProtocolServer* piTrans)
{
	CTransferChannelEx *pContext;

	ASSERT(piTrans && s != INVALID_HANDLE_VALUE && s != 0);

	// ����һ��StTransferChannelEx����
	pContext = m_queFreeContext.Pop();
	if ( !pContext)
	{
		pContext = new CTransferChannelEx();
	}

	ASSERT(pContext);
	if( pContext)
	{// ��ʼ�������Ա
		pContext->InitEx();
#ifdef _DEBUG
		pContext->rbufPacket.fn_GetBagSize = DebugGetBagSize;
#endif // _DEBUG
		pContext->hFile = s;
		pContext->piTrans = piTrans;
		pContext->bClosing = FALSE;
		if ( piTrans == m_TS[0])
		{
			pContext->type = EnTransferChannelType::TCP;
		}
		else if ( piTrans == m_TS[1])
		{
			pContext->type = EnTransferChannelType::UDP;
		}
		else
		{
			if ( piTrans->SupportIOCP() == CTEP_TS_SUPPORT_IOCP)
			{
				pContext->type = EnTransferChannelType::IocpMain;
			}
			else if ( piTrans->SupportIOCP() == CTEP_TS_SUPPORT_SYNC)
			{
				pContext->type = EnTransferChannelType::SyncMain;
			}
			else
			{
				ASSERT(0);
				m_queFreeContext.Push(pContext);
				pContext = nullptr;
			}
		}
	}

	return pContext;
}

void CIOCPServer::FreeBuffers()
{
	// ����m_pFreeBufferList�����б����ͷŻ��������ڴ�
	m_queFreePacket.Lock();

	ReadWritePacket *pFreeBuffer = m_queFreePacket.Pop();
	while(pFreeBuffer != NULL)
	{
		if(!::HeapFree(::GetProcessHeap(), 0, pFreeBuffer))
		{
			ASSERT(0);
			break;
		}
		pFreeBuffer = m_queFreePacket.Pop();
	}

	m_queFreePacket.Unlock();
}

void CIOCPServer::FreeContexts()
{
	// ����m_pFreeBufferList�����б����ͷŻ��������ڴ�
	m_queFreeContext.Lock();

	CTransferChannelEx *pFreeContext = m_queFreeContext.Pop();
	while(pFreeContext != NULL)
	{
		delete pFreeContext;
		pFreeContext = m_queFreeContext.Pop();
	}
	m_queFreeContext.Unlock();
}


BOOL CIOCPServer::AddAConnection(CTransferChannelEx *pContext)
{
	// ��ͻ������б�����һ��StTransferChannelEx����
	if(m_lstConnection.GetCount() <= (DWORD)m_nMaxConnections)
	{
		// ���ӵ���ͷ
		m_lstConnection.AddHead(pContext);
		return TRUE;
	}

	return FALSE;
}

void CIOCPServer::CloseAConnection(CTransferChannelEx *pContext)
{
	if ( pContext->bClosing == TRUE)
		return ;

	::EnterCriticalSection(&pContext->lckSend);
	if ( pContext->bClosing == FALSE)
	{
		pContext->bClosing = TRUE;

		// ���ȴ��б����Ƴ�Ҫ�رյ�����
		m_lcklstConnection.Lock();
		POSITION pos = m_lstConnection.Find(pContext);
		ASSERT(pos);
		if ( pos)
		{
			m_lstConnection.RemoveAt(pos);
		}
		m_lcklstConnection.Unlock();

		// Ȼ��رտͻ��׽���
		pContext->DisconnectServer();
		ReleaseContext(pContext);
	}
	else
	{
		ASSERT(pContext->bClosing == TRUE);
	}
	::LeaveCriticalSection(&pContext->lckSend);
}

void CIOCPServer::CloseAllConnections()
{
	// �������������б����ر����еĿͻ��׽���
	CTransferChannelEx *pContext = m_lstConnection.GetHead();
	while(pContext != NULL)
	{	
// 		::EnterCriticalSection(&pContext->lckSend);
// 		ASSERT(!pContext->bClosing);
// 		if ( !pContext->bClosing)
// 		{
// 			pContext->bClosing = TRUE;
// 
// 			// Ȼ��رտͻ��׽���
// 			pContext->piTrans->Disconnect(pContext);
// 
// 			ASSERT(pContext->pNext == 0);
// 			m_queFreeContext.Push(pContext);
// 		}
// 		::LeaveCriticalSection(&pContext->lckSend);
		CloseAConnection(pContext);
		pContext = m_lstConnection.GetHead();
	}
}


BOOL CIOCPServer::InsertPendingAccept(ReadWritePacket *pBuffer)
{
	// ��һ��I/O������������뵽m_pPendingAccepts����
	InterlockedIncrement(&m_nIocpOutStandingListen);
	m_quePendingAccepts.Push(pBuffer);
	return TRUE;
}

BOOL CIOCPServer::RemovePendingAccept(ReadWritePacket *pBuffer)
{
	// ����m_pPendingAccepts���������Ƴ�pBuffer��ָ��Ļ���������
	InterlockedDecrement(&m_nIocpOutStandingListen);
	return nullptr != m_quePendingAccepts.PopFind(pBuffer);
}


BOOL CIOCPServer::Start(int nMaxConnections, int nMaxFreeBuffers)
{
	BOOL bResult = FALSE;

	// �������Ƿ��Ѿ�����
	ASSERT(!m_bServerStarted);
	if(m_bServerStarted)
		return FALSE;

	// �����û�����
	m_nMaxConnections = nMaxConnections;
	m_nMaxFreeBuffers = nMaxFreeBuffers;

	// ��ʼ��״̬����
	m_bShutDown = FALSE;
	m_bServerStarted = TRUE;

	// ������ɶ˿ڶ���
	m_hCompletion = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	if ( m_hCompletion == NULL)
		return FALSE;

	HRESULT hr = E_FAIL;
	for ( DWORD i=0; i< m_TransCount; i++)
	{
		
		if ( m_TS[i])
		{
			if ( SUCCEEDED(m_TS[i]->InitializeCompletePort(dynamic_cast<ICTEPTransferProtocolCallBack*>(this))))
			{
				hr = S_OK;
				if ( i == 0) //TCP
				{
					// ע��FD_ACCEPT�¼���
					// ���Ͷ�ݵ�AcceptEx I/O�������̻߳���յ�FD_ACCEPT�����¼���˵��Ӧ��Ͷ�ݸ����AcceptEx I/O
					if ( SOCKET_ERROR == WSAEventSelect(m_TS[0]->GetListenSocket(), m_hAcceptEvent, FD_ACCEPT))
						hr = S_FALSE;	// �о���,��ȷ����
				}
			}
			else
			{
				m_TS[i]->Final();
				m_TS[i] = NULL;
			}
		}
	}
	if ( FAILED(hr))
	{
		m_log.FmtError(5, L"Server initialize failed. Error code : 0x%08x", hr);
		goto End;
	}

	if ( !OnStart())
	{
		m_log.FmtError(5, L"OnStart initialize failed. Error code : 0x%08x", hr);
#ifndef _DEBUG
		goto End;
#endif // _DEBUG
	}

	ASSERT(m_hListenThread == NULL);
	m_hListenThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
	if ( !m_hListenThread)
	{
		m_hListenThread = OpenThread(THREAD_SET_CONTEXT, FALSE, GetCurrentThreadId());
	}
	ASSERT(m_hListenThread);

	ASSERT(m_hSyncWorkThread == NULL);
	//m_hSyncWorkThread = (HANDLE)_beginthread(_SyncWorkerThreadProc, 0, this);
	m_hSyncWorkThread = CreateThread(0, 0, _SyncWorkerThreadProc, this, 0, 0);
	ASSERT(m_hSyncWorkThread);
	

	if ( _threadListen() >= 0)
		bResult = TRUE;

End:
	m_bServerStarted = FALSE;

	ASSERT(m_nIocpOutStandingSend == 0);
	ASSERT(m_nIocpOutStandingRecv == 0);
	ASSERT(m_nIocpOutStandingListen == 0);

	return bResult;
}

void CIOCPServer::Shutdown()
{
	if( !m_bServerStarted)
		return;

	// ֪ͨ�����̣߳�����ֹͣ����
	m_bShutDown = TRUE;
	::SetEvent(m_hAcceptEvent);
}

DWORD CIOCPServer::_threadListen()
{
	// ���ڼ����׽�����Ͷ�ݼ���Accept I/O
	HRESULT hr = E_FAIL;
	for (DWORD i=0; i< m_TransCount; i++)
	{
		if ( m_TS[i])
		{
			if ( SUCCEEDED(m_TS[i]->PostListen(true)))
			{
				hr = S_OK;
			}
			else
			{
				m_TS[i]->Final();
				m_TS[i] = nullptr;
			}
		}
	}

	if ( FAILED(hr))
	{
		return (DWORD)-1;
	}

	// �����¼��������飬�Ա����������WSAWaitForMultipleEvents����
	HANDLE hWaitEvents[2 + MAX_THREAD];
	int nEventCount = 0;
	hWaitEvents[nEventCount ++] = m_hAcceptEvent;
	hWaitEvents[nEventCount ++] = m_hRepostEvent;

	// ����ָ�������Ĺ����߳�����ɶ˿��ϴ���I/O
	for(ULONG i=0; i<m_dwWorkThreadCount; i++)
	{
		//hWaitEvents[nEventCount ++] = (HANDLE)_beginthread(_WorkerThreadProc, 0, this);
		hWaitEvents[nEventCount ++] = ::CreateThread(NULL, 0, _WorkerThreadProc, this, 0, NULL);
	}

	// �����������ѭ���������¼����������е��¼�

	int nIndex = WSA_WAIT_TIMEOUT;
	while( !m_bShutDown && nIndex != WSA_WAIT_FAILED)
	{
#ifdef _DEBUG
#	define DEF_WAIT_TIMEOUT		15*1000
#else
#	define DEF_WAIT_TIMEOUT		60*1000
#endif // _DEBUG
		nIndex = ::WSAWaitForMultipleEvents(nEventCount, hWaitEvents, FALSE, DEF_WAIT_TIMEOUT, TRUE);
		//nIndex = ::MsgWaitForMultipleObjects(nEventCount, hWaitEvents, FALSE, 60*1000, 0);

		// ����Ƿ�Ҫֹͣ����
		if( m_bShutDown || nIndex == WSA_WAIT_FAILED)
			break;

		if ( nIndex ==  WSA_WAIT_IO_COMPLETION)	// 0)APC����ִ��
		{
			continue;
		}
		else if( nIndex == WSA_WAIT_TIMEOUT)// 1����ʱ�������δ���ص�AcceptEx I/O�����ӽ����˶೤ʱ��
		{
			m_quePendingAccepts.Lock();
			ReadWritePacket* pPacket = m_quePendingAccepts.GetHead();
			while( pPacket != NULL)
			{
				long nSeconds = pPacket->piTrans->GetDuration(pPacket);

				// �������2���ӿͻ��������ͳ�ʼ���ݣ���������ͻ�go away
				if(nSeconds != -1 && (nSeconds > 2*60 || nSeconds < 0))
				{
					pPacket->piTrans->Disconnect(0, pPacket);
				}

				pPacket = pPacket->pNext;
			}
			m_quePendingAccepts.Unlock();
		}
		else
		{
			nIndex = nIndex - WAIT_OBJECT_0;
			int nLimit = 0;
			if(nIndex == 0)			// 2��m_hAcceptEvent�¼��������ţ�˵��Ͷ�ݵ�Accept���󲻹�����Ҫ����
			{
				if ( m_TS[0])
				{
					WSANETWORKEVENTS ne;
					::WSAEnumNetworkEvents(m_TS[0]->GetListenSocket(), hWaitEvents[nIndex], &ne);
					if(ne.lNetworkEvents & FD_ACCEPT)
					{
						nLimit = 50;  // ���ӵĸ�����������Ϊ50��
					}
				}
				else
				{
					ASSERT(0);
					ResetEvent(m_hAcceptEvent);
					nLimit = 50;
				}
			}
			else if(nIndex == 1)	// 3��m_hRepostEvent�¼��������ţ�˵������I/O���߳̽��ܵ��µĿͻ�
			{
				nLimit = InterlockedExchange(&m_nRepostCount, 0);
			}
			else if(nIndex > 1)		// I/O�����߳��˳���˵���д��������رշ�����
			{
				m_bShutDown = TRUE;
				continue;
			}

			// Ͷ��nLimit��AcceptEx I/O����
			int i = 0;
			while( i++ < nLimit && m_quePendingAccepts.GetCount() < m_nMaxAccepts)
			{
				if ( m_TS[0])
				{
					m_TS[0]->PostListen();
				}
			}
		}
	}

	m_bShutDown = FALSE;

	// �ر�����Ӧ��
	OnShutdown();
	
	// �ر���������
	CloseAllConnections();

	// �ر��߳�
	CloseHandle(m_hListenThread);
	m_hListenThread = nullptr;

	SetEvent(m_queSyncRead.m_hEvent);
	CloseHandle(m_hSyncWorkThread);
	m_hSyncWorkThread = nullptr;

	DWORD dwWait = WAIT_IO_COMPLETION ;
	while (dwWait == WAIT_IO_COMPLETION )
		dwWait = ::SleepEx(50, TRUE);		// ��I/O�����߳�һ��ִ�еĻ���

	// �رռ����׽���
	m_quePendingAccepts.Lock();
	ReadWritePacket* pPacket = m_quePendingAccepts.GetHead();
	while( pPacket != NULL)
	{
		pPacket->piTrans->Disconnect(0, pPacket);
		pPacket = pPacket->pNext;
	}
	m_quePendingAccepts.Unlock();

	dwWait = WAIT_IO_COMPLETION ;
	while (dwWait == WAIT_IO_COMPLETION )
		dwWait = ::SleepEx(50, TRUE);		// ��I/O�����߳�һ��ִ�еĻ���

	for (DWORD i=0;i<m_TransCount;i++)
	{
		if ( m_TS[i])
		{
			m_TS[i]->Final();
		}
	}

	dwWait = WAIT_IO_COMPLETION ;
	while (dwWait == WAIT_IO_COMPLETION )
		dwWait = ::SleepEx(500, TRUE);		// ��I/O�����߳�һ��ִ�еĻ���

	ASSERT(m_nIocpOutStandingListen == 0
		&& m_nIocpOutStandingRecv == 0
		&& m_nIocpOutStandingSend == 0);

	// ֪ͨ����I/O�����߳��˳�
	for(ULONG i=0; i<m_dwWorkThreadCount; i++)
	{
		::PostQueuedCompletionStatus(m_hCompletion, (DWORD)-1, 0, NULL);
	}

	// �ȴ�I/O�����߳��˳�
	::WaitForMultipleObjects(m_dwWorkThreadCount, &hWaitEvents[2], TRUE, 5*1000);

	for(ULONG i=2; i<m_dwWorkThreadCount + 2; i++)
	{	
		::CloseHandle(hWaitEvents[i]);
	}

	::CloseHandle(m_hCompletion);

	FreeBuffers();
	FreeContexts();

	return 0;
}

DWORD CIOCPServer::_threadSyncWorker()
{
#ifdef _DEBUG
	m_log.wprint(L"SyncWorkerThread ����...");
#endif // _DEBUG

	CTransferChannelEx* pContext = nullptr;
	DWORD dwWaitTime = 1;

	while ( !m_bShutDown)
	{
		pContext = m_queSyncRead.PopAndWait(dwWaitTime);
		if ( !pContext)
		{
			dwWaitTime *= 2;
			if (dwWaitTime > 1000)
				dwWaitTime = 1000;
			continue;
		}

		BOOL bNeedSleep = TRUE;
		DWORD dwCount = m_queSyncRead.GetCount();
		DWORD i = 0;

		do 
		{
			ReadWritePacket* pPacket = AllocateBuffer(pContext, OP_IocpRecv);
			ASSERT(pPacket && pContext->type == SyncMain);
			HRESULT hr = pContext->piTrans->PostRecv(pContext, pPacket);
			if ( S_OK == hr
				 && pPacket->ol.InternalHigh == 0)
			{
				ReleaseBuffer(pPacket);
				m_queSyncRead.PushAndSigned(pContext);

				if (  ++i >= dwCount)
					break;
				pContext = m_queSyncRead.PopAndWait(0);
				continue;
			}

#ifdef _DEBUG
			if ( SUCCEEDED(hr))
			{
				ASSERT(pPacket->ol.InternalHigh > 0);
				CTEPPacket_Header* pHeader = (CTEPPacket_Header*)pPacket->buff.buff;
				ASSERT(pPacket->buff.buff >= pContext->rbufPacket.m_pBuffer
					&& pPacket->buff.buff < pContext->rbufPacket.m_pBuffer + pContext->rbufPacket.m_dwBufferLength);
				ASSERT(pPacket->ol.InternalHigh <= CTEP_DEFAULT_BUFFER_SIZE);
				if ( pPacket->ol.InternalHigh > 7 && pPacket->buff.buff != pContext->rbufPacket.m_pBuffer)
				{
					unsigned char *pStart = (unsigned char *)pPacket->buff.buff - 10 + 2;
					for(unsigned char *p = pStart; p < (unsigned char *)pPacket->buff.buff + 2; p++)
					{
						ASSERT((UCHAR)(p[1] - p[0]) == 1);
					}
				}
				else if (pPacket->buff.buff == pContext->rbufPacket.m_pBuffer)
				{
					ASSERT(pHeader->magic == 'E');
					ASSERT(pHeader->PacketLength <= CTEP_DEFAULT_BUFFER_SIZE);
				}
				else
				{
					ASSERT(pPacket->buff.buff[pPacket->ol.InternalHigh-1] == 0);
				}

				if ( pContext->pParam)
					free(pContext->pParam);

				pContext->pParam = malloc(pPacket->ol.InternalHigh);
				memcpy(pContext->pParam, pPacket->buff.buff, pPacket->ol.InternalHigh);
			}
			else
			{
				ASSERT(pPacket->ol.InternalHigh == 0);
			}
#endif // _DEBUG

// 			pPacket->buff.size = pPacket->ol.InternalHigh;
// 			pPacket->ol.InternalHigh = 0;
// 			int bResult = pContext->PreSplitPacket(pPacket);
// 			if ( bResult == 0)
// 			{
// 				ReleaseBuffer(pPacket);
// 				m_queSyncRead.PushAndSigned(pContext);
// 
// 				if (  ++i >= dwCount)
// 					break;
// 				pContext = m_queSyncRead.PopAndWait(0);
// 				continue;
// 			}

			ASSERT(pContext);
			bNeedSleep = FALSE;
// 			if ( bResult > 0)
// 			{
// 				pPacket->opType = OP_PacketRecv;
// 			}
			BOOL bRet = PostQueuedCompletionStatus(m_hCompletion, pPacket->ol.InternalHigh
				, (ULONG_PTR)pContext, &pPacket->ol);
			ASSERT(bRet);
			if ( !bRet)
			{
				ReleaseBuffer(pPacket);
				CloseAConnection(pContext);
			}
			pContext = nullptr;

			if (  ++i >= dwCount)
				break;

			pContext = m_queSyncRead.PopAndWait(0);
		}
		while ( pContext);

		if ( bNeedSleep)
		{
			dwWaitTime *= 2;
			if (dwWaitTime > 1000)
				dwWaitTime = 1000;
			WaitForSingleObjectEx(m_queSyncRead.m_hEvent, dwWaitTime, true);
		}
		else
		{
			dwWaitTime = 1;
		}
	}

	return 0;
}

DWORD CIOCPServer::_threadIocpWorker()
{
#ifdef _DEBUG
	m_log.wprint(L"	WorkerThread ����...");
#endif // _DEBUG
	ReadWritePacket *pBuffer;
	DWORD dwTrans;
	LPOVERLAPPED lpol;
	ULONG_PTR dwKey;
	while( !m_bShutDown)
	{
		BOOL bOK = ::GetQueuedCompletionStatus(m_hCompletion, &dwTrans, &dwKey, &lpol, WSA_INFINITE);

		if( dwTrans == (DWORD)-1 && lpol == 0 && dwKey == 0) // �û�֪ͨ�˳�
		{
#ifdef _DEBUG
			ASSERT(bOK);
			m_log.wprint(L"	WorkerThread �˳� \n");
#endif // _DEBUG

			::PostQueuedCompletionStatus(m_hCompletion, (DWORD)-1, 0, NULL);

			DWORD dwWait = WAIT_IO_COMPLETION ;
			while (dwWait == WAIT_IO_COMPLETION )
				dwWait = ::SleepEx(0, TRUE);		// ��I/O�����߳�һ��ִ�еĻ���

			return 0;
		}

		CTransferChannelEx* pContext = (CTransferChannelEx*)dwKey;
		pBuffer = CONTAINING_RECORD(lpol, ReadWritePacket, ol);
		int nError = NO_ERROR;

		ASSERT(dwTrans == pBuffer->ol.InternalHigh || dwTrans == 0);
		if ( pBuffer->piTrans == m_TS[0] || pBuffer->piTrans == m_TS[1])	// TCP | UDP
		{
			DWORD dwFlags = 0;
			SOCKET s;
			if ( pContext)
			{
				s = pContext->s;
				ASSERT(pBuffer->opType == EmPacketOperationType::OP_IocpRecv || pBuffer->opType == EmPacketOperationType::OP_IocpSend);
			}
			else
			{
				ASSERT(pBuffer->opType == EmPacketOperationType::OP_Listen);
				s = pBuffer->piTrans->GetListenSocket();
			}

			bOK = WSAGetOverlappedResult(s, &pBuffer->wsaol, &dwTrans, FALSE, &dwFlags);
			if ( !bOK)
			{
				nError = WSAGetLastError();
			}
		}
		else if ( pContext && pContext->type != SyncMain)	// ����Iocp����
		{
			bOK = GetOverlappedResult(pContext->hFile, &pBuffer->ol, &dwTrans, FALSE);
			if ( !bOK)
			{
				nError = GetLastError();
			}
		}
		else	// ͬ������, RDP/ICA
		{
			dwTrans = pBuffer->ol.InternalHigh;
			nError = pBuffer->ol.Internal;
			if ( nError != 0)
			{
				dwTrans = 0;
				bOK = FALSE;
			}
		}

		if ( !bOK || nError != 0)
		{
			ASSERT(!bOK && nError != 0);
			m_log.FmtWarning(3, L"�ڴ��׽������д�����(%x)! pContext:0x%08x dwTrans:%d ErrCode:%d"
				,pBuffer->hFile, pContext, dwTrans, nError);
			ASSERT(nError !=  WSA_IO_INCOMPLETE && nError != ERROR_IO_PENDING);
			if ( nError == 0)
				nError = -1;
			dwTrans = 0;
		}


		if ( pBuffer->opType == EmPacketOperationType::OP_Listen)
		{
			ASSERT(pContext == NULL);
			if ( pBuffer->piTrans == m_TS[0])	// �����TCP
			{
				ASSERT(nError != 0 || dwTrans != 0);
			}
		}

		HandleIO(pContext, pBuffer, dwTrans, nError);
	}

#ifdef _DEBUG
	m_log.wprint(L"	WorkerThread �˳� \n");
#endif // _DEBUG
	return 0;
}

void CIOCPServer::HandleIO(CTransferChannelEx *pContext, ReadWritePacket *pBuffer, DWORD dwTrans, int nError)
{
	EmPacketOperationType ePacketType = pBuffer->opType;
	if ( pContext)
	{
		if ( pContext->type != SyncMain)
		{
			if( ePacketType == EmPacketOperationType::OP_IocpRecv)
			{
				InterlockedDecrement(&m_nIocpOutStandingRecv);
			}
			else if( ePacketType == EmPacketOperationType::OP_IocpSend)
			{
				InterlockedDecrement(&pContext->nOutstandingSend);

#if !CTEP_DEBUG_SENDPACKET_LEVEL_OFF
				InterlockedDecrement(&m_nIocpOutStandingSend);
#endif
			}
		}

		if( pContext->bClosing)
		{
			m_log.FmtMessage(1, L"	��鵽�׽����Ѿ������ǹر� \n");

			ReleaseBuffer(pBuffer);
			return;
		}
	}
	else
	{
#ifdef _DEBUG
		ASSERT( pBuffer->opType == OP_Listen && pBuffer->buff.size == 0);
		ASSERT( dwTrans == sizeof(CTEPPacket_Init) || dwTrans == 0);
#endif // _DEBUG
		RemovePendingAccept(pBuffer);// Listen����, ���ȴ��б�
#ifdef _DEBUG
		ASSERT( pBuffer->opType == OP_Listen && pBuffer->buff.size == 0);
		ASSERT( dwTrans == sizeof(CTEPPacket_Init) || dwTrans == 0);
#endif // _DEBUG
	}

	// ����׽����Ϸ����Ĵ�������еĻ���֪ͨ�û���Ȼ��ر��׽���
	if( nError != NO_ERROR)
	{
		if(pBuffer->opType != EmPacketOperationType::OP_Listen)
		{
			if ( InterlockedExchange((volatile UINT*)&pContext->bNeedNotificationClosing, FALSE))
			{
				if ( !OnConnectionError(pContext, pBuffer, nError))
					OnConnectionClosing(pContext, pBuffer);
			}

			CloseAConnection(pContext);
		}
		else
		{ // �ڼ����׽����Ϸ�������Ҳ���Ǽ����׽��ִ����Ŀͻ�������
			if ( pBuffer->piTrans)
			{
				pBuffer->piTrans->Disconnect(0, pBuffer);
			}
		}

		ReleaseBuffer(pBuffer);
		return;
	}

	// ��ʼ����
#ifdef _DEBUG
	ASSERT(dwTrans == 0 || pBuffer->buff.size == 0 || dwTrans == pBuffer->buff.size);
#endif // _DEBUG
	
	pBuffer->buff.size = dwTrans;
	if ( pBuffer->opType == EmPacketOperationType::OP_Listen)
	{
		ASSERT(pBuffer->hFile != INVALID_HANDLE_VALUE);
		if( pBuffer->hFile != INVALID_HANDLE_VALUE)
		{
			// Ϊ�½��ܵ���������ͻ������Ķ���
			CTransferChannelEx *pClient = AllocateContext(pBuffer->hFile, pBuffer->piTrans);
			BOOL bSuccess = FALSE;
			if(pClient)
			{
				if( AddAConnection(pClient))
				{
					do 
					{
						//SetFileCompletionNotificationModes(pBuffer->hFile, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS|FILE_SKIP_SET_EVENT_ON_HANDLE);
						pClient->piTrans->CompleteListen(pClient, pBuffer);
						if ( pClient->type == TCP || pClient->type == UDP || pClient->type == IocpMain)
						{
							// ���������ӵ���ɶ˿ڶ���
							HANDLE hIocp = ::CreateIoCompletionPort(pClient->hFile, m_hCompletion, (DWORD)pClient, 0);
							if ( hIocp != m_hCompletion)
							{
								ASSERT(hIocp == m_hCompletion);
								break;
							}
						}

						// ֪ͨ�û�
						pClient->bNeedNotificationClosing = TRUE;
						OnConnectionEstablished(pClient, pBuffer);

						// ��������Ͷ��1��Read������Щ�ռ����׽��ֹرջ����ʱ�ͷ�
						if ( pClient->type == SyncMain)
						{
							m_queSyncRead.PushAndSigned(pClient);
							bSuccess = TRUE;
						}
						else
						{
							ASSERT(pClient->type == TCP || pClient->type == UDP);
							ReadWritePacket* pRecv = AllocateBuffer(pClient, OP_IocpRecv);
							HRESULT hr = pClient->piTrans->PostRecv(pClient, pRecv);
							if ( pRecv && (hr == S_OK || hr == ERROR_IO_PENDING))
							{
								InterlockedIncrement(&m_nIocpOutStandingRecv);
								bSuccess = TRUE;
							}
							else
							{
								ASSERT(0);
							}
						}

					} while (0);
				}
				else
				{
					ASSERT(0);
				}
				
				if ( !bSuccess)
				{
					if ( InterlockedExchange((volatile UINT*)&pClient->bNeedNotificationClosing, FALSE))
					{
						OnConnectionClosing(pClient, 0);
					}
					CloseAConnection(pClient);
				}
			}
			else
			{
				// ��Դ���㣬�ر���ͻ������Ӽ���
				pBuffer->piTrans->Disconnect(0, pBuffer);
			}
		}

		// Accept������ɣ��ͷ�I/O������
		ReleaseBuffer(pBuffer);	

		// ֪ͨ�����̼߳�����Ͷ��һ��Accept����
		::InterlockedIncrement(&m_nRepostCount);
		::SetEvent(m_hRepostEvent);
	}
	else if ( pBuffer->opType == EmPacketOperationType::OP_PacketRecv)
	{
		ASSERT( pContext->type == SyncMain);
		OnReadCompleted(pContext, pBuffer);// ֪ͨ�û�
		ReleaseBuffer(pBuffer);
	}
	else if(pBuffer->opType == EmPacketOperationType::OP_IocpRecv)
	{
		if(dwTrans == 0)	// �Է��ر��׽���
		{
			// ��֪ͨ�û�
			if ( InterlockedExchange((volatile UINT*)&pContext->bNeedNotificationClosing, FALSE))
			{
				OnConnectionClosing(pContext, pBuffer);
			}

			// �ٹر�����
			CloseAConnection(pContext);
			ReleaseBuffer(pBuffer);	
		}
		else
		{
			OnReadCompleted(pContext, pBuffer);// ֪ͨ�û�
			ReleaseBuffer(pBuffer);
		}
	}
	else if(pBuffer->opType == EmPacketOperationType::OP_IocpSend)
	{

		if(dwTrans == 0)	// �Է��ر��׽���
		{
			// ��֪ͨ�û�
			if ( InterlockedExchange((volatile UINT*)&pContext->bNeedNotificationClosing, FALSE))
			{
				OnConnectionClosing(pContext, pBuffer);
			}

			// �ٹر�����
			CloseAConnection(pContext);
			ReleaseBuffer(pBuffer);	
		}
		else
		{
			// д������ɣ�֪ͨ�û�
			OnWriteCompleted(pContext, pBuffer);
			// �ͷ�SendText��������Ļ�����
			ReleaseBuffer(pBuffer);
		}
	}

	if ( pContext && !pContext->bClosing && ePacketType == EmPacketOperationType::OP_IocpRecv)
	{
		// ����Ͷ��һ���µĽ�������
		if ( pContext->type == SyncMain)
		{
			m_queSyncRead.PushAndSigned(pContext);
		}
		else
		{
			ASSERT(pContext->type == TCP || pContext->type == UDP);
			ReadWritePacket* pRecv = AllocateBuffer(pContext, OP_IocpRecv);
			HRESULT hr = pContext->piTrans->PostRecv(pContext, pRecv);
			if ( pRecv && (hr == S_OK || hr == ERROR_IO_PENDING))
			{
				InterlockedIncrement(&m_nIocpOutStandingRecv);
			}
			else
			{
				CloseAConnection(pContext);
			}
		}
	}
}

// BOOL CIOCPServer::SendText(StTransferChannelEx *pContext, char *pszText, int nLen, EmPacketLevel level)
// {
// 	// ����Ͷ�ݵķ��͵���������ֹ�û����������ݶ������գ����·������׳��������Ͳ���
// 	if(pContext->nOutstandingSend > m_nMaxSends)
// 	{
// 		m_log.FmtWarningW(3, L"PostSend overflow! %d > max(%d)"
// 			, pContext->nOutstandingSend, m_nMaxSends);
// 		ASSERT(0);
// 		return FALSE;
// 	}
// 
// 	ReadWritePacket *pBuffer = AllocateBuffer((StTransferChannel*)pContext, OP_IocpSend, nLen);
// 	if ( !pBuffer)
// 		return FALSE;
// 
// 	memcpy(pBuffer->buff.buff, pszText, nLen);
// 
// 	if ( m_mqueSendPriority.Push(pBuffer, level))
// 		return TRUE;
// 
// 	return sendMQUEPacket(pBuffer);
// }

BOOL CIOCPServer::SendPacket(CTransferChannelEx *pContext, ReadWritePacket* pBuffer, EmPacketLevel level)
{
	BOOL bResult = TRUE;
	if ( !pContext)
	{
		ReleaseBuffer(pBuffer);
		return FALSE;
	}

	// ����Ͷ�ݵķ��͵���������ֹ�û����������ݶ������գ����·������׳��������Ͳ���
	int nOutstandingSend = pContext->nOutstandingSend;
	if ( nOutstandingSend > 500)
	{
		SleepEx(0, TRUE);
	}

	if( nOutstandingSend > m_nMaxSends)
	{
		m_log.FmtErrorW(3, L"PostSend overflow! %d > max(%d)", nOutstandingSend, m_nMaxSends);
	}
	
	::EnterCriticalSection(&pContext->lckSend);
#if !CTEP_DEBUG_SENDPACKET_LEVEL_OFF
	if ( m_mqueSendPriority.Push(pBuffer, level))
	{
		::LeaveCriticalSection(&pContext->lckSend);
		return TRUE;
	}
#endif

	bResult = sendMQUEPacket(pBuffer);
	::LeaveCriticalSection(&pContext->lckSend);

	return bResult;
}

BOOL CIOCPServer::sendMQUEPacket(ReadWritePacket* pSend)
{
	HRESULT hr;
	CTransferChannelEx *pCtxSend = (CTransferChannelEx*)pSend->pTransChn;
	ASSERT(pCtxSend && pSend->opType == EmPacketOperationType::OP_IocpSend);
	if ( !pCtxSend || pCtxSend->bClosing)
	{
		ReleaseBuffer(pSend);
		return FALSE;
	}

	if ( pCtxSend->type != SyncMain)
	{
		// �׽����ϵ�δ��I/O����
		DWORD dwOutstandingSend = InterlockedIncrement(&pCtxSend->nOutstandingSend);
#if !CTEP_DEBUG_SENDPACKET_LEVEL_OFF
		DWORD dwIocpOsSend = InterlockedIncrement(&m_nIocpOutStandingSend);
		ASSERT ( dwIocpOsSend < MQUE_MAX_SEND + 15 && dwOutstandingSend < MQUE_MAX_SEND + 15);
#endif
	}

	::EnterCriticalSection(&pCtxSend->lckSend);
	DWORD SequenceIdLocal = InterlockedIncrement(&pCtxSend->SequenceIdLocal);
	((CTEPPacket_Header*)pSend->buff.buff)->SequenceId = SequenceIdLocal;
	hr = pCtxSend->SendPacketServer(pSend);
	::LeaveCriticalSection(&pCtxSend->lckSend);

	if ( S_OK == hr || hr == ERROR_IO_PENDING)
	{
		if ( pCtxSend->type != SyncMain)
		{
			OnWriteCompleted(0, 0);
		}
		else
		{
			ReleaseBuffer(pSend);
		}

		return TRUE;
	}

	if ( pCtxSend->type != SyncMain)
	{
		// �׽����ϵ�δ��I/O����
#if !CTEP_DEBUG_SENDPACKET_LEVEL_OFF
		InterlockedDecrement(&m_nIocpOutStandingSend);
#endif
		InterlockedDecrement(&pCtxSend->nOutstandingSend);
	}

	m_log.FmtWarning(5, L"sendMQUEPacket-PostSend Failed. SendSize:(%d) hr:0x%08x(%d)"
		, pSend->buff.size, hr, hr);
	CloseAConnection(pCtxSend);
	ReleaseBuffer(pSend);

	return FALSE;
}


void CIOCPServer::OnWriteCompleted(CTransferChannelEx * /*pContext*/, ReadWritePacket * /*pBuffer*/)
{
	if ( m_mqueSendPriority.IsUsed())
	{
		if ( m_nIocpOutStandingSend < MQUE_MIN_SEND)
		{
			if ( m_mqueSPLock.TryLock())
			{
				m_log.FmtMessage(5, L"WriteCompleted-MQue-Pop. IocpOsSend:%d QueueCount:%d"
					, m_nIocpOutStandingSend, m_mqueSendPriority.GetCount());
				ReadWritePacket* pSend = m_mqueSendPriority.Pop();
				while ( pSend)
				{
					sendMQUEPacket(pSend);
					if ( m_nIocpOutStandingSend > MQUE_MAX_SEND)
						break;

					pSend = m_mqueSendPriority.Pop();
				}

				if ( !pSend)// ����Ϊ��
				{
					m_mqueSendPriority.Lock();
					pSend = m_mqueSendPriority.Pop();
					while(pSend)
					{
						sendMQUEPacket(pSend);
						pSend = m_mqueSendPriority.Pop();
					}

					ASSERT(m_mqueSendPriority.GetCount() == 0);
#ifdef _DEBUG
					m_log.FmtMessageW(5, L"OnWriteCompleted CloseMQUE. IocpOsSend:%d"
						, m_nIocpOutStandingSend);
#endif
					m_mqueSendPriority.SetUsed(FALSE);
					m_mqueSendPriority.Unlock();
				}

				m_mqueSPLock.ReleaseLock();
			}
		}
	}
	else if ( m_nIocpOutStandingSend >= MQUE_MAX_SEND)
	{
		m_mqueSendPriority.SetUsed(TRUE);
#ifdef _DEBUG
		m_log.FmtMessageW(5, L"OnWriteCompleted OpenMQUE. IocpOsSend:%d", m_nIocpOutStandingSend);
#endif
	}
}
