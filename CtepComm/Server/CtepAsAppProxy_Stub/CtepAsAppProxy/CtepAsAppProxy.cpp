// CtepAsAppProxy.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"


#include "CtepAsAppProxy.h"
#include "CommonInclude/IniFile/Cipher.cpp"
#include "CommonInclude/IniFile/IniFile.cpp"
#include "CommonInclude/PipeImpl/ServerNamedPipe.cpp"

CCtepAsAppProxy g_One;

ICTEPAppProtocol* WINAPI CTEPGetInterfaceAppServer()
{
	return (ICTEPAppProtocol*)&g_One;
}




