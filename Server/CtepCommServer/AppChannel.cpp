#include "stdafx.h"

#include "AppChannel.h"


// class CCtepCommunicationServer
HRESULT CCtepCommunicationServer::WritePacket(CAppChannel *pAppChn, char* buff, ULONG size)
{
	if ( !pAppChn || !pAppChn->pUser || ((CUserDataEx*)pAppChn->pUser)->bClosing)
		return E_FAIL;

	if ( !buff || size == 0)
		return E_INVALIDARG;

	DWORD nPacketCount = size / CTEP_DEFAULT_BUFFER_DATA_SIZE
		+ (size % CTEP_DEFAULT_BUFFER_DATA_SIZE > 0 ? 1 : 0);

	DWORD dwPacketHeader = 
		nPacketCount == 1 ? sizeof(CTEPPacket_Header) : sizeof(CTEPPacket_Message);

	CUserDataEx* pUser = (CUserDataEx*)pAppChn->pUser;

	DWORD dwSizeSent = 0;
	for (DWORD i = 0; i < nPacketCount; i++)
	{
		USHORT dwPacketLength = (USHORT)min(CTEP_DEFAULT_BUFFER_DATA_SIZE, size - dwSizeSent);
		ReadWritePacket* pPacket = MallocPacket(pAppChn, dwPacketLength);
		if ( !pPacket)
		{
			ASSERT(0);
			return E_OUTOFMEMORY;
		}

		// ��дͷ
		pPacket->buff.size = dwPacketHeader + dwPacketLength;
		if ( nPacketCount == 1)
		{
			Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
				, pUser->UserId, pAppChn->AppChannelId, buff+dwSizeSent, dwPacketLength);
		}
		else
		{
			if ( i == 0)
			{
				Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
					, pUser->UserId, pAppChn->AppChannelId, buff+dwSizeSent, dwPacketLength
					, size, true, false);
			}
			else if ( i == nPacketCount - 1)
			{
				Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
					, pUser->UserId, pAppChn->AppChannelId, buff+dwSizeSent, dwPacketLength
					, size, false, true);
			}
			else
			{
				Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
					, pUser->UserId, pAppChn->AppChannelId, buff+dwSizeSent, dwPacketLength
					, size, false, false);
			}
		}
		if ( FAILED(WritePacket(pAppChn, pPacket)))
			return E_FAIL;

		dwSizeSent += dwPacketLength;
	}

	return S_OK;
}


	// CAppChannel::uPacketOption option
CAppChannel* CCtepCommunicationServer::CreateDynamicChannel(CAppChannel* pStaticChannel
	, EmPacketLevel level /*= Middle*/, USHORT option /*= NULL*/)
{
	CAppChannelEx *pAppChnEx = (CAppChannelEx*)pStaticChannel;
	CUserDataEx* pUserEx = (CUserDataEx*)pAppChnEx->pUser;

	ASSERT(pAppChnEx && pAppChnEx->pStaticAppChannel && pUserEx);
	if ( !pAppChnEx || !pAppChnEx->pStaticAppChannel || !pUserEx || pUserEx->bClosing)
		return nullptr;

	CAppChannel *pNewAppChannel = allocateAppChannel((CUserDataEx*)pAppChnEx->pUser
		, pAppChnEx->piAppProtocol, pAppChnEx->pStaticAppChannel, level, option);

	// ���Ϳͻ���Ӧ�ò㶯̬ͨ��������Ϣ
	if ( pNewAppChannel)
	{
		ReadWritePacket* pPacketSend = MallocPacket(pNewAppChannel);
		if ( pPacketSend)
		{
			pPacketSend->buff.size = Create_CTEPPacket_CreateAppRsp(
				(CTEPPacket_CreateAppRsp*)pPacketSend->buff.buff, pUserEx->UserId
				, pNewAppChannel->AppChannelId
				, 0
				, pStaticChannel->piAppProtocol->GetName()
				, option, (USHORT)level
				, pStaticChannel->AppChannelId
				, TRUE);

			WritePacket(pNewAppChannel, pPacketSend);
		}
		else
		{
			m_log.Error(3, L"ConnectEstablished - MallocPacket Failed. No Enough Memory.");
		}
	}

	return pNewAppChannel;
}

