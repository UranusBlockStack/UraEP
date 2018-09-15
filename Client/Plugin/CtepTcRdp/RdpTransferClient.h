#include "VirtualChannelManager.h"

class CRdpTransferClient : public CVirtualChannelManager
{
public:
	//������
	CRdpTransferClient(PCHANNEL_ENTRY_POINTS pEntryPoints);
	~CRdpTransferClient();
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
	Log4CppLib								m_log;

	CRITICAL_SECTION						cs;
	CTransferChannel*						m_pTransChn;
};



