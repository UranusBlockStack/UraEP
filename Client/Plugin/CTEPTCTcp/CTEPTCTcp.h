#pragma once


#include "CommonInclude/CommonImpl.h"
#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"


class CTransProTcpClt : public ICTEPTransferProtocolClient
{
public:
	CTransProTcpClt();
	~CTransProTcpClt();

public:
	virtual LPCSTR GetName() override {return "TCP";}	// ���ش���Э������;

	virtual HRESULT Initialize(ICTEPTransferProtocolClientCallBack* pI) override;
	virtual void Final() override;

	virtual HRESULT Connect(StTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) override;
	virtual HRESULT Disconnect(StTransferChannel* pTransChn) override;// RDP����: E_NOTIMPL

	virtual HRESULT Send(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT Recv(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override;	// TCP/UDP, RDP��֧��,����E_NOIMPL


private:
	Log4CppLib m_log;

	USHORT m_uPort;
	ICTEPTransferProtocolClientCallBack* m_piCallBack;

	StTransferChannel* volatile m_pTransChn;

	_LiMB::CMyCriticalSection lckSend;
	_LiMB::CMyCriticalSection lckRecv;
};

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();