void	CCtepCommunicationServer::CloseDynamicChannel(CAppChannel* pDynamicChannel)
{
	CAppChannelEx *pAppChnEx = (CAppChannelEx*)pDynamicChannel;
	ASSERT(pAppChnEx && pAppChnEx->pStaticAppChannel && pAppChnEx != pAppChnEx->pStaticAppChannel);
	if ( !pAppChnEx || pAppChnEx == pAppChnEx->pStaticAppChannel)
		return ;

	// ���Ϳͻ���Ӧ�ó���̬ͨ��ɾ����Ϣ
	CUserDataEx* pUserEx = (CUserDataEx*)pAppChnEx->pUser;
	if ( pUserEx && !pUserEx->bClosing)
	{
		ReadWritePacket* pBuffer = MallocPacket(pDynamicChannel);
		if ( pBuffer)
		{
			pBuffer->buff.size = Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer->buff.buff
				, sizeof(CTEPPacket_CloseAppRsp), CTEP_PACKET_CONTENT_CLOSE_APP_RSP
				, pUserEx->UserId, pDynamicChannel->AppChannelId);

			WritePacket(pDynamicChannel, pBuffer);
		}
	}

	releaseAppChannel(pAppChnEx);
}

void CCtepCommunicationServer::CloseStaticChannel(CAppChannel* pStaticChannel)
{
	CUserDataEx* pUserEx = (CUserDataEx*)pStaticChannel->pUser;
	CAppChannelEx *pStaticChn = ((CAppChannelEx*)pStaticChannel)->pStaticAppChannel;
	ASSERT(pUserEx && pStaticChn);

	ASSERT(((CAppChannelEx*)pStaticChannel)->pStaticAppChannel == pStaticChannel
		&& pStaticChannel->Type == StaticChannel);

	// �ر�ָ����̬Channel�Լ�Channel֮�µ����ж�̬Channel
	CAppChannelEx *pDynamicChn = pStaticChn->PopLink();
	while(pDynamicChn)
	{
		CloseDynamicChannel(pDynamicChn);
		pDynamicChn = pStaticChn->PopLink();
	}

	// ���Ϳͻ���Ӧ�ò㾲̬ͨ��ɾ����Ϣ
	if ( pUserEx && !pUserEx->bClosing)
	{
		ReadWritePacket* pBuffer = MallocPacket(pStaticChannel);
		if ( pBuffer)
		{
			pBuffer->buff.size = Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer->buff.buff
				, sizeof(CTEPPacket_CloseAppRsp), CTEP_PACKET_CONTENT_CLOSE_APP_RSP
				, pUserEx->UserId, pStaticChannel->AppChannelId);

			WritePacket(pStaticChannel, pBuffer);
		}
	}

	releaseAppChannel(pStaticChannel);
}

void CCtepCommunicationServer::closeAppChannel(CAppChannel* pAppChannel)			// close static channel.
{
	if ( !pAppChannel)
	{
		ASSERT(0);
		return ;
	}

	if ( pAppChannel->Type == DynamicChannel)
	{
		CloseDynamicChannel(pAppChannel);
		return ;
	}
	else
	{
		CloseStaticChannel(pAppChannel);
		return ;
	}
}

