#pragma once

#include "CTEP_Communicate_TransferLayer_Interface.h"
#include "CTEP_Communicate_App_Interface.h"

// ����per-Handle���ݡ���������һ���׽��ֵ���Ϣ
struct StTransferChannelEx
{
	void InitEx()
	{
		ASSERT( !bNeedNotificationClosing);
		head.Init();
		nOutstandingSend = 0;
		bClosing = TRUE;
		bNeedNotificationClosing = FALSE;
		bufPacket.Init(cPacketBuf);

		pNext = NULL;
	}
	inline HRESULT DisconnectServer(ReadWritePacket* pBuffer = 0)
	{
		return head.piTrans->Disconnect(&head, pBuffer);
	}
	inline HRESULT DisconnectClient()
	{
		return head.piTransClt->Disconnect(&head);
	}
	inline HRESULT SendPacketClient(ReadWritePacket* pBuffer)
	{
		return head.piTransClt->Send(&head, pBuffer);
	}
	inline HRESULT SendPacketServer(ReadWritePacket* pBuffer)
	{
		return head.piTrans->Send(&head, pBuffer);
	}
	
	CTEPPacket_Header* SplitPacket(ReadWritePacket* pPacket)
	{
		long leftsize;
		CTEPPacket_Header* pHeader;

		// ��ȡpPacket��ʣ���ֽ���
		if ( !pPacket->pointer)
		{
			pPacket->pointer = pPacket->buff.buff;
			leftsize = pPacket->buff.size;
		}
		else
		{
			leftsize = pPacket->buff.size - (pPacket->pointer - pPacket->buff.buff);
			ASSERT(leftsize >= 0);
		}

		if ( leftsize <= 0)
			return nullptr;

		// ����ϴ�ƴװ�õ�������
		if ( bufPacket.size > 0 && bufPacket.size == bufPacket.maxlength)
			bufPacket.Init(cPacketBuf);

		// ����Ƿ����ϴ�ʣ�µ��ֽ�
		if ( bufPacket.size == 0)
		{
			pHeader = (CTEPPacket_Header*)pPacket->pointer;

			// ���pPacket���Ƿ���һ��������
			if (	leftsize < sizeof(CTEPPacket_Header)
				|| pHeader->PacketLength > leftsize)
			{
				memcpy(bufPacket.buff, pPacket->pointer, leftsize);
				pPacket->pointer += leftsize;
				bufPacket.size = leftsize;
				return nullptr;
			}
			else
			{
				pPacket->pointer += pHeader->PacketLength;
				return pHeader;
			}
		}

		// ����ϴ�ʣ�µ��ֽ��Ƿ���������İ�ͷ
		if ( bufPacket.size < sizeof(CTEPPacket_Header))
		{
			DWORD dwCopy = min((DWORD)leftsize, sizeof(CTEPPacket_Header)-bufPacket.size);
			memcpy(bufPacket.buff+bufPacket.size, pPacket->pointer, dwCopy);
			pPacket->pointer += dwCopy;
			bufPacket.size += dwCopy;

			if ( bufPacket.size < sizeof(CTEPPacket_Header))
				return nullptr;
		}

		pHeader = (CTEPPacket_Header*)bufPacket.buff;
		ASSERT(bufPacket.maxlength == 0 || bufPacket.maxlength == pHeader->PacketLength);
		bufPacket.maxlength = pHeader->PacketLength;
		ASSERT(bufPacket.size <= bufPacket.maxlength
			&& bufPacket.maxlength >= sizeof(CTEPPacket_Header));

		if (	bufPacket.size > bufPacket.maxlength
			|| bufPacket.maxlength < sizeof(CTEPPacket_Header))
		{
			bufPacket.Init(cPacketBuf);
			return (CTEPPacket_Header*)-1;
		}

		if ( bufPacket.size == bufPacket.maxlength)
			return pHeader;

		DWORD dwCopy = min((DWORD)leftsize, bufPacket.maxlength - bufPacket.size);
		memcpy(bufPacket.buff+bufPacket.size, pPacket->pointer, dwCopy);
		pPacket->pointer += dwCopy;
		bufPacket.size += dwCopy;

		if ( bufPacket.size == bufPacket.maxlength)
			return pHeader;

		return nullptr;
	}

public:
	StTransferChannel head;// 

	volatile long nOutstandingSend;// ���׽������׳����ص�����������

	volatile BOOL bClosing;					// �׽����Ƿ�ر�
	volatile UINT bNeedNotificationClosing;	// �Ƿ���Ҫ֪ͨApp�û��˳�, TRUE:��Ҫ֪ͨ, FALSE:����Ҫ֪ͨ

	// ����δ��ɵ�һ���ײ�ְ�
	CBuffer			bufPacket;
	char			cPacketBuf[CTEP_DEFAULT_BUFFER_SIZE];

	StTransferChannelEx *pNext;
};


class CUserDataEx : public CUserData
	, public CMyCriticalSection
{
public:
	explicit CUserDataEx(StTransferChannelEx* pMain, const GUID& guid = GUID_NULL):pTransChnMain(pMain)
		, pTransChnTcp(nullptr), pTransChnUdp(nullptr), bClosing(FALSE), guidUser(guid)
	{
		UserId = (USHORT)-1;
		dwSessionId = (DWORD)-1;
		ZeroObject(wsUserName);
	}

public:
	BOOL				 bClosing;
	GUID				 guidUser;
	StTransferChannelEx* pTransChnMain;
	StTransferChannelEx* pTransChnTcp;
	StTransferChannelEx* pTransChnUdp;
};

