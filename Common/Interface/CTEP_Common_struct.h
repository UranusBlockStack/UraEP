// ���ĵ��������ļ��ṹ������ļ������뵼��������.
// ��Ƶ���Щ����,�������Ѿ���������,���ܸܺ���,����Ƴ������Ѿ�������!�����������࿴����.
// ������ǿ�����,��һ���������̵�����,�벻Ҫ���������ȱ��,лл����!

#pragma once

#include <windef.h>
#include <tchar.h>
#include <ws2ipdef.h>
#include "CTEP_Trans_Packet_Protocol.h"

#pragma warning(disable:4482)	//warning C4482: ʹ���˷Ǳ�׼��չ: �޶�����ʹ����ö�١�EmPacketOperationType��

#define READWRITE_DATA	// ����һ���ɶ�д������
#define READONLY_DATA	// ����һ��ֻ������
#define OPAQUE_DATA		// ����һ�����ɶ�д�Ĳ�͸������

#ifndef _VIRT
#	define _VIRT(x)		virtual x
#	define _VIRT_H		virtual HRESULT
#endif
// 
// �����ļ��ṹ,����,�뵼������������
// 
class  CUserData;
class  CAppChannel;
typedef CAppChannel CAppChl;
class  CTransferChannel;
typedef CTransferChannel CTransChl;

struct ReadWritePacket;
typedef ReadWritePacket RWPacket;

interface ICTEPTransferProtocolCallBack;
interface ICTEPTransferProtocolServer;
interface ICTEPTransferProtocolClient;

typedef ICTEPTransferProtocolCallBack *ICTEPTransferProtocolCallBackPtr;
typedef ICTEPTransferProtocolServer *ICTEPTransferProtocolServerPtr;
typedef ICTEPTransferProtocolClient *ICTEPTransferProtocolClientPtr;

interface ICTEPAppProtocolCallBack;
interface ICTEPAppProtocol;
interface ICTEPAppProtocolEx;

interface ICTEPAppProtocolCrossCallBack;

//	���º���ΪCtep�������˴����Dll�ĵ�������,������CtepCommServer�Խ�ʹ��,���ṩ���ض��ײ�Э�������֧��(��RDP/SPICE��).
//Dll�����Ʊ���������: "CtepTs" ��ͷ,������dllΪ��׺��
#define FUNC_CTEPGetInterfaceTransServer	"CTEPGetInterfaceTransServer"
typedef ICTEPTransferProtocolServer* WINAPI T_CTEPGetInterfaceTransServer();
typedef T_CTEPGetInterfaceTransServer * Fn_CTEPGetInterfaceTransServer;

//	���º���ΪCtep�ͻ��˴����Dll�ĵ�������,������CtepCommClient�Խ�ʹ��,���ṩ���ض��ײ�Э�������֧��(��RDP/SPICE��).
//Dll�����Ʊ���������: "CtepTc" ��ͷ,������dllΪ��׺��
#define FUNC_CTEPGetInterfaceTransClient	"CTEPGetInterfaceTransClient"
typedef ICTEPTransferProtocolClient* WINAPI T_CTEPGetInterfaceTransClient();
typedef T_CTEPGetInterfaceTransClient *Fn_CTEPGetInterfaceTransClient;

//	���º���ΪCtep��������Ӧ�ò�Dll�ĵ�������,������CtepCommServer�Խ�ʹ��,��ʵ��һ������CTEP�ķ�������Ӧ�ó���(��CTMMR/USBREDIR).
//Dll�����Ʊ���������: "CtepAs" ��ͷ,������dllΪ��׺��
#define FUNC_CTEPGetInterfaceAppServer		"CTEPGetInterfaceAppServer"
typedef ICTEPAppProtocol* WINAPI T_CTEPGetInterfaceAppServer();
typedef T_CTEPGetInterfaceAppServer *Fn_CTEPGetInterfaceAppServer;

//	���º���ΪCtep�ͻ���Ӧ�ò�Dll�ĵ�������,������CtepCommClient�Խ�ʹ��,��ʵ��һ������CTEP�Ŀͻ���Ӧ�ó���(��CTMMR/USBREDIR).
//Dll�����Ʊ���������: "CtepAc" ��ͷ,������dllΪ��׺��
#define FUNC_CTEPGetInterfaceAppClient		"CTEPGetInterfaceAppClient"
typedef ICTEPAppProtocol* WINAPI T_CTEPGetInterfaceAppClient();
typedef T_CTEPGetInterfaceAppClient *Fn_CTEPGetInterfaceAppClient;

// 
// һ�����ڿ���̵ķ�������CTEP����/�������ͨ��
// 

//	���º���ΪCtep��������Ӧ�ò�DLL�ĵ�������,������CtepAppProxy�Խ�ʹ��,��ʵ��һ������CTEP�ķ�������Ӧ�ó���(��CTMMR/USBREDIR).
//Dll�����Ʊ���������: "CtepAPxs" ��ͷ,������dllΪ��׺��
#define FUNC_CTEPGetInterfaceAppProxyServer		"CTEPGetInterfaceAppProxyServer"
typedef ICTEPAppProtocol* WINAPI T_CTEPGetInterfaceAppProxyServer();
typedef T_CTEPGetInterfaceAppProxyServer *Fn_CTEPGetInterfaceAppProxyServer;


//	���º������ڵ��������ʹ�õ�CtepӦ�ò�Appʹ�ýӿ�.
#define FUNC_CTEPGetInterfaceAppStubCallBack	"CTEPGetInterfaceAppStubCallBack"
typedef ICTEPAppProtocolCrossCallBack* WINAPI T_CTEPGetInterfaceAppCrossCallBack();
typedef T_CTEPGetInterfaceAppCrossCallBack *Fn_CTEPGetInterfaceAppStubCallBack;





