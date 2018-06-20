#pragma once

#include <Ws2tcpip.h>
#include "CTEP_Common_struct.h"

enum EnTransferChannelType
{
	TransTypeEmpty = 0,
	TCP,
	UDP,
	OTHER,
};

struct StTransferChannel	// һ���ײ㴫��ͨ��, ��ʾ��һ���û��Ự������
{
	void Init()
	{
		dwSessionId = (DWORD)-1; pParam = 0;
		hFile = INVALID_HANDLE_VALUE;
		ZeroObject(addrLocal);
		ZeroObject(addrRemote);
		piTrans = 0;
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
	

	CRITICAL_SECTION Lock;					// ��������ṹ

	volatile CUserData			*pUser;

	union
	{
		ICTEPTransferProtocolServer *piTrans;
		ICTEPTransferProtocolClient *piTransClt;
	};
	

	EnTransferChannelType type;
};


interface ICTEPTransferProtocolCallBack
{
	//֧����ɶ˿�
	virtual HANDLE GetCompletePort() = 0;	// ������ɶ˿ھ��
	virtual HANDLE GetListenEvent() = 0;	// only TCP
	virtual BOOL InsertPendingAccept(ReadWritePacket *pBuffer) = 0;

	virtual ReadWritePacket* AllocatePacket(ICTEPTransferProtocolServer* pI) = 0;
	virtual void FreePacket(ReadWritePacket* ) = 0;
};

interface ICTEPTransferProtocolServer
{
	virtual LPCSTR GetName() = 0;	// ���ش���Э������
	virtual bool SupportIOCP() = 0;	// �Ƿ�֧����ɶ˿�ģ��
	virtual long GetDuration(ReadWritePacket* pPacket) = 0;		// �ж�һ��Socket���ڵ�ʱ��,ֻ��TCPʵ��,��������-1;
	virtual SOCKET GetListenSocket() = 0; //Only TCP/UDP, ��������INVALID_SOCKET(-1)

	// ֧����ɶ˿�
	virtual HRESULT InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) = 0;
	virtual HRESULT PostListen(bool bFirst = false) = 0;
	virtual HRESULT CompleteListen(StTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	virtual HRESULT PostRecv(StTransferChannel* pTransChn, bool bFirst = false) = 0;
	virtual HRESULT PostSend(StTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;

	// ��֧����ɶ˿ڵ�
	virtual HRESULT Initialize(ICTEPTransferProtocolCallBack* piCallBack) = 0;
	virtual HRESULT Listen(StTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	virtual HRESULT Recv(StTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	virtual HRESULT Send(StTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;

	//������
	virtual HRESULT Disconnect(StTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) = 0;
	virtual void Final() = 0;

	// Tcp/Rdp Only
	virtual USHORT GetPort() = 0;	// ����TCP/RDP�����˿ں�
};



interface ICTEPTransferProtocolClientCallBack
{
	virtual HRESULT Connected(StTransferChannel* pTransChn) = 0;
	virtual HRESULT Disconnected(StTransferChannel* pTransChn, DWORD dwErr) = 0;
	virtual HRESULT Recv(StTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;

// 	virtual ReadWritePacket* AllocatePacket(ICTEPTransferProtocolClient* pI) = 0;
// 	virtual void FreePacket(ReadWritePacket* ) = 0;
};

interface ICTEPTransferProtocolClient	// ������...
{
	//������
	virtual LPCSTR GetName() = 0;	// ���ش���Э������

	virtual HRESULT Initialize(ICTEPTransferProtocolClientCallBack* pI) = 0;
	virtual void Final() = 0;

	virtual HRESULT Connect(StTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) = 0;// RDP����: E_NOTIMPL
	virtual HRESULT Disconnect(StTransferChannel* pTransChn) = 0;// RDP����: E_NOTIMPL

	virtual HRESULT Send(StTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	virtual HRESULT Recv(StTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;	// TCP/UDP, RDP��֧��,����E_NOIMPL
};