class CAppChannelEx : public CAppChannel, public CMyCriticalSection
{
public:
	CAppChannelEx(CUserDataEx *pUserEx, ICTEPAppProtocol* piAppProt, 
		CAppChannelEx *pStaAppChn /*= nullptr*/, EmPacketLevel level /*= Middle*/, USHORT option /*= 0*/)
		: pStaticAppChannel(pStaAppChn), pNextDynamicAppChannel(nullptr)
	{
		bufData.Init();
		pUser = pUserEx;
		pAppParam = 0;
		AppChannelId = (USHORT)-1;
		piAppProtocol = piAppProt;
		Level = level;
		uPacketOption = option;
		ASSERT(pUser);
		ASSERT( !pStaticAppChannel || 0 == (uPacketOption&Packet_Can_Lost));

		if ( pStaticAppChannel && (uPacketOption&Packet_Can_Lost) && pUserEx->pTransChnUdp)
		{
			this->pTransChannel = (StTransferChannel*)pUserEx->pTransChnUdp;
		}
		else if ( pUserEx->pTransChnTcp)
		{
			this->pTransChannel = (StTransferChannel*)pUserEx->pTransChnTcp;
		}
		else if ( pUserEx->pTransChnMain)
		{
			this->pTransChannel = (StTransferChannel*)pUserEx->pTransChnMain;
		}
		else
		{
			pTransChannel = nullptr;
			ASSERT(0);
		}

		if ( pStaticAppChannel)
		{
			Type = DynamicChannel;
			PushLink(pStaticAppChannel);
		}
		else
		{
			Type = StaticChannel;
			pStaticAppChannel = this;
		}
	}
	~CAppChannelEx()
	{
		if ( bufData.buff)
		{
			free(bufData.buff);
		}
		RemoveLink();
	}

	inline char* SplitPacket( CTEPPacket_Message* pMsg, __out DWORD& dwSize)
	{
		CTEPPacket_Header *pHeader = (CTEPPacket_Header*)pMsg;
		if ( !pMsg)
		{
			if ( bufData.buff)
			{
				free(bufData.buff);
				bufData.Init();
			}

			return nullptr;
		}

		if ( pHeader->EntirePacket())
		{
			dwSize = pMsg->GetMessageLength();
			return pMsg->GetBuffer();
		}

		DWORD dwEntirSize = pMsg->entireLength;
		if ( dwEntirSize < bufData.size + pMsg->GetMessageLength())// ���մ�С��������С
		{
			ASSERT(0);
			return (char*)-1;
		}

		if ( pHeader->FirstPacket())
		{
			if ( bufData.buff || bufData.size > 0)
			{
				ASSERT(0);
				return (char*)-1;
			}

			bufData.buff = (char*)malloc(dwEntirSize);
			if ( !bufData.buff)
			{
				ASSERT(0);
				return (char*)-1;
			}

			bufData.maxlength = dwEntirSize;
		}

		memcpy(bufData.buff + bufData.size, pMsg->GetBuffer(), pMsg->GetMessageLength());
		bufData.size += pMsg->GetMessageLength();

		if ( pHeader->LastPacket())
		{
			if ( dwEntirSize != bufData.size || dwEntirSize != bufData.maxlength)
			{
				ASSERT(0);
				return (char*)-1;
			}

			dwSize = dwEntirSize;
			return bufData.buff;
		}

		return nullptr;
	}

	inline void PushLink(CAppChannelEx* pStaChn)
	{
		ASSERT(pStaChn && pStaChn == pStaticAppChannel && Type == DynamicChannel);
		pStaChn->Lock();
		pNextDynamicAppChannel = pStaChn->pNextDynamicAppChannel;
		pStaChn->pNextDynamicAppChannel = this;
		pStaChn->Unlock();
	}
	inline void RemoveLink()// DynamicChannel ��master���Ƴ�
	{
		CAppChannelEx *pTemp = pStaticAppChannel;
		if ( pTemp && this != pTemp)
		{
			ASSERT(Type == DynamicChannel);
			pTemp->Lock();
			if ( pStaticAppChannel)
			{
				pStaticAppChannel = nullptr;

				CAppChannelEx *p1, *p2;
				p1 = pTemp;
				p2 = p1->pNextDynamicAppChannel;
				while ( p2 && p2 != this)
				{
					p1 = p1->pNextDynamicAppChannel;
					p2 = p2->pNextDynamicAppChannel;
				}

				if ( p2)
				{
					p1->pNextDynamicAppChannel = p2->pNextDynamicAppChannel;
					p2->pNextDynamicAppChannel = nullptr;
				}
			}
			pTemp->Lock();
		}
		else
		{
			ASSERT(Type == StaticChannel);
			ASSERT( pNextDynamicAppChannel == nullptr);	// ��̬ͨ���ر�ʱ,���뱣֤��������������ж�̬ͨ�����Ѿ��˳����
		}
	}
	inline CAppChannelEx* PopLink()// ȡ��һ����̬ͨ��,�п���ȡ���Լ�,Ҳ�п��ܲ����Լ�
	{
		CAppChannelEx* pResult = nullptr;
		if ( pStaticAppChannel && pStaticAppChannel->pNextDynamicAppChannel)
		{
			pStaticAppChannel->Lock();

			if ( pStaticAppChannel && pStaticAppChannel->pNextDynamicAppChannel)
			{
				pResult = pStaticAppChannel->pNextDynamicAppChannel;
				pStaticAppChannel->pNextDynamicAppChannel = pResult->pNextDynamicAppChannel;
			}

			pStaticAppChannel->Unlock();

			pResult->pNextDynamicAppChannel = nullptr;
		}
		return pResult;
	}

public:
	CAppChannelEx	*pStaticAppChannel;
	CAppChannelEx	*pNextDynamicAppChannel;

	//����δ��ɵ�һ���ϲ�ְ�
	CBuffer			bufData;
};











