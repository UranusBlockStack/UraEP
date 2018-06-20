// CTEPTStestTcp.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
/*
#include <winsock2.h>
#include <windows.h>
#include <Mswsock.h>
#pragma comment(lib, "WS2_32.lib")

#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"


class CMyTestTPSvr : public ICTEPTransferProtocolServer
{
	Log4CppLib m_log;
	ICTEPTransferProtocolCallBack* m_piCallBack;

	SOCKET m_sListen;	// �����׽��־��
	LPFN_ACCEPTEX m_lpfnAcceptEx;	// AcceptEx������ַ
	LPFN_GETACCEPTEXSOCKADDRS m_lpfnGetAcceptExSockaddrs; // GetAcceptExSockaddrs������ַ
	CRITICAL_SECTION cs;

public:
	CMyTestTPSvr() : m_piCallBack(0), m_sListen(INVALID_SOCKET)
		, m_lpfnGetAcceptExSockaddrs(NULL), m_lpfnAcceptEx(NULL)
		, m_log("TS_TCP")
	{
		// ��ʼ��WS2_32.dll
		WSADATA wsaData;
		WORD sockVersion = MAKEWORD(2, 2);
		::WSAStartup(sockVersion, &wsaData);
		InitializeCriticalSection(&cs);
	}
	~CMyTestTPSvr()
	{
		DeleteCriticalSection(&cs);
		::WSACleanup();
	}
	LPCSTR GetName() override	// ���ش���Э������
	{
		return "TCP";
	}

	bool SupportIOCP() override
	{return true;}	// �Ƿ�֧����ɶ˿�ģ��


	// ֧����ɶ˿�
	HRESULT InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) override
	{
		HRESULT hr = E_FAIL;
		HANDLE hReturn = NULL;
		GUID GuidAcceptEx = WSAID_ACCEPTEX;
		GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;		
		DWORD dwBytes;

		m_piCallBack = piCallBack;

		// ���������׽��֣��󶨵����ض˿ڣ��������ģʽ
		m_sListen = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		SOCKADDR_IN si;
		si.sin_family = AF_INET;
		si.sin_port = ::ntohs(4567);
		si.sin_addr.S_un.S_addr = INADDR_ANY;
		if( ::bind(m_sListen, (sockaddr*)&si, sizeof(si)) == SOCKET_ERROR)
		{
			DWORD dwErr = WSAGetLastError();
			hr = -1 * dwErr;
			goto End;
		}
		::listen(m_sListen, 200);
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

		// ע��FD_ACCEPT�¼���
		// ���Ͷ�ݵ�AcceptEx I/O�������̻߳���յ�FD_ACCEPT�����¼���˵��Ӧ��Ͷ�ݸ����AcceptEx I/O
		if ( SOCKET_ERROR == WSAEventSelect(m_sListen, piCallBack->GetListenEvent(), FD_ACCEPT))
		{
			hr = S_OK;
		}

End:
		if ( FAILED(hr))
		{
			::closesocket(m_sListen);
			m_sListen = INVALID_SOCKET;
		}

		return hr;
	}

	HRESULT PostListen(bool bFirst = false) override
	{
		if ( INVALID_SOCKET == m_sListen || !m_piCallBack)
		{
			return E_FAIL;
		}

		int nCount = bFirst ? 10 : 1;
		::EnterCriticalSection(&cs);
		for ( int i = 0; i < nCount; i++)
		{
			ReadWritePacket *pBuffer;
			pBuffer = m_piCallBack->AllocatePacket(dynamic_cast<ICTEPTransferProtocolServer*>(this));
			m_piCallBack->InsertPendingAccept(pBuffer);

			// 		// ����I/O����
			pBuffer->opType = EmPacketOperationType::OP_Listen;

			// Ͷ�ݴ��ص�I/O  
			DWORD dwBytes;
			pBuffer->hFile = (HANDLE)::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

			unsigned   long   sendbufsize = 0;
			int size = sizeof(sendbufsize);
			int iRet = getsockopt((SOCKET)pBuffer->hFile, SOL_SOCKET, SO_RCVBUF, (char*)&sendbufsize, &size);
			if ( sendbufsize < 64*1024)
			{
				sendbufsize = 64*1024;
				iRet = setsockopt((SOCKET)pBuffer->hFile, SOL_SOCKET, SO_RCVBUF, (char*)&sendbufsize, size);
			}
			sendbufsize = 0;
			iRet = getsockopt((SOCKET)pBuffer->hFile, SOL_SOCKET, SO_SNDBUF, (char*)&sendbufsize, &size);
			if ( sendbufsize < 64*1024)
			{
				sendbufsize = 64*1024;
				iRet = setsockopt((SOCKET)pBuffer->hFile, SOL_SOCKET, SO_SNDBUF, (char*)&sendbufsize, size);
			}

			BOOL b = m_lpfnAcceptEx((SOCKET)m_sListen, 
				(SOCKET)pBuffer->hFile,
				pBuffer->pBuffer, 
				pBuffer->nMaxLength - ((sizeof(sockaddr_in) + 16) * 2),
				sizeof(sockaddr_in) + 16, 
				sizeof(sockaddr_in) + 16, 
				&dwBytes, 
				&pBuffer->ol);
			if( !b && ::WSAGetLastError() != WSA_IO_PENDING)
			{
				::LeaveCriticalSection(&cs);

				return E_FAIL;
			}
		}

		::LeaveCriticalSection(&cs);

		return S_OK;
	}

	virtual SOCKET GetListenSocket()
	{
		return m_sListen;
	}

	virtual long GetDuration(ReadWritePacket* pPacket) override
	{
		int nSeconds;
		int nLen = sizeof(nSeconds);

		// ȡ�����ӽ�����ʱ��
		::getsockopt((SOCKET)pPacket->hFile, 
			SOL_SOCKET, SO_CONNECT_TIME, (char *)&nSeconds, &nLen);
		return nSeconds;
	}
	HRESULT PostRecv(StTransferChannel* pTransChn, bool bFirst = false) override
	{
		if ( INVALID_SOCKET == m_sListen || !m_piCallBack)
		{
			return E_FAIL;
		}

		HRESULT hr = E_FAIL;
		DWORD dwCount = bFirst ? 3 : 1;

		for(DWORD i=0; i<dwCount; i++)
		{
			ReadWritePacket *pBuffer = m_piCallBack->AllocatePacket(dynamic_cast<ICTEPTransferProtocolServer*>(this));
			if( pBuffer)
			{
				// ����I/O����
				pBuffer->opType = EmPacketOperationType::OP_Recv;

				::EnterCriticalSection(&pTransChn->Lock);

				// �������к�
				pBuffer->nSequenceNumber = pTransChn->nReadSequence;

				// Ͷ�ݴ��ص�I/O
				DWORD dwBytes;
				DWORD dwFlags = 0;
				WSABUF buf;
				buf.buf = pBuffer->pBuffer;
				buf.len = pBuffer->nMaxLength;

				if(::WSARecv((SOCKET)pTransChn->hFile, &buf, 1, &dwBytes, &dwFlags, &pBuffer->ol, NULL) != NO_ERROR)
				{
					DWORD nErr = WSAGetLastError();
					if( nErr != WSA_IO_PENDING)
					{
						::LeaveCriticalSection( &pTransChn->Lock);
						if ( hr == S_OK)
						{
							return S_FALSE;
						}
						else
						{
							return nErr > 0 ? 0 - nErr : -1;
						}
					}
					else
					{
						hr = S_OK;
					}
				}
				else
				{
					hr = S_OK;
				}

				// �����׽����ϵ��ص�I/O�����Ͷ����кż���

				pTransChn->nOutstandingRecv++;
				pTransChn->nReadSequence++;

				::LeaveCriticalSection(&pTransChn->Lock);
			}
		}

		return S_OK;
	}

	virtual HRESULT PostSend(StTransferChannel* pTransChn, ReadWritePacket* pPacket)
	{
		// ����I/O���ͣ������׽����ϵ��ص�I/O����
		pPacket->opType = EmPacketOperationType::OP_Send;

		// Ͷ�ݴ��ص�I/O
		DWORD dwBytes;
		DWORD dwFlags = 0;
		WSABUF buf;
		buf.buf = pPacket->pBuffer;
		buf.len = pPacket->nSize;
		if( NO_ERROR != ::WSASend((SOCKET)pTransChn->hFile, &buf, 1, &dwBytes, dwFlags, &pPacket->ol, NULL))
		{
			DWORD dwErr = ::WSAGetLastError();
			if( dwErr != WSA_IO_PENDING)
			{
				m_log.FmtErrorW(3, L"PostSend failed. ErrCode:%d", dwErr);
				return FALSE;
			}
		}	

		// �����׽����ϵ��ص�I/O����
		::EnterCriticalSection(&pTransChn->Lock);
		DWORD dwOutstandingSend = ++pTransChn->nOutstandingSend;
		::LeaveCriticalSection(&pTransChn->Lock);
#ifdef _DEBUG
		if ( dwOutstandingSend > 50)
		{
			m_log.FmtWarningW(3, L"PostSend OutstandingSend Count:%d", dwOutstandingSend);
		}
#endif // _DEBUG

		return TRUE;
	}

	HRESULT CompleteListen(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override
	{
		// ȡ�ÿͻ���ַ
		int nLocalLen, nRmoteLen;
		LPSOCKADDR pLocalAddr, pRemoteAddr;
		m_lpfnGetAcceptExSockaddrs(
			pPacket->pBuffer,
			pPacket->nMaxLength - ((sizeof(sockaddr_in) + 16) * 2),
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


	// ��֧����ɶ˿ڵ�
	HRESULT Initialize(ICTEPTransferProtocolCallBack* piCallBack) override
	{
		HRESULT hr = S_OK;

		return hr;

	}
	HRESULT Listen(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override
	{
		HRESULT hr = S_OK;

		return hr;

	}
	HRESULT Recv(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override
	{
		HRESULT hr = S_OK;

		return hr;

	}

	//������
	HRESULT Send(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override
	{
		HRESULT hr = S_OK;

		return hr;

	}

	HRESULT Disconnect(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override
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

	void Final() override
	{
		if ( m_piCallBack)
		{
			m_piCallBack = NULL;
		}

		if ( m_sListen != INVALID_SOCKET)
		{
			SOCKET s = m_sListen;
			m_sListen = INVALID_SOCKET;
			::closesocket(s);
		}
	}
}
gOne;


ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer()
{
	return dynamic_cast<ICTEPTransferProtocolServer*>(&gOne);
}












*/