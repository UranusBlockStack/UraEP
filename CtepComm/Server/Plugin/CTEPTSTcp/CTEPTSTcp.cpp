// CTEPTStestTcp.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "CTEPTSTcp.h"

CTransProTcpSvr gOne;

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer()
{
	return dynamic_cast<ICTEPTransferProtocolServer*>(&gOne);
}

CTransProTcpSvr::CTransProTcpSvr()
	: m_piCallBack(0),
	m_sListen(INVALID_SOCKET),
	m_lpfnGetAcceptExSockaddrs(NULL),
	m_lpfnAcceptEx(NULL),
	m_log("TS_TCP"),
	m_uPort(DEFAULT_PORT)
{
	// ��ʼ��WS2_32.dll
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	::WSAStartup(sockVersion, &wsaData);
	InitializeCriticalSection(&cs);
}
CTransProTcpSvr::~CTransProTcpSvr()
{
	DeleteCriticalSection(&cs);
	::WSACleanup();
}

// ֧����ɶ˿�
HRESULT CTransProTcpSvr::InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack)
{
	HRESULT hr = E_FAIL;
	HANDLE hReturn = NULL;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;		
	DWORD dwBytes;

	m_piCallBack = piCallBack;

	// ��ȡ�����ļ�
	m_uPort;

	// ���������׽��֣��󶨵����ض˿ڣ��������ģʽ
	m_sListen = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN si;
	si.sin_family = AF_INET;
	si.sin_port = ::ntohs(m_uPort);
	si.sin_addr.S_un.S_addr = INADDR_ANY;
	if( ::bind(m_sListen, (sockaddr*)&si, sizeof(si)) == SOCKET_ERROR)
	{
		long dwErr = (long)WSAGetLastError();
		ASSERT(dwErr>0);
		hr = -1 * dwErr;
		goto End;
	}
	::listen(m_sListen, MAX_LISTEN_CONNECTION);
	// ������չ����AcceptEx

	::WSAIoctl(m_sListen, 
		SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&GuidAcceptEx, 
		sizeof(GuidAcceptEx),
		&m_lpfnAcceptEx, 
		sizeof(m_lpfnAcceptEx), 
		&dwBytes, 
		NULL, 
		NULL);

	// ������չ����GetAcceptExSockaddrs
	::WSAIoctl(m_sListen,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockaddrs,
		sizeof(GuidGetAcceptExSockaddrs),
		&m_lpfnGetAcceptExSockaddrs,
		sizeof(m_lpfnGetAcceptExSockaddrs),
		&dwBytes,
		NULL,
		NULL
		);

	if ( !m_lpfnAcceptEx || !m_lpfnGetAcceptExSockaddrs)
	{
		goto End;
	}

	// �������׽��ֹ�������ɶ˿ڣ�ע�⣬����Ϊ�����ݵ�CompletionKeyΪ0
	hReturn = ::CreateIoCompletionPort((HANDLE)m_sListen
		, piCallBack->GetCompletePort(), (ULONG_PTR)0, 0);
	if ( !hReturn || hReturn != piCallBack->GetCompletePort())
	{
		CloseHandle(hReturn);
		goto End;
	}

	hr = S_OK;

End:
	if ( FAILED(hr))
	{
		::closesocket(m_sListen);
		m_sListen = INVALID_SOCKET;
		if ( m_piCallBack)
		{
			m_piCallBack = NULL;
		}
	}

	return hr;
}

HRESULT CTransProTcpSvr::PostListen(bool bFirst)
{
	if ( INVALID_SOCKET == m_sListen || !m_piCallBack)
	{
		return E_FAIL;
	}

	int nCount = bFirst ? 10 : 1;
	::EnterCriticalSection(&cs);
	for ( int i = 0; i < nCount; i++)
	{
		ReadWritePacket *pPacket;
		pPacket = m_piCallBack->AllocatePacket(dynamic_cast<ICTEPTransferProtocolServer*>(this));
		m_piCallBack->InsertPendingAccept(pPacket);

		// 		// ����I/O����
		pPacket->opType = EmPacketOperationType::OP_Listen;

		// Ͷ�ݴ��ص�I/O  
		DWORD dwBytes;
		pPacket->s = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		unsigned   long   sendbufsize = 0;
		int size = sizeof(sendbufsize);
		int iRet = getsockopt((SOCKET)pPacket->hFile, SOL_SOCKET, SO_RCVBUF, (char*)&sendbufsize, &size);
		if ( sendbufsize < 64*1024)
		{
			sendbufsize = 64*1024;
			iRet = setsockopt((SOCKET)pPacket->hFile, SOL_SOCKET, SO_RCVBUF, (char*)&sendbufsize, size);
		}
		sendbufsize = 0;
		iRet = getsockopt((SOCKET)pPacket->hFile, SOL_SOCKET, SO_SNDBUF, (char*)&sendbufsize, &size);
		if ( sendbufsize < 64*1024)
		{
			sendbufsize = 64*1024;
			iRet = setsockopt((SOCKET)pPacket->hFile, SOL_SOCKET, SO_SNDBUF, (char*)&sendbufsize, size);
		}

		BOOL b = m_lpfnAcceptEx(m_sListen, 
			pPacket->s,
			pPacket->buff.buff, 
			pPacket->buff.maxlength - ((sizeof(sockaddr_in) + 16) * 2),
			sizeof(sockaddr_in) + 16, 
			sizeof(sockaddr_in) + 16, 
			&dwBytes, 
			&pPacket->ol);
		if( !b && ::WSAGetLastError() != WSA_IO_PENDING)
		{
			::closesocket(pPacket->s);
			pPacket->s = INVALID_SOCKET;
			::LeaveCriticalSection(&cs);

			return E_FAIL;
		}
	}

	::LeaveCriticalSection(&cs);

	return S_OK;
}

