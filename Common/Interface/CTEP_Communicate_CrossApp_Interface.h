#pragma once

// ****************************************ERROR CODE***********************************************
#define E_UNINIT			MAKE_MY_ERRCODE(0x4001)							// �ӿ�δ��ʼ��
#define E_DISCONNECTED		MAKE_WINDOWS_ERRCODE(ERROR_PIPE_NOT_CONNECTED)	// �ӿ������ѶϿ�
#define E_REFUSE_APP		MAKE_MY_ERRCODE(0x4100)							// Ӧ�����Ʊ��ܾ�
#define E_REFUSE_STATE		MAKE_MY_ERRCODE(0x4101)							// ��ǰ״̬�ܾ��˲���

//		E_INVALIDARG														// ������Ч
//		E_FAIL																// ����δ֪����
//		
//		S_OK																// ִ����ȷ
//		S_FALSE																// ִ�����ظ��Ĳ���
#define S_IO_PENDING		 ERROR_IO_PENDING								// �첽������û�����
// ****************************************ERROR CODE END*******************************************

enum CROSSAPP_STATUS
{
	error			= -1,
	none			= 0,
	initialized		= 1,
	connecting		= 2,
	working			= 3,
	closing			= 4,
};

interface ICTEPAppProtocolCrossCallBack
{
	_VIRT(CROSSAPP_STATUS) GetStatus() = 0;	// ��ȡ��ǰ���ӵ�״̬
	_VIRT_H		Initialize(LPCSTR sAppName, DWORD SessionId = (DWORD)-1) = 0;		// ��ʼ����ȡ
	_VIRT_H		Connect() = 0;						//����һ����ǰ�û���ǰӦ�õ�ͨ��(��̬���߾�̬)
	_VIRT_V		Disconnect() = 0;

	_VIRT_H		Write(__in char* buff, __in DWORD size) = 0;

	// Read�᷵�ض����İ�, ������ɺ���Ҫ����FreeBuff�ͷŶ�Ӧ�ڴ�,��ֵpOL�᷵��S_IO_PENDING, pOLΪ��ʱ��һֱ�ȴ��������ݲŷ���
	_VIRT_H		Read(__out char** buff, __out DWORD* dwRead, HANDLE* pHandleEvent = nullptr) = 0;
	_VIRT_H		FreeBuff(__in char* buff) = 0;
};

ICTEPAppProtocolCrossCallBack* CrossAppGetInstance();
void CrossAppRelease(ICTEPAppProtocolCrossCallBack* pi);






























