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
	InvalidChannel = -1,
	StaticChannel = 0,
	DynamicChannel = 1,
	CrossDyncChannel = 2,
};

class __declspec(novtable) CAppChannel		// һ��Ӧ��ͨ��, ��ʾһ��Ӧ�ö�һ���û��ķ���
{
public:
	void Init()
	{
		bClosing = TRUE;

		pAppParam = NULL;
		Type = InvalidChannel;
		AppChannelId = INVALID_ACID;
		StaticACId   = INVALID_ACID;
		TargetACId   = INVALID_ACID;

		dwInterfaceVer = (DWORD)-1;
		piAppProtocol = nullptr;
		pUser = nullptr;

		uPacketOption = 0;
		Level = Low;
		nCount = 0;
		sAppName = nullptr;
	}

public:
	READONLY_DATA  BOOL				bClosing;
	READWRITE_DATA volatile void*	pAppParam;

	READONLY_DATA  EnAppChannelType	Type;			//ͨ������,��̬ͨ�����߶�̬ͨ��

	READONLY_DATA  USHORT			AppChannelId;
	READONLY_DATA  USHORT			StaticACId;		// ��̬ͨ�������ľ�̬ͨ��,����Ǿ�̬ͨ����ֵΪINVALID_ACID

	READONLY_DATA  DWORD			dwInterfaceVer;
	READWRITE_DATA USHORT			TargetACId;
	union
	{
		READONLY_DATA ICTEPAppProtocol*   piAppProtocol;		// ͨ����Ӧ�ķ���ӿ�
		READONLY_DATA ICTEPAppProtocolEx* piAppProtocolEx;		// ͨ����Ӧ�ķ���ӿ�, ��dwInterfaceVer = 1ʱ��Ч
	};
	
	READONLY_DATA CUserData			*pUser;			// ͨ����Ӧ���û�

#define Packet_Can_Lost				0x0001
#define Packet_Need_Encryption		0x1000
	READONLY_DATA USHORT			uPacketOption;
	READONLY_DATA EmPacketLevel		Level;

	READONLY_DATA ULONG				nCount;			// App Dynamic Channel Count

	READONLY_DATA LPCSTR			sAppName;		// Ӧ������

public:
	inline LPCWSTR debugType()
	{
		switch(Type)
		{
		case StaticChannel:
			return L"StaticChannel";
		case DynamicChannel:
			return L"DynamicChannel";
		case CrossDyncChannel:
			return L"CrossDyncChannel";
		}
		return L"EnAppChannelType:??????";
	}
};

interface ATL_NO_VTABLE ICTEPAppProtocolCallBack
{
	_VIRT_H		WritePacket(USHORT AppChannelId, char* buff, ULONG size, CUserData* pUser = nullptr) = 0;
	_VIRT_H		WritePacket(CAppChl *, char* buff, ULONG size) = 0;
	_VIRT_H		WritePacket(CAppChl *, RWPacket *) = 0;

	_VIRT_B		 CreateDynamicChannel(CAppChl* pStaticChannel, EmPacketLevel Level = Middle, USHORT dwType = NULL) = 0;
	_VIRT_V		 CloseDynamicChannel(CAppChl* pDynamicChannel) = 0;
	_VIRT_H			 CloseDynamicChannel(USHORT AppChannelId, CUserData* pUser = nullptr) = 0;

	_VIRT(CAppChl*)	 LockChannel(USHORT AppChannelId, CUserData* pUser = nullptr) = 0;
	_VIRT_V		 UnlockChannel(CAppChl* pAppChannel) = 0;
					 
	_VIRT(RWPacket*) MallocSendPacket(CAppChl *pAppChn, USHORT size) = 0;// size > 0 && size < CTEP_DEFAULT_BUFFER_DATA_SIZE
	_VIRT_V		 FreePacket(RWPacket *) = 0;
					 
	_VIRT_D	 GetInterfaceVer() = 0;	// return 0x0100 = CTEP v1.0, 0x0200 = CTEP v2.0
};

interface ICTEPAppProtocolCallBackEx : public ICTEPAppProtocolCallBack	// CTEP 2.0
{
	_VIRT_H RegisterCallBackEvent(StCallEvent Calls[], DWORD dwCallCount) = 0;
	_VIRT_H UnregisterCallBackEvent(StCallEvent Calls[], DWORD dwCallCount) = 0;

