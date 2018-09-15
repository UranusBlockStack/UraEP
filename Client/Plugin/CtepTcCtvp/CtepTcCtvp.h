#pragma once
#ifndef __CTEPTCCTVP_H__
#define __CTEPTCCTVP_H__

#include "../../CtepCommClient/CtepCommClientExport.h"

#include "CommonInclude/CommonImpl.h"
#include "Interface\CTEP_Communicate_TransferLayer_Interface.h"
#include "VirtualChannelManager.h"

class CTVPTransferClient : public CCTVPVirtualChannelManager
{
public:
	//������
	CTVPTransferClient(PCTVPCHANNEL_ENTRY_POINTS pEntryPoints);
	~CTVPTransferClient();
	_VIRT(LPCSTR) GetName() override;	// ���ش���Э������
	//RDP ��ʼ��
	_VIRT_H Initialize(ICTEPTransferProtocolClientCallBack* pI) override;
	_VIRT_V Final() override;

	_VIRT_H Connect(CTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) override;
	_VIRT_H Disconnect(CTransferChannel* pTransChn) override;

	_VIRT_H Send(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	_VIRT_H Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;

public:
	void RecvData(char *pbuff, DWORD size);
private:
	CRITICAL_SECTION						cs;
	CTransferChannel*						m_pTransChn;
};






#endif

