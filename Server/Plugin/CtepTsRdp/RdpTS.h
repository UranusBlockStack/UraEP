#pragma once

#include <Windows.h>
#include <MSWSock.h>
#include "RdpChannelServer.h"
#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"

#define DEFAULT_PORT              4567
#define MAX_LISTEN_CONNECTION     200

class CTransProRdpSvr : public ICTEPTransferProtocolServer
{
public:
	virtual LPCSTR GetName() override {return "RDP";};	// ���ش���Э������;
	virtual DWORD SupportIOCP() override {return CTEP_TS_SUPPORT_SYNC;};	// �Ƿ�֧����ɶ˿�ģ��
	virtual long GetDuration(ReadWritePacket* pPacket) override;		// �ж�һ��Socket���ڵ�ʱ��,ֻ��TCPʵ��,��������-1;
	virtual SOCKET GetListenSocket() override; //Only TCP/UDP, ��������INVALID_SOCKET(-1)
	virtual HRESULT InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) override;
	virtual HRESULT PostListen(bool bFirst = false) override;
	virtual HRESULT CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket)  override;

	virtual HRESULT Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual void Final() override;

	// Tcp/Rdp Only
	virtual USHORT GetPort() override {return 0;}	// ����TCP/RDP�����˿ں�

private:
	Log4CppLib m_log;
	CMyCriticalSection csSvr;
};

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();

