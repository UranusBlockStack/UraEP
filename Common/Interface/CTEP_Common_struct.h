// 本文档描述了文件结构层面的文件名称与导出函数名.
// 设计的有些复杂,不过我已经尽量简化了,功能很复杂,能设计成这样已经不错了!如果看不懂请多看几遍.
// 如果还是看不懂,那一定是你智商的问题,请不要怀疑设计有缺陷,谢谢合作!

#pragma once

#include <windef.h>
#include <tchar.h>
#include "CTEP_Trans_Packet_Protocol.h"

#define READWRITE_DATA	// 描述一个可读写的数据
#define READONLY_DATA	// 描述一个只读数据
#define OPAQUE_DATA		// 描述一个不可读写的不透明数据


// 
// 关于文件结构,名称,与导出函数的描述
// 
class  CUserData;
class  CAppChannel;
class  CTransferChannel;
struct ReadWritePacket;

interface ICTEPTransferProtocolCallBack;
interface ICTEPTransferProtocolServer;
interface ICTEPTransferProtocolClient;

interface ICTEPAppProtocolCallBack;
interface ICTEPAppProtocol;

interface ICTEPAppProtocolStubCallBack;

//	以下函数为Ctep服务器端传输层Dll的导出函数,用于与CtepCommServer对接使用,以提供对特定底层协议的连接支持(如RDP/SPICE等).
//Dll的名称必须以名称: "CtepTs" 开头,并且以dll为后缀名
#define FUNC_CTEPGetInterfaceTransServer	"CTEPGetInterfaceTransServer"
typedef ICTEPTransferProtocolServer* (WINAPI *Fn_CTEPGetInterfaceTransServer)();
ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();

//	以下函数为Ctep客户端传输层Dll的导出函数,用于与CtepCommClient对接使用,以提供对特定底层协议的连接支持(如RDP/SPICE等).
//Dll的名称必须以名称: "CtepTc" 开头,并且以dll为后缀名
#define FUNC_CTEPGetInterfaceTransClient	"CTEPGetInterfaceTransClient"
typedef ICTEPTransferProtocolClient* (WINAPI *Fn_CTEPGetInterfaceTransClient)();
ICTEPTransferProtocolClient* WINAPI CTEPGetInterfaceTransClient();

//	以下函数为Ctep服务器端应用层Dll的导出函数,用于与CtepCommServer对接使用,以实现一个基于CTEP的服务器端应用程序(如CTMMR/USBREDIR).
//Dll的名称必须以名称: "CtepAs" 开头,并且以dll为后缀名
#define FUNC_CTEPGetInterfaceAppServer		"CTEPGetInterfaceAppServer"
typedef ICTEPAppProtocol* (WINAPI *Fn_CTEPGetInterfaceAppServer)();
ICTEPAppProtocol* WINAPI CTEPGetInterfaceAppServer();

//	以下函数为Ctep客户端应用层Dll的导出函数,用于与CtepCommClient对接使用,以实现一个基于CTEP的客户端应用程序(如CTMMR/USBREDIR).
//Dll的名称必须以名称: "CtepAc" 开头,并且以dll为后缀名
#define FUNC_CTEPGetInterfaceAppClient		"CTEPGetInterfaceAppClient"
typedef ICTEPAppProtocol* (WINAPI *Fn_CTEPGetInterfaceAppClient)();
ICTEPAppProtocol* WINAPI CTEPGetInterfaceAppClient();

// 
// 一下用于跨进程的服务器端CTEP代理/存根代码通信
// 

//	以下函数为Ctep服务器端应用层DLL的导出函数,用于与CtepAppProxy对接使用,以实现一个基于CTEP的服务器端应用程序(如CTMMR/USBREDIR).
//Dll的名称必须以名称: "CtepAPxs" 开头,并且以dll为后缀名
#define FUNC_CTEPGetInterfaceAppProxyServer		"CTEPGetInterfaceAppProxyServer"
typedef ICTEPAppProtocol* (WINAPI *Fn_CTEPGetInterfaceAppProxyServer)();
ICTEPAppProtocol* WINAPI CTEPGetInterfaceAppProxyServer();

//	以下函数用于导出跨进程使用的Ctep应用层App使用接口.
#define FUNC_CTEPGetInterfaceAppStubCallBack	"CTEPGetInterfaceAppStubCallBack"
typedef ICTEPAppProtocolStubCallBack* (WINAPI *Fn_CTEPGetInterfaceAppStubCallBack)();
ICTEPAppProtocolStubCallBack* WINAPI CTEPGetInterfaceAppStubCallBack();




// 
// 一些通用数据结构的描述
// 

// 描述一个指定用户的用户特性数据
class ATL_NO_VTABLE CUserData
{
public:
	CUserData() : UserId((USHORT)-1), dwSessionId((DWORD)-1)
	{
		wsUserName[0] = NULL;
	}

	USHORT UserId;				// -1表示无效
	DWORD dwSessionId;			// -1表示无效
	WCHAR wsUserName[260];		// \0表示无效
};

// 描述一个包操作类型
enum EmPacketOperationType
{
	OP_Empty = 0,
	OP_Listen,			// 监听包,用于监听新的用户连接的包
	OP_IocpSend,		// 完成端口上的发送数据包
	OP_IocpRecv,		// 完成端口上的接收数据包
};

// 内存模组,描述了一块内存地址以及内部的有效数据数量(单位:bytes)
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

// 一个独立的操作包
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
	OPAQUE_DATA CHAR* volatile	pointer;

	union
	{
		READWRITE_DATA ICTEPTransferProtocolServer*	piTrans;
		READWRITE_DATA ICTEPTransferProtocolClient*	piTransClt;
	};
	
	OPAQUE_DATA CTransferChannel * pTransChn;

	OPAQUE_DATA ReadWritePacket  * pNext;

	OPAQUE_DATA CHAR	 cBuf[CTEP_DEFAULT_BUFFER_SIZE];
};







