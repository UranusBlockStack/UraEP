

#pragma once

#if _DEBUG
#define DbgPrintf(...)  printf(__VA_ARGS__)
#else
#define DbgPrintf(...)
#endif

#include "CommonInclude/Tools/MoudlesAndPath.h"
class CLoadModules
{
#define   MAXCOUNT_TRANS_MODULES 16 // 最多管理的模块总数
#define   MAXCOUNT_APP_MODULES   256 // 最多管理的模块总数
	interface GetNameCover
	{
		virtual LPCSTR GetName() = 0;
	};
	typedef GetNameCover* (WINAPI *Fn_GetInterface)();

public:
	CLoadModules(): m_TransCount(0), m_AppCount(0)
	{
		ZeroMemory(&m_TS, sizeof(m_TS));
		ZeroMemory(&m_TS, sizeof(m_APP));
	}
	union
	{
		interface ICTEPTransferProtocolServer* m_TS[MAXCOUNT_TRANS_MODULES];		// index 0: TCP, 1:UDP
		interface ICTEPTransferProtocolClient* m_TC[MAXCOUNT_TRANS_MODULES];		// index 0: TCP, 1:UDP
	};

	interface ICTEPAppProtocol* m_APP[MAXCOUNT_APP_MODULES];

	DWORD m_TransCount;
	DWORD m_AppCount;

	enum EnMoudleType
	{
		CtepTransServer,
		CtepTransClient,
		CtepAppServer,
		CtepAppClient,
	};

	// Directory必须是模块所在目录下的子路径或者NULL，且长度必须小于MODULE_NAME_LENGTH,不能以'\'结尾
	int FindMoudleInterface(EnMoudleType eType, const WCHAR *Directory = 0)
	{
		DWORD			 iCount = 0;
		HANDLE           hDirectory = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATA  FindFileData = {0};
		WCHAR            TargetDirectory[MAX_PATH];

		if (eType == CtepTransServer || eType == CtepTransClient)
		{
			m_TransCount = 2;
			ZeroMemory(&m_TS, sizeof(m_TS));
		}
		else
		{
			ZeroMemory(&m_APP, sizeof(m_APP));
			m_AppCount = 0;
		}


		GetNameCover* pI = 0;

		if ( !GetSelfDir(TargetDirectory))
			return 0-GetLastError();
		if ( Directory)
		{
			wcscat_s(TargetDirectory, Directory);
			wcscat_s(TargetDirectory, L"\\");
		}

		LPCSTR pFuncName = 0;
		Fn_GetInterface fn = 0;
		if ( eType == CtepTransServer)
		{
			wcscat_s(TargetDirectory, L"CTEPTS*.dll");
			pFuncName = FUNC_CTEPGetInterfaceTransServer;
		}
		else if ( eType == CtepTransClient)
		{
			pFuncName = FUNC_CTEPGetInterfaceTransClient;
			wcscat_s(TargetDirectory, L"CTEPTC*.dll");
		}
		else if ( eType == CtepAppServer)
		{
			pFuncName = FUNC_CTEPGetInterfaceAppServer;
			wcscat_s(TargetDirectory, L"CTEPAS*.dll");
		}
		else if ( eType == CtepAppClient)
		{
			pFuncName = FUNC_CTEPGetInterfaceAppClient;
			wcscat_s(TargetDirectory, L"CTEPAC*.dll");
		}

		hDirectory = FindFirstFile(TargetDirectory, &FindFileData);
		if (hDirectory == INVALID_HANDLE_VALUE)
		{
			if (ERROR_FILE_NOT_FOUND != GetLastError ())
			{
				DbgPrintf ("error: %s FindFirstFile: %d\n", __FUNCTION__, GetLastError());
			}
			return 0 - GetLastError();
		}

		DbgPrintf("The first file found is %ws\n", FindFileData.cFileName);
		HMODULE hModule = NULL;

		do
		{
			WCHAR Path[MAX_PATH+1];
			GetSelfDir(Path);
			if ( Directory)
			{
				wcscat_s(Path, Directory);
				wcscat_s(Path, L"\\");
			}
			wcscat_s(Path, FindFileData.cFileName);
			DbgPrintf("Debug: %s file name: %ws\n", __FUNCTION__, Path);
			hModule = LoadLibrary(Path);
			if ( !hModule)
				continue;

			fn = (Fn_GetInterface)GetProcAddress(hModule, pFuncName);
			if ( !fn || !(pI = fn()))
			{
				FreeLibrary(hModule);
				continue;
			}

			iCount++;
			if (eType == CtepTransServer || eType == CtepTransClient)
			{
				if ( !_stricmp("TCP", pI->GetName()))
				{
					m_TS[0] = (ICTEPTransferProtocolServer*)pI;
				}
				else if ( !_stricmp("UDP", pI->GetName()))
				{
					m_TS[1] = (ICTEPTransferProtocolServer*)pI;
				}
				else
				{
					m_TS[m_TransCount++] = (ICTEPTransferProtocolServer*)pI;
				}
				DbgPrintf("Debug: %s file name:[%ws] TransName:[%s]\n\n"
					, __FUNCTION__, FindFileData.cFileName, pI->GetName());
			}
			else
			{
				m_APP[m_AppCount++] = (ICTEPAppProtocol*)pI;
				DbgPrintf("Debug: %s file name:[%ws] AppName:[%s]\n\n"
					, __FUNCTION__, FindFileData.cFileName, pI->GetName());
			}
		}
		while ( FindNextFile(hDirectory, &FindFileData));

		FindClose(hDirectory);

		if (eType == CtepTransServer || eType == CtepTransClient)
		{
			if ( iCount == 0)
			{
				m_TransCount = 0;
			}
		}

		return iCount;
	}
};




