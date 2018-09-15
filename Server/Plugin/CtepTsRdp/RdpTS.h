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
	_VIRT(LPCSTR) GetName() override {return "RDP";};	// ���ش���Э������;
	_VIRT_D SupportIOCP() override {return CTEP_TS_SUPPORT_SYNC;};	// �Ƿ�֧����ɶ˿�ģ��
	_VIRT_L GetDuration(ReadWritePacket* pPacket) override;		// �ж�һ��Socket���ڵ�ʱ��,ֻ��TCPʵ��,��������-1;
	_VIRT(SOCKET) GetListenSocket() override; //Only TCP/UDP, ��������INVALID_SOCKET(-1)
	_VIRT_H InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) override;
	_VIRT_H PostListen(bool bFirst = false) override;
	_VIRT_H CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	_VIRT_H PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	_VIRT_H PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket)  override;

	_VIRT_H Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	_VIRT_V Final() override;

	// Tcp/Rdp Only
	_VIRT(WORD) GetPort() override {return 0;}	// ����TCP/RDP�����˿ں�

private:
	Log4CppLib m_log;
	CMyCriticalSection csSvr;
};

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();