// 
// һЩͨ�����ݽṹ������
// 

// ����һ��ָ���û����û���������
enum EmUserType
{
	User_Normal = 0,		// ��ͨ�û�����,�û����������ɵײ���·���ϲ�Ӧ�����ӹ�ͬ����
	User_WinSession = 1,	// windows Session�û�����,����������windows�Ự����һ��
};
enum EmUserStatus
{
	User_Invalid = -1,		// ��ǰUser������Ч״̬,�����Ѿ��������ͷ�
	User_Disconnected = 0,	// ��ǰUser��������״̬,�û�û����������˽�������
	User_Connected = 1,		// ��ǰUser�Ѿ����û�����������
};
class ATL_NO_VTABLE CUserData
{
public:
	CUserData()
	{
		Init();
	}

	void Init()
	{
		wsUserName[0] = NULL;

		UserId		= INVALID_UID;
		dwSessionId	= INVALID_SESSIONID;
		Type		= (User_Normal);
		Status		= User_Invalid;

		guidUser	= GUID_NULL;

		ZeroObject(addrLocal6);
		ZeroObject(addrRemote6);
	}

	USHORT			UserId;					// -1��ʾ��Ч
	DWORD			dwSessionId;			// -1��ʾ��Ч
	WCHAR			wsUserName[260];		// \0��ʾ��Ч

	GUID			guidUser;
	EmUserType		Type;
	EmUserStatus	Status;

	union
	{
		SOCKADDR_IN  addrLocal;			// ���ӵı��ص�ַ
		SOCKADDR_IN6 addrLocal6;			// ���ӵı��ص�ַ
	};

	union
	{
		SOCKADDR_IN addrRemote;			// ���ӵ�Զ�̵�ַ
		SOCKADDR_IN6 addrRemote6;			// ���ӵ�Զ�̵�ַ
	};
};

// ����һ������������
enum EmPacketOperationType
{
	OP_Empty = 0,
	OP_Listen,			// ������,���ڼ����µ��û����ӵİ�
	OP_IocpSend,		// ��ɶ˿��ϵķ������ݰ�
	OP_IocpRecv,		// ��ɶ˿��ϵĽ������ݰ�
};

// �ڴ�ģ��,������һ���ڴ��ַ�Լ��ڲ�����Ч��������(��λ:bytes)
struct CBuffer
{
	void Init(char* buffer = 0, DWORD buffsize = 0)
	{
		buff = buffer;
		size = 0;
		maxlength = buffsize;
	}
	char* buff;
	DWORD size;
	DWORD maxlength;
};

// һ�������Ĳ�����
struct ReadWritePacket
{
	void PacketInit()
	{
		buff.Init(cBuf, CTEP_DEFAULT_BUFFER_SIZE);
		pointer = nullptr;
		pTransChn = NULL;
		opType = OP_Empty;
		piTrans = NULL;
		hFile = INVALID_HANDLE_VALUE;
		pNext = nullptr;
		ol.Internal = 0;
		ol.InternalHigh = 0;
	}

	union
	{
		WSAOVERLAPPED wsaol;
		OVERLAPPED	  ol;
	};

	READWRITE_DATA EmPacketOperationType opType;

	union
	{
		READWRITE_DATA HANDLE  hFile;
		READWRITE_DATA SOCKET  s;
	};

	OPAQUE_DATA CBuffer			buff;			// default: pBuffer = Buf;
	OPAQUE_DATA CHAR* volatile	pointer;		// ��ǰ����ָ��

	union
	{
		READWRITE_DATA ICTEPTransferProtocolServer*	piTrans;
		READWRITE_DATA ICTEPTransferProtocolClient*	piTransClt;
	};
	
	OPAQUE_DATA CTransferChannel * pTransChn;

	OPAQUE_DATA ReadWritePacket  * pNext;

	OPAQUE_DATA CHAR	 cBuf[CTEP_DEFAULT_BUFFER_SIZE];
};



// 
// һЩ��Ϣ�ͻص������ĺ�����ʽ����

//Ĭ�Ϻ�����ʽ,ռλʹ��
typedef void (WINAPI *Call_BaseFunction)(void* pParam);

//windows�û��Ự��Ϣ�ص�������ʽ����,���лỰ��Ϣʱ,CTEP Commģ��֪ͨ����ģ��.
//wParam value is:
//	WTS_CONSOLE_CONNECT			0x1 	A session was connected to the console terminal.
//	WTS_CONSOLE_DISCONNECT		0x2		A session was disconnected from the console terminal.
//	WTS_REMOTE_CONNECT			0x3		A session was connected to the remote terminal.
//	WTS_REMOTE_DISCONNECT		0x4		A session was disconnected from the remote terminal.
//	WTS_SESSION_LOGON			0x5		A user has logged on to the session.
//	WTS_SESSION_LOGOFF			0x6		A user has logged off the session.
//	WTS_SESSION_LOCK			0x7		A session has been locked.
//	WTS_SESSION_UNLOCK			0x8		A session has been unlocked.
//	WTS_SESSION_REMOTE_CONTROL	0x9		A session has changed its remote controlled status. To determine the status, call GetSystemMetrics and check the SM_REMOTECONTROL metric.
//lParam vallue is: SessionId.
typedef void (WINAPI *Call_SessionEvent)(void *pParam, WPARAM wParam, LPARAM lParam);


struct StCallEvent
{
	void* pParam;
	union
	{
		Call_BaseFunction  fnBase;
		Call_SessionEvent  fnSessionEvent;
	};

	enum EmEventType
	{
		none = 0,
		SessionEvent,
	}type;
};