CAppChannel* CCtepCommunicationServer::allocateAppChannel(CUserDataEx *user, ICTEPAppProtocol* piAppProt
	, CAppChannelEx *pStaAppChn /*= nullptr*/, EmPacketLevel level /*= Middle*/, USHORT option /*= 0*/)
{
	ASSERT(user && piAppProt);
	if ( level > High)
		level = Middle;
	else if ( level < Low)
		level = Low;

	CAppChannelEx* pNewAppChannel = new CAppChannelEx(user, piAppProt, pStaAppChn, level, option);

	pNewAppChannel->AppChannelId = m_smapAppChn.Push(pNewAppChannel);

	if ( pNewAppChannel->AppChannelId == m_smapAppChn.InvalidCount())
	{
		releaseAppChannel(pNewAppChannel);
		pNewAppChannel = nullptr;
	}

	return pNewAppChannel;
}
void CCtepCommunicationServer::releaseAppChannel(CAppChannel* pAppChn)
{
	ASSERT(pAppChn);
	if ( pAppChn->AppChannelId != m_smapAppChn.InvalidCount())
	{
		CAppChannelEx* pAppChnEx = (CAppChannelEx*)pAppChn;
		if ( pAppChnEx->piAppProtocol)
		{
			pAppChnEx->Lock();
			if ( pAppChnEx->piAppProtocol)
			{
				pAppChnEx->piAppProtocol->Disconnect(pAppChnEx->pUser, pAppChn);
				pAppChnEx->piAppProtocol = nullptr;
			}
			pAppChnEx->Unlock();
		}

		m_smapAppChn.Pop(pAppChn->AppChannelId);
	}

	delete (CAppChannelEx*)pAppChn;
}
CUserDataEx* CCtepCommunicationServer::allocateUser(StTransferChannelEx* pTransChnMain)
{
	ASSERT(pTransChnMain->head.pUser == nullptr);
	CUserDataEx* pUserEx = new CUserDataEx(pTransChnMain, GUID_NULL);
	UuidCreate(&pUserEx->guidUser);
	pUserEx->UserId = m_smapUser.Push(pUserEx);
	if ( pUserEx->UserId == (USHORT)-1)
	{
		releaseUser(pUserEx);
		return nullptr;
	}

	if ( pUserEx)
	{
		pTransChnMain->head.pUser = pUserEx;
	}

	return pUserEx;
}
void CCtepCommunicationServer::releaseUser(CUserDataEx* pUserEx)
{
	ASSERT( !pUserEx->pTransChnMain && !pUserEx->pTransChnTcp && !pUserEx->pTransChnUdp);
	if ( pUserEx->UserId != (USHORT)-1)
	{
		m_smapUser.Pop(pUserEx->UserId);
	}
	delete pUserEx;
}



