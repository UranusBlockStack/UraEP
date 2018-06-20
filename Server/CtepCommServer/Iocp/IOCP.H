
#pragma once
#include "CommonInclude/CommonImpl.h"
#include "CommonInclude/Tools/FastQueue.h"

#include "Interface/CTEP_Communicate_App_Interface.h"
#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"
#include "Interface/CTEP_Trans_Packet_Protocol.h"

#include "Interface/CTEP_Common_Struct_Ex.h"

#include "../LoadModules.h"

#include <winsock2.h>
#include <windows.h>
#include <Mswsock.h>

#include <atlcoll.h>
using namespace ATL;

#define MAX_THREAD	32			// I/O�����̵߳�����


class CIOCPServer : public ICTEPTransferProtocolCallBack
	, public CLoadModules
{
protected:
	static Log4CppLib m_log;

public:
//interface ICTEPTransferProtocolCallBack
	virtual HANDLE GetCompletePort() override {return m_hCompletion;}	// ������ɶ˿ھ��
	virtual HANDLE GetListenEvent() override {return m_hListenThread;}
	//virtual BOOL InsertPendingAccept(ReadWritePacket *pBuffer) = 0;
	virtual ReadWritePacket* AllocatePacket(ICTEPTransferProtocolServer* pI) override 
	{
		ReadWritePacket* pPacket = AllocateBuffer(0, OP_Empty, 0);
		if ( pPacket)
			pPacket->piTrans = pI;
		return pPacket;
	}
	virtual void FreePacket(ReadWritePacket* p) override 
	{
		ReleaseBuffer(p);
	}

public:
	CIOCPServer();
	~CIOCPServer();

	// ��ʼ����
	BOOL Start(int nMaxConnections = 5000, int nMaxFreeBuffers = 2000);
	// ֹͣ����
	void Shutdown();

	// �ر�һ�����Ӻ͹ر���������
	void CloseAConnection(StTransferChannelEx *pContext);
	void CloseAllConnections();

	// ȡ�õ�ǰ����������
	ULONG GetCurrentConnection() { return m_queConnectionList.GetCount(); }

	// ��ָ���ͻ������ı�
	BOOL SendText(StTransferChannelEx *pContext, char *pszText, int nLen, EmPacketLevel level = Middle);
	BOOL SendPacket(StTransferChannelEx *pContext, ReadWritePacket* pBuffer, EmPacketLevel level = Middle);

protected:
	// ������ͷŻ���������
	ReadWritePacket *AllocateBuffer(StTransferChannel* pContext, EmPacketOperationType type, int nLen = 0);
	void ReleaseBuffer(ReadWritePacket *pBuffer);

	// ������ͷ��׽���������
	StTransferChannelEx *AllocateContext(HANDLE s, ICTEPTransferProtocolServer* piTrans);
	//void ReleaseContext(StTransferChannelEx *pContext);// ���ܱ����Ͻ�CloseAConnection

	// �ͷſ��л����������б��Ϳ��������Ķ����б�
	void FreeBuffers();
	void FreeContexts();

	// �������б�������һ������
	BOOL AddAConnection(StTransferChannelEx *pContext);

	// ������Ƴ�δ���Ľ�������
	BOOL InsertPendingAccept(ReadWritePacket *pBuffer);
	BOOL RemovePendingAccept(ReadWritePacket *pBuffer);

	void HandleIO(StTransferChannelEx *pContext, ReadWritePacket *pBuffer, DWORD dwTrans, int nError);
	void OnWriteCompleted(StTransferChannelEx *pContext, ReadWritePacket *pBuffer);// һ�������ϵ�д�������
	BOOL sendMQUEPacket(ReadWritePacket* pSend);


// �¼�֪ͨ����
	// ������һ���µ�����
	virtual void OnConnectionEstablished(StTransferChannelEx *pContext, ReadWritePacket *pBuffer) = 0;
	// һ�����ӹر�
	virtual void OnConnectionClosing(StTransferChannelEx *pContext, ReadWritePacket *pBuffer) = 0;
	// ��һ�������Ϸ����˴���
	virtual BOOL OnConnectionError(StTransferChannelEx * /*pContext*/, ReadWritePacket * /*pBuffer*/, int /*nError*/){return FALSE;}
	// һ�������ϵĶ��������
	virtual void OnReadCompleted(StTransferChannelEx *pContext, ReadWritePacket *pBuffer) = 0;
	// ����������
	virtual BOOL OnStart() = 0;
	// �������ر�
	virtual void OnShutdown() = 0;

protected:
	// ͨ�����ͷּ��ṹ��Ϣ
#define MQUE_MAX_SEND	500
#define MQUE_MIN_SEND	200
	MutliPriorityQueue<ReadWritePacket> m_mqueSendPriority;
	ThreadLock							m_mqueSPLock;


	// ��¼���нṹ��Ϣ
	FastQueue<ReadWritePacket> m_queFreePacket;
	FastQueue<StTransferChannelEx> m_queFreeContext;

	// ��¼�׳���Accept����
	FastQueue<ReadWritePacket> m_quePendingAccepts;   // �׳������б���

	// ��¼�����б�
	FastQueue<StTransferChannelEx> m_queConnectionList;

	// ����Ͷ��Accept����
	HANDLE m_hAcceptEvent;
	HANDLE m_hRepostEvent;
	LONG   m_nRepostCount;

	DWORD  m_dwWorkThreadCount;	// ��¼�����߳�����

	int m_nMaxAccepts;
	int m_nMaxSends;
	int m_nMaxFreeBuffers;
	int m_nMaxConnections;

	HANDLE m_hListenThread;			// �����߳�
	HANDLE m_hCompletion;			// ��ɶ˿ھ��

	long volatile m_nIocpOutStandingSend;
	long volatile m_nIocpOutStandingRecv;
	long volatile m_nIocpOutStandingListen;

	BOOL m_bShutDown;		// ����֪ͨ�����߳��˳�
	BOOL m_bServerStarted;	// ��¼�����Ƿ�����

private:	// �̺߳���
	static DWORD WINAPI _ListenThreadProc(LPVOID lpParam){return ((CIOCPServer*)lpParam)->_threadListen();}
	DWORD _threadListen();
	static DWORD WINAPI _WorkerThreadProc(LPVOID lpParam){return ((CIOCPServer*)lpParam)->_threadWorker();}
	DWORD _threadWorker();
};
