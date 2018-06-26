#pragma once

#include "CTEP_Common_struct.h"

enum EmPacketLevel
{
	Low = 0,
	Middle = 1,
	High = 2,
};

enum EnAppChannelType
{
	StaticChannel = 0,
	DynamicChannel = 1,
};
inline LPCWSTR debugEmAppChannelType(EnAppChannelType type)
{
	switch(type)
	{
	case StaticChannel:
		return L"StaticChannel";
	case DynamicChannel:
		return L"DynamicChannel";
	}
	return L"EnAppChannelType:??????";
}

class __declspec(novtable) CAppChannel		// һ��Ӧ��ͨ��, ��ʾһ��Ӧ�ö�һ���û��ķ���
{
public:
	READONLY_DATA BOOL				bClosing;
	READWRITE_DATA void*			pAppParam;

	READONLY_DATA EnAppChannelType	Type;			//ͨ������,��̬ͨ�����߶�̬ͨ��

	READONLY_DATA USHORT			AppChannelId;
	READONLY_DATA ICTEPAppProtocol* piAppProtocol;	// ͨ����Ӧ�ķ���ӿ�
	READONLY_DATA CUserData			*pUser;			// ͨ����Ӧ���û�

#define Packet_Can_Lost				0x0001
#define Packet_Need_Encryption		0x1000
	READONLY_DATA USHORT			uPacketOption;
	READONLY_DATA EmPacketLevel		Level;

	READONLY_DATA ULONG volatile  nCount;	// App Dynamic Channel Count
};


interface ICTEPAppProtocolCallBack
{
	virtual HRESULT WritePacket(CAppChannel *, char* buff, ULONG size) = 0;
	virtual HRESULT WritePacket(CAppChannel *, ReadWritePacket *) = 0;

	virtual CAppChannel* CreateDynamicChannel(CAppChannel* pStaticChannel
		 , EmPacketLevel Level = Middle, USHORT dwType = NULL) = 0;
	virtual void	CloseDynamicChannel(CAppChannel* pDynamicChannel) = 0;

	virtual CAppChannel* LockChannel(USHORT AppChannelId) = 0;
	virtual void UnlockChannel(CAppChannel* pAppChannel) = 0;

	virtual ReadWritePacket*	MallocSendPacket(CAppChannel *pAppChn, USHORT size) = 0;// size > 0 && size < CTEP_DEFAULT_BUFFER_DATA_SIZE
	virtual void				FreePacket(ReadWritePacket *) = 0;
};


interface ICTEPAppProtocol
{
	virtual LPCSTR   GetName() = 0;	// AppName character count <= 15
#define CTEP_TYPE_APP_SERVER	0x1
#define CTEP_TYPE_APP_CLIENT	0
	virtual HRESULT  Initialize(ICTEPAppProtocolCallBack* pI, DWORD dwType) = 0;
	virtual void	 Final() = 0;	// server close.

	virtual HRESULT  Connect(CUserData* pUser, CAppChannel* pNewAppChannel, CAppChannel *pStaticAppChannel/* = nullptr*/) = 0;
	virtual HRESULT  ReadPacket(CUserData* pUser, CAppChannel *pAppChannel, char* pBuff, ULONG size) = 0;
	virtual void	 Disconnect(CUserData* pUser, CAppChannel *pAppChannel) = 0;
};






