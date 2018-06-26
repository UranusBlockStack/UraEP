#pragma once

#include <Ws2tcpip.h>
#include "CTEP_Common_struct.h"

enum EnTransferChannelType
{
	TransTypeEmpty = 0,
	TCP,
	UDP,
	IocpMain,
	SyncMain,
};

inline LPWSTR debugEnTransfaerChannelType(EnTransferChannelType type)
{
	switch(type)
	{
	case TransTypeEmpty:
		return L"TransTypeEmpty";
	case TCP:
		return L"TCP";
	case UDP:
		return L"UDP";
	case IocpMain:
		return L"IocpMain";
	case SyncMain:
		return L"SyncMain";

	}
	return L"EnTransferChannelType??????";
}

class __declspec(novtable) CTransferChannel	// һ���ײ㴫��ͨ��, ��ʾ��һ���û��Ự������
{
public:
	CTransferChannel()
	{
		::InitializeCriticalSection(&lckSend);
	}
	~CTransferChannel()
	{
		::DeleteCriticalSection(&lckSend);
	}
	void Init()
	{
		bClosing = TRUE;

		pUser = nullptr;
		piTrans = nullptr;

		dwSessionId = (DWORD)-1; pParam = 0;
		hFile = INVALID_HANDLE_VALUE;
		ZeroObject(addrLocal6);
		ZeroObject(addrRemote6);
		type = EnTransferChannelType::TransTypeEmpty;
	}
	READWRITE_DATA DWORD dwSessionId;
	READWRITE_DATA void *pParam;
	union
	{
		READWRITE_DATA HANDLE hFile;
		READWRITE_DATA SOCKET s;
	};

	union
	{
		READWRITE_DATA SOCKADDR_IN  addrLocal;			// ���ӵı��ص�ַ
		READWRITE_DATA SOCKADDR_IN6 addrLocal6;			// ���ӵı��ص�ַ
	};
	
	union
	{
		READWRITE_DATA SOCKADDR_IN addrRemote;			// ���ӵ�Զ�̵�ַ
		READWRITE_DATA SOCKADDR_IN6 addrRemote6;			// ���ӵ�Զ�̵�ַ
	};
	
	volatile BOOL bClosing;					// �׽����Ƿ�ر�
	EnTransferChannelType type;
	CRITICAL_SECTION lckSend;			// ������

	volatile CUserData			*pUser;
	union
	{
		ICTEPTransferProtocolServer *piTrans;
		ICTEPTransferProtocolClient *piTransClt;
	};
};


interface ICTEPTransferProtocolCallBack
{
	//֧����ɶ˿�
	virtual HANDLE GetCompletePort() = 0;	// ������ɶ˿ھ��
	//virtual HANDLE GetListenEvent() = 0;	// only TCP
	virtual BOOL InsertPendingAccept(ReadWritePacket *pBuffer) = 0;

	virtual ReadWritePacket* AllocatePacket(ICTEPTransferProtocolServer* pI) = 0;
	virtual void FreePacket(ReadWritePacket* ) = 0;
};

interface ICTEPTransferProtocolServer
{
	virtual LPCSTR GetName() = 0;	// ���ش���Э������

#define CTEP_TS_SUPPORT_IOCP		1
#define CTEP_TS_SUPPORT_SYNC		0
	virtual DWORD SupportIOCP() = 0;	// �Ƿ�֧����ɶ˿�ģ��
	virtual long GetDuration(ReadWritePacket* pPacket) = 0;		// �ж�һ��Socket���ڵ�ʱ��,ֻ��TCPʵ��,��������-1;
	virtual SOCKET GetListenSocket() = 0; //Only TCP/UDP, ��������INVALID_SOCKET(-1)

	virtual HRESULT InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) = 0;
	virtual HRESULT PostListen(bool bFirst = false) = 0;
	virtual HRESULT CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	virtual HRESULT PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	virtual HRESULT PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;

	virtual HRESULT Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) = 0;
	virtual void Final() = 0;

	// Tcp/Rdp Only
	virtual USHORT GetPort() = 0;	// ����TCP/RDP�����˿ں�
};



interface ICTEPTransferProtocolClientCallBack
{
	virtual HRESULT Connected(CTransferChannel* pTransChn) = 0;
	virtual HRESULT Disconnected(CTransferChannel* pTransChn, DWORD dwErr) = 0;
	virtual HRESULT Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;

	virtual ReadWritePacket* AllocatePacket(ICTEPTransferProtocolClient* pI) = 0;
	virtual void FreePacket(ReadWritePacket* ) = 0;
};

interface ICTEPTransferProtocolClient	// ������...
{
	//������
	virtual LPCSTR GetName() = 0;	// ���ش���Э������

	virtual HRESULT Initialize(ICTEPTransferProtocolClientCallBack* pI) = 0;
	virtual void Final() = 0;

	virtual HRESULT Connect(CTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) = 0;// RDP����: E_NOTIMPL
	virtual HRESULT Disconnect(CTransferChannel* pTransChn) = 0;// RDP����: E_NOTIMPL

	virtual HRESULT Send(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	virtual HRESULT Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;	// TCP/UDP, RDP��֧��,����E_NOIMPL
};



