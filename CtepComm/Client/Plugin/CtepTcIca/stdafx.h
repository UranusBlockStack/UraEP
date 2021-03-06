// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//
#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息
// Windows 头文件:
#include <windows.h>

#include <atlbase.h>
#include <atlstr.h>
#include <WinError.h>

// TODO: 在此处引用程序需要的其他头文件
#include "Log4Cpp_Lib/Log4CppLib.h"

#include "CommonInclude/CommonImpl.h"

// 加载ICA相关资源
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