	_VIRT(CAppChl*) CreateDynamicChannelLock(USHORT AppChannelId, EmPacketLevel Level = Middle, USHORT dwType = NULL) = 0;
	_VIRT(CAppChl*) CreateDynamicChannelLock(CAppChl* pStaticChannel, EmPacketLevel Level = Middle, USHORT dwType = NULL) = 0;
};

interface ICTEPAppProtocol
{
	_VIRT(LPCSTR)   GetName() = 0;	// AppName character count <= 15, 
	_VIRT_D    GetInterfaceVersion() = 0; //Ĭ�Ϸ���0, ICTEPAppProtocolEx�ӿڷ���1,

#define CTEP_TYPE_APP_SERVER	1
#define CTEP_TYPE_APP_CLIENT	0
	_VIRT_H  Initialize(ICTEPAppProtocolCallBack* pI, DWORD dwType) = 0;
	_VIRT_V	 Final() = 0;	// server close.

	_VIRT_H		 Connect(   CUserData* pUser, CAppChl* pNewAppChannel, CAppChl* pStaticAppChannel = nullptr) = 0;	// һ������̬ͨ��
	_VIRT_H		 ReadPacket(CUserData* pUser, CAppChl *pAppChannel, char* pBuff, ULONG size) = 0;
	_VIRT_V		 Disconnect(CUserData* pUser, CAppChl *pAppChannel) = 0;													// һ����̬ͨ����
};

interface ICTEPAppProtocolEx : public ICTEPAppProtocol
{
	_VIRT_D     GetNameCount() = 0;// ֧�ֵ�App����,Ĭ��Ϊ1, ProxyӦ���п��ܻ������App.
	_VIRT(LPCSTR)   GetNameIndex(long iIndex = 0) = 0;	// AppName character count <= 15, ����Proxy֧�ֶ��App�Ĵ���ʹ��

	_VIRT_B     QueryDisconnect(CUserData* pUser, CAppChl *pAppChannel) = 0;	//Commģ��ѯ��Appģ��ָ����̬ͨ���Ƿ���Ա��ر���,���Է���TRUE,�ܾ��رշ���FALSE

	_VIRT_V     ChannelStateChanged(CUserData* pUser, CAppChl *pAppChannel) = 0;	//Commģ��֪ͨAppģ��ָ��ͨ����״̬�����ı�(Connected, Disconnected)

	_VIRT_B     ConnectCrossApp(CUserData* pUser, CAppChl* pNewAppChannel, CAppChl* pStaticAppChannel) = 0;
};

// 
// �û�����̵�CTEP AppͨѶʹ�õĽӿ�.
// 
// enum EmCtepAppProxyTargetChannel
// {
// 	StaticAppChannel = 0,	//���� or ���� ͬ��ľ�̬ͨ������
// 	BroadCastAppChannel,	//���� or ���� ͬ�������ͨ���㲥����
// 	SpecialAppChannel,		//���� or ���� ͬ���ָ����ĳ��ͨ������
// 	RemoteAppChannel,		//���� or ���� ��һ��(server2client, client2server)��ͨ������
// };

// interface ICTEPAppProtocolCrossCallBack
// {
// 	_VIRT_H Connect(LPCSTR sAppName, CUserData* pUser, EnAppChannelType type) = 0;//����һ����ǰ�û���ǰӦ�õ�ͨ��(��̬���߾�̬)
// 	_VIRT_H Disconnect();
// 
// 	_VIRT_H WritePacket(EmCtepAppProxyTargetChannel type, USHORT targetAppId
// 		, char* buff, ULONG size) = 0; //��ָ��AppChannelͨ����������, ֻ�ܷ�����ǰ�û������Ӧ�õ�Appͨ��֮�以�෢��.
// 
// #define CTEP_RESULT_S_OK				0
// #define CTEP_RESULT_S_TIMEOUT			5
// #define CTEP_RESULT_E_FAILD				-1
// 	_VIRT_H ReadPacket(__out char** pBuff, __out DWORD *pSize, __out EmCtepAppProxyTargetChannel *pType, __out USHORT *pAppId);
// };