SOCKET CTransProTcpSvr::GetListenSocket()
{
	return m_sListen;
}

// ����ָ�������Ѿ�������ʱ�䳤��, TCPר��. ���ظ�����ʾʧ��, -1��ʾ��֧�ִ˹���
long CTransProTcpSvr::GetDuration(ReadWritePacket* pPacket)
{
	int nSeconds;
	int nLen = sizeof(nSeconds);

	// ȡ�����ӽ�����ʱ��
	if ( pPacket->s != INVALID_SOCKET)
	{
		::getsockopt(pPacket->s, 
			SOL_SOCKET, SO_CONNECT_TIME, (char *)&nSeconds, &nLen);
	}
	else
	{
		return -2;
	}

	return nSeconds;
}
HRESULT CTransProTcpSvr::PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	if ( !m_piCallBack)
	{
		ASSERT(0);
		return E_FAIL;
	}

	if( !pPacket)
		return E_FAIL;
	// ����I/O����
	pPacket->opType = EmPacketOperationType::OP_IocpRecv;

	// Ͷ�ݴ��ص�I/O
	DWORD dwFlags = 0;
	WSABUF buf = {pPacket->buff.maxlength, pPacket->buff.buff};
	if(::WSARecv(pTransChn->s, &buf, 1, 0, &dwFlags, &pPacket->ol, NULL) != NO_ERROR)
	{
		if(::WSAGetLastError() != WSA_IO_PENDING)
		{
			return E_FAIL;
		}

		return ERROR_IO_PENDING;
	}

	return S_OK;
}

HRESULT CTransProTcpSvr::PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	// ����I/O���ͣ������׽����ϵ��ص�I/O����
#ifdef _DEBUG
	USHORT* pCountPacket = (USHORT*)pPacket->buff.buff;
	ASSERT(*pCountPacket == pPacket->buff.size);
	ASSERT(pPacket->opType == EmPacketOperationType::OP_IocpSend);
#endif // _DEBUG
	pPacket->opType = EmPacketOperationType::OP_IocpSend;

	// Ͷ�ݴ��ص�I/O
	DWORD dwBytes;
	DWORD dwFlags = 0;
	WSABUF buf = {pPacket->buff.size, pPacket->buff.buff};
	ASSERT(buf.buf == pPacket->buff.buff && buf.len == pPacket->buff.size);
	if( NO_ERROR != ::WSASend(pTransChn->s, &buf, 1, &dwBytes, dwFlags, &pPacket->ol, NULL))
	{
		long dwErr = (long)::WSAGetLastError();
		if( dwErr != WSA_IO_PENDING)
		{
			ASSERT(dwErr > 0);
			m_log.FmtErrorW(3, L"PostSend failed. ErrCode:%d", dwErr);
			return 0 - dwErr;
		}
		else
		{
			return ERROR_IO_PENDING;
		}
	}
	return S_OK;
}

HRESULT CTransProTcpSvr::CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	ASSERT(pPacket->opType == EmPacketOperationType::OP_Listen);
	// ȡ�ÿͻ���ַ
	int nLocalLen, nRmoteLen;
	LPSOCKADDR pLocalAddr, pRemoteAddr;

	pTransChn->type = TransType_TCP;

	m_lpfnGetAcceptExSockaddrs(
		pPacket->buff.buff,
		pPacket->buff.maxlength - ((sizeof(sockaddr_in) + 16) * 2),
		sizeof(sockaddr_in) + 16,
		sizeof(sockaddr_in) + 16,
		(SOCKADDR **)&pLocalAddr,
		&nLocalLen,
		(SOCKADDR **)&pRemoteAddr,
		&nRmoteLen);
	memcpy(&pTransChn->addrLocal, pLocalAddr, nLocalLen);
	memcpy(&pTransChn->addrRemote, pRemoteAddr, nRmoteLen);

	return S_OK;

}


HRESULT CTransProTcpSvr::Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	if ( pPacket && pTransChn)
	{
		ASSERT(pPacket->hFile == pPacket->hFile);
	}

	if ( pPacket)
	{
		ASSERT(!pTransChn);
		ASSERT( pPacket->opType == EmPacketOperationType::OP_Listen);
		SOCKET s= (SOCKET)pPacket->hFile;
		pPacket->hFile = INVALID_HANDLE_VALUE;
		::closesocket(s);
	}

	if ( pTransChn)
	{
		ASSERT(!pPacket);
		SOCKET s = (SOCKET)pTransChn->hFile;
		pTransChn->hFile = INVALID_HANDLE_VALUE;
		::closesocket(s);
	}

	if ( !pPacket && !pTransChn)
	{
		ASSERT(0);
		return E_FAIL;
	}

	return S_OK;
}

void CTransProTcpSvr::Final()
{
	if ( m_sListen != INVALID_SOCKET)
	{
		::closesocket(m_sListen);
		m_sListen = INVALID_SOCKET;
	}

	m_piCallBack = NULL;
}












