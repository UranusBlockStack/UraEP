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

	virtual HRESULT Connect(CTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) override;
	virtual HRESULT Disconnect(CTransferChannel* pTransChn) override;// RDP����: E_NOTIMPL

	virtual HRESULT Send(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;	// TCP/UDP, RDP��֧��,����E_NOIMPL


private:
	Log4CppLib m_log;

	USHORT m_uPort;
	ICTEPTransferProtocolClientCallBack* m_piCallBack;

	CTransferChannel* volatile m_pTransChn;

	_LiMB::CMyCriticalSection lckSend;
	_LiMB::CMyCriticalSection lckRecv;
};

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();
