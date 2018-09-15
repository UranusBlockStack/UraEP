// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//
#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  �� Windows ͷ�ļ����ų�����ʹ�õ���Ϣ
// Windows ͷ�ļ�:
#include <windows.h>

#include <atlbase.h>
#include <atlstr.h>
#include <WinError.h>

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#include "Log4Cpp_Lib/Log4CppLib.h"

#include "CommonInclude/CommonImpl.h"

// ����ICA�����Դ
#include "citrix.h"
#include "ica.h"

#ifdef AF_IPX
#undef AF_IPX
#endif
#include "ica-c2h.h"

#include "vd.h"
#include "wdapi.h"
#include "clterr.h"
//#include "ctxdebug.h"

#include "miapi.h"
#include "vdapi.h"
#include "cmacro.h"

#pragma comment(lib, "lib/clibdll.lib")
#pragma comment(lib, "lib/vdapi.lib")