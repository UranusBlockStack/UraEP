#ifndef __CTVPCHANNEL_H__
#define __CTVPCHANNEL_H__

#ifdef _WIN32 
#define VCAPITYPE __stdcall
#define VCEXPORT
#else // _WIN32
#define VCAPITYPE CALLBACK
#define VCEXPORT  __export
#endif // _WIN32

#pragma once
#include <windows.h>
#include <tchar.h>

#define CTVPCHANNEL_NAME_LEN                    7
#define VIRTUAL_CHANNEL_VERSION_WIN2000         1


#define CHANNEL_RC_OK                           0       //���гɹ��Ķ��������ֵ
#define CHANNEL_OPTION_INITIALIZED              1001    //��ʼ��ʱ��CTVPCHANNEL_DEF�е� option�����ֵ



// VOID VCAPITYPE CTVPVirtualChannelInitEventProc(LPVOID pInitHandle, UINT event, LPVOID pData, UINT dataLength);
// void WINAPI CTVPVirtualChannelOpenEvent(DWORD openHandle, UINT event, LPVOID pdata, UINT32 dataLength, UINT32 totalLength, UINT32 dataFlags);

//�ڳ�ʼ��ʱ��CTVPVirtualChannelInitEventProc ���ظ�
#define CHANNEL_EVENT_INITIALIZED               1101    //
#define CHANNEL_EVENT_CONNECTED                 1102
#define CHANNEL_EVENT_V1_CONNECTED              1103
#define CHANNEL_EVENT_DISCONNECTED              1104
#define CHANNEL_EVENT_TERMINATED                1105

//�ڴ�ͨ��ʱע�� CTVPVIRTUALCHANNELOPEN �ص�����
#define CHANNEL_EVENT_DATA_RECEIVED             2001
#define CHANNEL_EVENT_WRITE_COMPLETE            2002
#define CHANNEL_EVENT_WRITE_CANCELLED           2003
#define CHANNEL_EVENT_READ_COMPLETE             2004
#define CHANNEL_EVENT_READ_CANCELLED            2005

typedef struct tagCTVPCHANNEL_DEF
{
	char            name[CTVPCHANNEL_NAME_LEN + 1];
	ULONG           options;
} CTVPCHANNEL_DEF;
typedef CTVPCHANNEL_DEF UNALIGNED FAR *PCTVPCHANNEL_DEF;
typedef PCTVPCHANNEL_DEF UNALIGNED FAR *PPCTVPCHANNEL_DEF;

typedef VOID VCAPITYPE CTVPCHANNEL_INIT_EVENT_FN(LPVOID pInitHandle,
	UINT   event,
	LPVOID pData,
	UINT   dataLength);
typedef CTVPCHANNEL_INIT_EVENT_FN FAR * PCTVPCHANNEL_INIT_EVENT_FN;

typedef VOID VCAPITYPE CTVPCHANNEL_OPEN_EVENT_FN(DWORD  openHandle,
	UINT   event,
	LPVOID pData,
	UINT32 dataLength,
	UINT32 totalLength,
	UINT32 dataFlags);
typedef CTVPCHANNEL_OPEN_EVENT_FN FAR * PCTVPCHANNEL_OPEN_EVENT_FN;


//�ڳ�ʼ��ʱע�� PCTVPCHANNEL_INIT_EVENT_FN �ص�����
typedef UINT VCAPITYPE CTVPVIRTUALCHANNELINIT(
	LPVOID FAR *           ppInitHandle,
	PCTVPCHANNEL_DEF           pChannel,
	INT                    channelCount,
	ULONG                  versionRequested,
	PCTVPCHANNEL_INIT_EVENT_FN pChannelInitEventProc);
typedef CTVPVIRTUALCHANNELINIT FAR * PCTVPVIRTUALCHANNELINIT;


//�ڴ�ͨ��ʱע�� CTVPVIRTUALCHANNELOPEN �ص�����
typedef UINT VCAPITYPE CTVPVIRTUALCHANNELOPEN(
	LPVOID                 pInitHandle,
	LPDWORD                pOpenHandle,
	PCHAR                  pChannelName,
	PCTVPCHANNEL_OPEN_EVENT_FN pChannelOpenEventProc);
typedef CTVPVIRTUALCHANNELOPEN FAR * PCTVPVIRTUALCHANNELOPEN;


typedef UINT VCAPITYPE CTVPVIRTUALCHANNELWRITE(DWORD  openHandle,
	LPVOID pData,
	ULONG  dataLength,
	LPVOID pUserData);
typedef CTVPVIRTUALCHANNELWRITE FAR * PCTVPVIRTUALCHANNELWRITE;

typedef UINT VCAPITYPE CTVPVIRTUALCHANNELREAD(DWORD  openHandle,
	LPVOID pData,
	ULONG  dataLength,
	LPVOID pUserData);
typedef CTVPVIRTUALCHANNELREAD FAR * PCTVPVIRTUALCHANNELREAD;


typedef UINT VCAPITYPE CTVPVIRTUALCHANNELCLOSE(DWORD openHandle);
typedef CTVPVIRTUALCHANNELCLOSE FAR * PCTVPVIRTUALCHANNELCLOSE;





typedef struct tagCtvpCHANNEL_ENTRY_POINTS
{
	DWORD cbSize;
	DWORD protocolVersion;
	PCTVPVIRTUALCHANNELINIT  pVirtualChannelInit;
	PCTVPVIRTUALCHANNELOPEN  pVirtualChannelOpen;
	PCTVPVIRTUALCHANNELCLOSE pVirtualChannelClose;
	PCTVPVIRTUALCHANNELWRITE pVirtualChannelWrite;
} CTVPCHANNEL_ENTRY_POINTS, FAR * PCTVPCHANNEL_ENTRY_POINTS;

typedef BOOL VCAPITYPE VIRTUALCHANNELENTRY(PCTVPCHANNEL_ENTRY_POINTS pEntryPoints);
typedef VIRTUALCHANNELENTRY FAR * PVIRTUALCHANNELENTRY;

#endif
