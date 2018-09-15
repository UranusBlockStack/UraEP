  #pragma once

#include <Ws2tcpip.h>
#include "CTEP_Common_struct.h"

#pragma warning(push)
#pragma warning(disable:4482)	//warning C4482: ʹ���˷Ǳ�׼��չ: �޶�����ʹ����ö�١�EnTransferChannelType��

enum EnTransferChannelType
{
	TransType_Empty = 0,

	TransType_TCP,			//TCP socket����,����Ϊ��ͨ���͸���ͨ��;
	TransType_UDP,			//UDP socket����,ֻ����Ϊ����ͨ��,Ŀǰδʵ��

	TransType_IocpMain,		//֧����ɶ˿����͵���ͨ��,��:CTVP,�ܵ���
	TransType_SyncMain,		//��֧����ɶ˿ڵ���ͨ��,ʹ��ͬ������, ��:ICA,RDP��
	TransType_AsyncMain,	//��֧����ɶ˿ڵ���ͨ��,��֧���첽����

	TransType_CrossApp,		//�������˿���̶�̬Ӧ��ͨ��ר�õײ�ͨ��,��֧��IOCP�������ܵ�ʵ��
};

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
		type = EnTransferChannelType::TransType_Empty;
	}
	READWRITE_DATA DWORD dwSessionId;
	READWRITE_DATA volatile void *pParam;
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
	
	volatile BOOL				bClosing;					// �׽����Ƿ�ر�
	EnTransferChannelType		type;
	CRITICAL_SECTION			lckSend;			// ������

	volatile CUserData			*pUser;
	union
	{
		ICTEPTransferProtocolServer *piTrans;
		ICTEPTransferProtocolClient *piTransClt;
	};

public:
	inline LPWSTR debugEnTransfaerChannelType()
	{
		switch(type)
		{
		case TransType_Empty:
			return L"TransType_Empty";
		case TransType_TCP:
			return L"TransType_TCP";
		case TransType_UDP:
			return L"TransType_UDP";
		case TransType_IocpMain:
			return L"TransType_IocpMain";
		case TransType_SyncMain:
			return L"TransType_SyncMain";
		case TransType_CrossApp:
			return L"TransType_CrossApp";
		}
		return L"EnTransferChannelType??????";
	}
};

// CTEP�ײ㴫��� Server�˻ص��ӿ�
interface ICTEPTransferProtocolCallBack
{
	//֧����ɶ˿�
	_VIRT(HANDLE) GetCompletePort() = 0;	// ������ɶ˿ھ��
	_VIRT_B InsertPendingAccept(ReadWritePacket *pBuffer) = 0;	//����ȴ��û���������,������ֿ�������������
																	//,ʹ���������ʱ����ʵ��ICTEPTransferProtocolServer�ӿڵ�GetDuration����

	_VIRT(ReadWritePacket*) AllocatePacket(ICTEPTransferProtocolServer* pI) = 0;
	_VIRT_V FreePacket(ReadWritePacket* ) = 0;

	// CTEP 2.0
	_VIRT_H RegisterCallBackEvent(StCallEvent Calls[], DWORD dwCallCount) = 0;
	_VIRT_H UnregisterCallBackEvent(StCallEvent Calls[], DWORD dwCallCount) = 0;
};

interface ICTEPTransferProtocolServer
{
	_VIRT(LPCSTR) GetName() = 0;	// ���ش���Э������

#define CTEP_TS_SUPPORT_IOCP		1
#define CTEP_TS_SUPPORT_ASYNC		2
#define CTEP_TS_SUPPORT_SYNC		0
	_VIRT_D SupportIOCP() = 0;	// �Ƿ�֧����ɶ˿�ģ��
	_VIRT_L GetDuration(ReadWritePacket* pPacket) = 0;		// �ж�һ��Socket���ڵ�ʱ��,ֻ��TCPʵ��,��������-1;
	_VIRT(SOCKET) GetListenSocket() = 0; //Only TCP/UDP, ��������INVALID_SOCKET(-1)

	_VIRT_H InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) = 0;
	_VIRT_H PostListen(bool bFirst = false) = 0;
	_VIRT_H CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	_VIRT_H PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	_VIRT_H PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;

	_VIRT_H Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) = 0;
	_VIRT_V Final() = 0;

	// Tcp/Rdp Only
	_VIRT(WORD) GetPort() = 0;	// ����TCP/RDP�����˿ں�
};



interface ICTEPTransferProtocolClientCallBack
{
	_VIRT_H Connected(CTransferChannel* pTransChn) = 0;
	_VIRT_H Disconnected(CTransferChannel* pTransChn, DWORD dwErr) = 0;
	_VIRT_H Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;

	_VIRT(RWPacket*) AllocatePacket(ICTEPTransferProtocolClient* pI) = 0;
	_VIRT_V FreePacket(ReadWritePacket* ) = 0;
};

interface ICTEPTransferProtocolClient
{
	//������
	_VIRT(LPCSTR) GetName() = 0;	// ���ش���Э������

	_VIRT_H Initialize(ICTEPTransferProtocolClientCallBack* pI) = 0;
	_VIRT_V Final() = 0;

	_VIRT_H Connect(CTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) = 0;// RDP����: E_NOTIMPL
	_VIRT_H Disconnect(CTransferChannel* pTransChn) = 0;// RDP����: E_NOTIMPL

	_VIRT_H Send(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	_VIRT_H Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;	// TCP/UDP, RDP��֧��,����E_NOIMPL
};

#pragma warning(pop)

