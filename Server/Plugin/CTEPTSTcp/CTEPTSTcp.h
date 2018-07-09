#ifndef __CTEPTSTCP_H__
#define __CTEPTSTCP_H__
#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#pragma comment(lib,"WS2_32.lib")
#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"

#define DEFAULT_PORT              4567
#define MAX_LISTEN_CONNECTION     200
class CTransProTcpSvr : public ICTEPTransferProtocolServer
{
public:
	CTransProTcpSvr();
	~CTransProTcpSvr();

public:
	virtual LPCSTR GetName() override {return "TCP";}	// ���ش���Э������;
	virtual DWORD SupportIOCP() override{return CTEP_TS_SUPPORT_IOCP;}	// �Ƿ�֧����ɶ˿�ģ��
	virtual SOCKET GetListenSocket() override;

	virtual HRESULT InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) override;
	virtual HRESULT PostListen(bool bFirst = false) override;
	
	virtual HRESULT PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;

	virtual long CTransProTcpSvr::GetDuration(ReadWritePacket* pPacket) override;
	virtual HRESULT Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual void Final() override;

	// Tcp/Rdp Only
	virtual USHORT GetPort() override {return m_uPort;}	// ����TCP/RDP�����˿ں�

private:
	Log4CppLib m_log;

	USHORT m_uPort;
	ICTEPTransferProtocolCallBack* m_piCallBack;

	SOCKET m_sListen;
	CRITICAL_SECTION cs;
	LPFN_ACCEPTEX m_lpfnAcceptEx;	// AcceptEx������ַ
	LPFN_GETACCEPTEXSOCKADDRS m_lpfnGetAcceptExSockaddrs; // GetAcceptExSockaddrs������ַ
};

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();

#endif