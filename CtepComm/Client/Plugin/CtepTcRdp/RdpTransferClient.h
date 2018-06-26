#include "VirtualChannelManager.h"

class CRdpTransferClient : public CVirtualChannelManager
{
public:
	//������
	CRdpTransferClient(PCHANNEL_ENTRY_POINTS pEntryPoints);
	~CRdpTransferClient();
	virtual LPCSTR GetName() override;	// ���ش���Э������
	//RDP ��ʼ��
	virtual HRESULT Initialize(ICTEPTransferProtocolClientCallBack* pI) override;
	virtual void Final() override;

	virtual HRESULT Connect(CTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) override;
	virtual HRESULT Disconnect(CTransferChannel* pTransChn) override;

	virtual HRESULT Send(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;

public:
	void RecvData(char *pbuff, DWORD size);
private:
	Log4CppLib								m_log;

	CRITICAL_SECTION						cs;
	CTransferChannel*						m_pTransChn;
};



