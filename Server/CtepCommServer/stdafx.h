// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // ĳЩ CString ���캯��������ʽ��

#include <atlbase.h>
#include <atlstr.h>

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#pragma warning(push, 4)
#pragma warning(disable:4200)// ʹ���˷Ǳ�׼��չ : �ṹ/�����е����С����, ��UDT������СΪ�������ʱ���޷����ɸ��ƹ��캯���򸱱���ֵ�����
#pragma warning(disable:4481)// ʹ���˷Ǳ�׼��չ: ����д˵������override��
#pragma warning(disable:4482)// ʹ���˷Ǳ�׼��չ: �޶�����ʹ����ö�١�EmPacketOperationType��

#include <windows.h>
#include "Log4Cpp_Lib/log4cpplib.h"
//#include "CommonInclude/Log4CppLibSimple.h"
using namespace _Log4Cpp;

#include "CommonInclude/CommonImpl.h"