void CCtepCommunicationServer::OnConnectionEstablished(StTransferChannelEx *pContext, ReadWritePacket *pBuffer)
{
	m_log.wprint(L" ���յ�һ���µ�����[0x%08x]��%d���� %S \n"
		, pContext, GetCurrentConnection(), ::inet_ntoa(pContext->head.addrRemote.sin_addr));
	CUserDataEx *pUserEx = nullptr;

	if ( pContext->head.type == TCP)
	{
		if (pBuffer->buff.size != sizeof(CTEPPacket_Init))
		{
			ASSERT(pBuffer->buff.size < sizeof(CTEPPacket_Init));
			goto ErrorEnd;
		}

		CTEPPacket_Init* pPacketInit = (CTEPPacket_Init*)pBuffer->buff.buff;
		if ( !pPacketInit->IsPacketInit())
			goto ErrorEnd;

		if ( pPacketInit->header.GetUserId() == (WORD)-1)
		{
			ASSERT(IsEqualGUID(GUID_NULL, pPacketInit->guidUserSession));
			pUserEx = allocateUser(pContext);
			if ( !pUserEx)
			{
				m_log.Error(3, L"ConnectEstablished - allocateUser Failed.");
				goto ErrorEnd;
			}
		} 
		else
		{
			pUserEx = m_smapUser.Find(pPacketInit->header.GetUserId());
			if ( !pUserEx)
			{
				m_log.Error(3, L"ConnectEstablished Failed. UserData Not Found.");
				goto ErrorEnd;
			}

			ASSERT(pUserEx->pTransChnTcp == nullptr);
			if ( pUserEx->pTransChnTcp != nullptr)
			{
				m_log.Error(3, L"ConnectEstablished Failed. Tcp Channel has Connected.");
				goto ErrorEnd;
			}

			GUID guidServer;
			memcpy(&guidServer, &pPacketInit->guidUserSession, sizeof(GUID));
			if ( !IsEqualGUID(guidServer, pUserEx->guidUser))
			{
				m_log.Error(3, L"ConnectEstablished Failed. Guid not match.");
				goto ErrorEnd;
			}

			pUserEx->Lock();
			pUserEx->pTransChnTcp = pContext;
			pContext->head.pUser = pUserEx;
			pUserEx->Unlock();
		}
	}
	else if ( pContext->head.type == UDP)
	{
		ASSERT(0);// Not Implement
		pUserEx = allocateUser(pContext);
	}
	else
	{
		ASSERT(pBuffer->buff.size == 0);
		pUserEx = allocateUser(pContext);
	}

	// �ظ�Init Response
	ASSERT(pUserEx && pUserEx->pTransChnMain);
	ReadWritePacket* pPacketSend =
		AllocateBuffer((StTransferChannel*)pUserEx->pTransChnMain, OP_Send);
	if ( !pPacketSend)
	{
		m_log.Error(3, L"ConnectEstablished - AllocateBuffer Failed. No Enough Memory.");
		goto ErrorEnd;
	}

	pPacketSend->buff.size = Create_CTEPPacket_Init_Responce(
		(CTEPPacket_Init_Responce*)pPacketSend->buff.buff
		, pUserEx->UserId, pUserEx->dwSessionId, pUserEx->guidUser, pUserEx->wsUserName
		, m_TS[0] ? m_TS[0]->GetPort() : 0, m_TS[1] ? m_TS[1]->GetPort() : 0
		, m_LocalIPv4, m_nIPv4Count);

	SendPacket(pContext, pPacketSend);
	return ;

ErrorEnd:
	m_log.Error(5, L"OnConnectionEstablished Failed!");
	CloseAConnection(pContext);
}

void CCtepCommunicationServer::OnConnectionClosing(StTransferChannelEx *pContext, ReadWritePacket *pBuffer)
{
	UNREFERENCED_PARAMETER(pBuffer);
	CUserDataEx* pUserEx = (CUserDataEx*)pContext->head.pUser;
	m_log.wprint(L" һ�����ӹرգ�[0x%08x] User:[0x%08x]\n", pContext, pUserEx);
	if ( !pUserEx)
	{
		ASSERT(0);
		return ;
	}

	// ���ǶϿ��������.δʵ�� E_NOT_IMPLEMENT
	// �ر��û�
	pContext->head.pUser = nullptr;

	pUserEx->Lock();
	// �ر����еײ�����
	if ( pUserEx->pTransChnTcp)
	{
		CloseAConnection(pUserEx->pTransChnTcp);
		if ( pContext == pUserEx->pTransChnTcp)
			pUserEx->pTransChnTcp = nullptr;
	}
	if ( pUserEx->pTransChnUdp)
	{
		CloseAConnection(pUserEx->pTransChnUdp);
		if ( pContext == pUserEx->pTransChnUdp)
			pUserEx->pTransChnUdp = nullptr;
	}
	if ( pUserEx->pTransChnMain)
	{
		CloseAConnection(pUserEx->pTransChnMain);
		if ( pContext == pUserEx->pTransChnMain)
			pUserEx->pTransChnMain = nullptr;
	}

	// �رո��û��򿪵�����Ӧ��
	if ( !pUserEx->bClosing)
	{
		m_smapAppChn.Lock();
		POSITION pos = m_smapAppChn.GetStartPosition();
		while (pos)
		{
			CAppChannelEx* pFind = m_smapAppChn.GetNextValue(pos);
			if ( pFind->pUser == (CUserData*)pUserEx)
			{
				closeAppChannel(pFind);
			}
		}
		m_smapAppChn.Unlock();

		pUserEx->bClosing = TRUE;
	}

	BOOL bCanReleaseUser = FALSE;
	if ( !pUserEx->pTransChnMain && !pUserEx->pTransChnTcp && !pUserEx->pTransChnUdp)
	{
		bCanReleaseUser = TRUE;
	}

	pUserEx->Unlock();

	if ( bCanReleaseUser)
	{
		releaseUser(pUserEx);
	}
}

void CCtepCommunicationServer::OnReadCompleted(StTransferChannelEx *pContext, ReadWritePacket *pBuffer)
{
	ASSERT(pContext && pBuffer);
	if ( !pContext || !pBuffer)
		return ;

	m_log.wprint(L"�յ�һ�����ݰ�[0x%08x] . ��С:%d", pContext, pBuffer->buff.size);

	CUserDataEx* pUserEx = (CUserDataEx*)pContext->head.pUser;
	if ( !pUserEx || pUserEx->bClosing)
		return ;

	CTEPPacket_Header* pHeader;
	while ( nullptr != (pHeader = pContext->SplitPacket(pBuffer)))
	{
		if ( pHeader->IsInit())
		{
			// �ظ�Init Response
			ASSERT(pUserEx && pUserEx->pTransChnMain);
			ReadWritePacket* pPacketSend =
				AllocateBuffer((StTransferChannel*)pUserEx->pTransChnMain, OP_Send);
			if ( !pPacketSend)
			{
				m_log.Error(3, L"ConnectEstablished - AllocateBuffer Failed. No Enough Memory.");
				goto ErrorEnd;
			}

			pPacketSend->buff.size = Create_CTEPPacket_Init_Responce(
				(CTEPPacket_Init_Responce*)pPacketSend->buff.buff
				, pUserEx->UserId, pUserEx->dwSessionId, pUserEx->guidUser, pUserEx->wsUserName
				, m_TS[0] ? m_TS[0]->GetPort() : 0, m_TS[1] ? m_TS[1]->GetPort() : 0
				, m_LocalIPv4, m_nIPv4Count);

			SendPacket(pContext, pPacketSend);
			continue;
		}

		CAppChannelEx* pAppEx = m_smapAppChn.Find(pHeader->GetAppId());
		if ( pAppEx && pAppEx->pUser != (CUserData*)pUserEx)
		{
			ASSERT(0);// �ҵ���ͨ�������ڵ�ǰ�û�
			continue;
		}

		if ( pHeader->IsCloseApp())
		{
			// ���͹رջظ�
			ReadWritePacket* pPacketSend = AllocateBuffer((StTransferChannel*)pContext, EmPacketOperationType::OP_Send, 0);
			if ( pPacketSend)
			{
				pPacketSend->buff.size = Create_CTEPPacket_Header(
					(CTEPPacket_Header*)pPacketSend->buff.buff
					, sizeof(CTEPPacket_CloseAppRsp), CTEP_PACKET_CONTENT_CLOSE_APP_RSP
					, pUserEx->UserId, pHeader->GetAppId());

				SendPacket(pContext, pPacketSend);
			}
			else
			{
				m_log.Error(3, L"ConnectEstablished - MallocPacket Failed. No Enough Memory.");
			}

			if ( pAppEx)
			{
				closeAppChannel(pAppEx);
			}
			continue;
		}

		if ( pHeader->IsCreateApp())
		{
			CTEPPacket_CreateApp* pPacketRecv = (CTEPPacket_CreateApp*)pHeader;
			CAppChannelEx* pNewAppChannel = nullptr;
			for (DWORD i=0; i< m_AppCount; i++)
			{
				if ( !_stricmp(m_APP[i]->GetName(), pPacketRecv->AppName))
				{
					if ( !pAppEx)
					{
						pPacketRecv->ePacketLevel = EmPacketLevel::Middle;
						pPacketRecv->uPacketOption &= Packet_Need_Encryption;
					}
					pNewAppChannel = (CAppChannelEx*)allocateAppChannel(pUserEx, m_APP[i], pAppEx
						, (EmPacketLevel)pPacketRecv->ePacketLevel, pPacketRecv->uPacketOption);
					break;
				}
			}

			// ���ʹ����ظ�
			ReadWritePacket* pPacketSend = AllocateBuffer((StTransferChannel*)pContext, EmPacketOperationType::OP_Send, 0);
			if ( pPacketSend)
			{
				pPacketSend->buff.size = Create_CTEPPacket_CreateAppRsp(
					(CTEPPacket_CreateAppRsp*)pPacketSend->buff.buff, pUserEx->UserId
					, pNewAppChannel ? pNewAppChannel->AppChannelId : (USHORT)-1
					, pPacketRecv->Key
					, pPacketRecv->AppName
					, pPacketRecv->uPacketOption
					, pPacketRecv->ePacketLevel
					, pHeader->GetAppId()
					, pNewAppChannel ? TRUE : FALSE);

				SendPacket(pContext, pPacketSend);
				if ( pNewAppChannel)
				{
					LOCK((CMyCriticalSection*)pNewAppChannel);
					pNewAppChannel->piAppProtocol->Connect(pUserEx, pNewAppChannel, pAppEx);
				}
				
			}
			else
			{
				m_log.Error(3, L"ConnectEstablished - MallocPacket Failed. No Enough Memory.");
			}



			continue;
		}

		if ( pAppEx && pHeader->IsMessage())
		{
			CTEPPacket_Message* pMsg = (CTEPPacket_Message*)pHeader;

			DWORD dwSize = 0;
			char* buff = pAppEx->SplitPacket(pMsg, dwSize);
			if ( buff && buff != (char*)-1)
			{
				LOCK((CMyCriticalSection*)pAppEx);
				if ( pAppEx->piAppProtocol)
				{
					pAppEx->piAppProtocol->ReadPacket(pUserEx
						, pAppEx, buff, dwSize);
					pAppEx->SplitPacket(0, dwSize);
				}
			}
			else if ( buff == (char*)-1)
			{
				closeAppChannel(pAppEx);
			}

			continue;
		}

		ASSERT(0);
	}

	return ;
ErrorEnd:
	m_log.Error(5, L"OnReadCompleted Failed!");
	CloseAConnection(pContext);
}

BOOL CCtepCommunicationServer::OnStart()
{
	BOOL bRet = FALSE;
	for ( DWORD i=0; i< m_AppCount; i++)
	{
		if ( m_APP[i])
		{
			HRESULT hr = m_APP[i]->Initialize(dynamic_cast<ICTEPAppProtocolCallBack*>(this));
			if ( SUCCEEDED(hr))
			{
				bRet = TRUE;
			}
			else
			{
				m_log.FmtError(5, L"App[%S] initialize failed. ErrCode:0x%08x"
					, m_APP[i]->GetName(), hr);
				m_APP[i]->Final();
				m_APP[i] = 0;
			}
		}
	}
	if ( !bRet)
	{
		m_log.FmtError(5, L"App initialize failed.");
	}

	return bRet;
}

void CCtepCommunicationServer::OnShutdown()
{
	// �ر�����Ӧ������
	CAppChannelEx* pApp;
	while(NULL != (pApp = m_smapAppChn.Pop()))
	{
		closeAppChannel(pApp);
	}

	// ֪ͨ����Ӧ�ùر�
	for ( DWORD i=0; i< m_AppCount; i++)
	{
		if ( m_APP[i])
		{
			m_APP[i]->Final();
		}
	}
}










