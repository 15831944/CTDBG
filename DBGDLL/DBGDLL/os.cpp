//
// 一些Win32系统调用的封装
//
// 1：为了调用某些简化的系统api
// 2：防止被调试进程的检测
//
#include "stdafx.h"
#include <stdio.h>
#include <windows.h>

//全局
#define GETPD(x)			(*(PDWORD)(x))
#define GETPF(x)			(*(PFLOAT)(x))
#define GETPW(x)			(*(PWORD)(x))
#define GETPB(x)			(*(PBYTE)(x))
#define NAKED				 __declspec(naked)

namespace os
{
	//
	// Win32.Sleep
	VOID Sleep( DWORD dwMilliseconds )
	{
		HANDLE se = CreateEventA( NULL, FALSE, 0, 0 );
		WaitForSingleObject( se, dwMilliseconds );
		CloseHandle( se );
	}

	//
	// Win32.MessageBoxA
	VOID MsgBox( const char *fmt, ... )
	{
		int nBuf;
		char szBuffer[1000];
		va_list args;

		va_start( args, fmt );
		nBuf = _vsnprintf_s( szBuffer, 1000, fmt, args );
		va_end( args );

		MessageBoxA( 0, szBuffer, "info", MB_ICONINFORMATION );
	}

	//
	// 将window置前
	void SetActive( HWND m_hWnd )
	{
		SetWindowPos( m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE ); //设置Z-Order
		SetWindowPos( m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE ); //还原Z-Order
		SetForegroundWindow( m_hWnd );
		Sleep( 50 );
	}


	//
	// 获取可执行文件的oep                                     
	DWORD GetExeEntryPoint( LPCTSTR szFileName )
	{
		HANDLE hFile;
		HANDLE hMapping;
		PVOID pBaseAddr;
		if((hFile = CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0 )) == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}
		//创建内存映射文件
		if(!(hMapping = CreateFileMapping( hFile, 0, PAGE_READONLY | SEC_COMMIT, 0, 0, 0 )))
		{
			CloseHandle( hFile );
			return FALSE;
		}
		//把文件映像存入pBaseAddr
		if(!(pBaseAddr = MapViewOfFile( hMapping, FILE_MAP_READ, 0, 0, 0 )))
		{
			CloseHandle( hMapping );
			CloseHandle( hFile );
			return FALSE;
		}

		IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)pBaseAddr;
		IMAGE_NT_HEADERS *nt_header = (IMAGE_NT_HEADERS *)((DWORD)pBaseAddr + dos_header->e_lfanew);

		//入口地址+模块地址就是当前模块的OEP
		DWORD dwOEP = nt_header->OptionalHeader.AddressOfEntryPoint + nt_header->OptionalHeader.ImageBase;

		//清除内存映射和关闭文件
		UnmapViewOfFile( pBaseAddr );
		CloseHandle( hMapping );
		CloseHandle( hFile );
		return dwOEP;
	}

	//
	// virtualprotect的实现
	NAKED BOOL __stdcall _VirtualProtect( DWORD addr, DWORD len )
	{
		_asm
		{
			push ebp
			mov  ebp, esp
			sub  esp, 4

			lea  eax, [ebp - 4]
			push eax
			push 0x40
			lea  eax, len
			push eax
			lea  eax, addr
			push eax
			push - 1
			call _ZwVp

			mov  esp, ebp
			pop  ebp
			retn 8

			_ZwVp:
			mov     eax, 0x4D
				xor ecx, ecx
				lea     edx, dword ptr[esp + 4]
				call    dword ptr fs : [0xC0]
				add     esp, 4
				retn    0x14
		}

	}

	//
	// VirtualProtect
	BOOL WINAPI ctVirtualProtect( DWORD lpAddress, DWORD dwSize )
	{
#if ANTI_OSAPI_DETECTION
		return _VirtualProtect( lpAddress, dwSize );
#else
		DWORD op;
		return VirtualProtect( (PVOID)lpAddress, dwSize, PAGE_EXECUTE_READWRITE, &op );
#endif
	}

	//
	// 使用0xE9改变运行流程
	VOID ctHookJmp( DWORD hookAddr, void* hkProc )
	{
		if(hookAddr)
		{
			ctVirtualProtect( hookAddr, 5 );
			*(PBYTE)hookAddr = 0xE9;
			*(PDWORD)(hookAddr + 1) = (DWORD)hkProc - 5 - hookAddr;
		}
	}


	//
	// 启动线程函数
	// 返回线程HANDLE   bCloseHandle=0 时  默认
	// 返回线程tid	   bCloseHandle=1 时
	HANDLE ctCreateThread( PVOID StartAddress, BOOL bCloseHandle )
	{
		DWORD tid = 0;
#if ANTI_OSAPI_DETECTION
		//将thread开始地址放在其他地方，此处放在了ntdll中的空间中 (only win7x64)
		DWORD _v_start = ctGetProcAddress( GetModuleHandleA( "ntdll.dll" ), "RtlDosSearchPath_Ustr" ) + 0x69A;
		ctVirtualProtect( _v_start, 10 );
		//mov eax,xxxx   jmp eax
		BYTE jmpcodes[7] = {0};
		jmpcodes[0] = 0xB8;
		GETPD( jmpcodes + 1 ) = (DWORD)StartAddress;
		jmpcodes[5] = 0xFF;
		jmpcodes[6] = 0xE0;
		BYTE bakcodes[7] = {0};
		memcpy( bakcodes, (PVOID)_v_start, 7 );
		//修改跳转
		memcpy( (PVOID)_v_start, jmpcodes, 7 );
		//可以把调用FsCreateThread这个函数放到ntdll空间内去运行
		//这样的话 调用方也要在系统dll空间内的
		HANDLE ht = CreateThread( NULL, NULL, (LPTHREAD_START_ROUTINE)_v_start, NULL, 0, &tid );
		Sleep( 350 );
		//还原跳转
		memcpy( (PVOID)_v_start, bakcodes, 7 );
#else
		HANDLE ht = CreateThread( NULL, NULL, (LPTHREAD_START_ROUTINE)StartAddress, NULL, 0, &tid );
#endif
		if(bCloseHandle == TRUE)
		{
			CloseHandle( ht );
			return (HANDLE)tid;
		}
		return ht;
	}


	//
	// 实现GetProcaddress
	DWORD ctGetProcAddress( HMODULE hModule, char *FuncName )
	{
#if ANTI_OSAPI_DETECTION
		DWORD retAddr = 0;
		DWORD *namerav, *funrav;
		DWORD cnt = 0;
		DWORD maxIndex, minIndex, temp, lasttemp;
		WORD *nameOrdinal;
		WORD nIndex = 0;
		int cmpresult = 0;
		char *ModuleBase = (char*)hModule;
		PIMAGE_DOS_HEADER pDosHeader;
		PIMAGE_FILE_HEADER pFileHeader;
		PIMAGE_OPTIONAL_HEADER pOptHeader;
		PIMAGE_EXPORT_DIRECTORY pExportDir;

		if(hModule == 0)
			return 0;

		pDosHeader = (PIMAGE_DOS_HEADER)ModuleBase;
		pFileHeader = (PIMAGE_FILE_HEADER)(ModuleBase + pDosHeader->e_lfanew + 4);
		pOptHeader = (PIMAGE_OPTIONAL_HEADER)((char*)pFileHeader + sizeof( IMAGE_FILE_HEADER ));
		pExportDir = (PIMAGE_EXPORT_DIRECTORY)(ModuleBase + pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		namerav = (DWORD*)(ModuleBase + pExportDir->AddressOfNames);
		funrav = (DWORD*)(ModuleBase + pExportDir->AddressOfFunctions);
		nameOrdinal = (WORD*)(ModuleBase + pExportDir->AddressOfNameOrdinals);
		if((DWORD)FuncName < 0x0000FFFF)
		{
			retAddr = (DWORD)(ModuleBase + funrav[(WORD)FuncName]);
		}
		else
		{

			maxIndex = pExportDir->NumberOfFunctions;
			minIndex = 0;
			lasttemp = 0;
			while(1)
			{
				temp = (maxIndex + minIndex) / 2;
				if(temp == lasttemp)
				{
					//Not Found!
					retAddr = 0;
					break;
				}
				cmpresult = strcmp( FuncName, ModuleBase + namerav[temp] );
				if(cmpresult < 0)
				{
					maxIndex = lasttemp = temp;
				}
				else if(cmpresult > 0)
				{
					minIndex = lasttemp = temp;
				}
				else
				{
					nIndex = nameOrdinal[temp];
					retAddr = (DWORD)(ModuleBase + funrav[nIndex]);
					break;
				}

			}
		}
		return retAddr;
#else
		return (DWORD)GetProcAddress( hModule, FuncName );
#endif
	}

	// 
	// 调用LdrLoadDll实现加载dll
	HMODULE ctLoadDll( PCH lpcbFileName )
	{
#if ANTI_OSAPI_DETECTION
		WCHAR lpFileName[200];
		MByteToWChar( lpcbFileName, lpFileName, 200 );

		DWORD _LdrLoadDll = (DWORD)ctGetProcAddress( GetModuleHandleA( "ntdll.dll" ), "LdrLoadDll" );

		//这里用PBYTE 构造一个PUNICODE_STRING
		BYTE bytebuff[200];
		GETPW( bytebuff + 0 ) = wcslen( lpFileName ) * 2;
		GETPW( bytebuff + 2 ) = wcslen( lpFileName ) * 2;
		GETPD( bytebuff + 4 ) = (DWORD)bytebuff + 8;			//直接给下一个地址+4是str
		lstrcpyW( (PWCHAR)(bytebuff + 8), lpFileName );		//UNICODE

		DWORD DllHandle;
		//LdrLoadDll(0, 0, (PUNICODE_STRING)bytebuff,(PHANDLE) &DllHandle);  
		_asm
		{
			lea  eax, DllHandle
			push eax
			lea  eax, bytebuff
			push eax
			push 0
			push 0
			mov  eax, _LdrLoadDll
			call _call_LdrLoadDll
			jmp  _over

			_call_LdrLoadDll :
			push ebp
				mov  ebp, esp
				add  eax, 5
				jmp  eax

				_over :
			mov  eax, eax
		}

		return (HMODULE)DllHandle;
#else
		return LoadLibraryA( lpcbFileName );
#endif
	}

	//
	// Hook os-api by addr
	//	@param-3： 需要拷贝的原始函数字节码，默认是5
	//	@return： 返回可以调用原始函数的地址
	//
	PBYTE FsHookFunc( PBYTE ori_func, PBYTE hkFunc, int hookbytes )
	{
		if(ori_func && !IsBadReadPtr( ori_func, 5 ))
		{
			PBYTE pbackfunc = (PBYTE)VirtualAlloc( NULL, 20, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
			if(pbackfunc)
			{
				//生成一个跳转回去的
				//copy  头x个字节  一般所有的函数都是可以的 
				memcpy( pbackfunc, (PVOID)ori_func, hookbytes );

				//直接跳到+x位置去
				pbackfunc[hookbytes] = 0xE9;
				//jmp到的地址 = 当前地址 + GETPD(当前地址+1) +5
				//反之 GETPD(当前地址+1) = jmp到的地址 - 当前地址 - 5
				GETPD( pbackfunc + hookbytes + 1 ) = ((DWORD)ori_func + hookbytes) - ((DWORD)pbackfunc + hookbytes) - 5;

				ctHookJmp( (DWORD)ori_func, hkFunc );
				return pbackfunc;
			}
		}

		return 0;
	}
	//
	// Hook os-api by name
	//	@param-3： 需要拷贝的原始函数字节码，默认是5
	//	@return： 返回可以调用原始函数的地址
	//
	PBYTE FsHookFunc( PCH dllname, PCH funcname, PBYTE hkFunc, int hookbytes )
	{
		HMODULE hm = GetModuleHandleA( dllname );
		if(hm == NULL)
		{
			hm = ctLoadDll( dllname );
			if(hm == NULL)
			{
				//test
				os::MsgBox( "cannot load module： %s", dllname );
				return 0;
			}
		}

		return FsHookFunc( (PBYTE)ctGetProcAddress( hm, funcname ), hkFunc, hookbytes );
	}

	//
	// 宽字符-窄字符 互相转换 
	BOOL MByteToWChar( LPCSTR lpcszStr, LPWSTR lpwszStr, DWORD dwSize )
	{
		DWORD dwMinSize;
		dwMinSize = MultiByteToWideChar( CP_ACP, 0, lpcszStr, -1, NULL, 0 );
		if(dwSize < dwMinSize)
			return FALSE;
		MultiByteToWideChar( CP_ACP, 0, lpcszStr, -1, lpwszStr, dwMinSize );
		return TRUE;
	}
	BOOL WCharToMByte( LPCWSTR lpcwszStr, LPSTR lpszStr, DWORD dwSize )
	{
		DWORD dwMinSize;
		dwMinSize = WideCharToMultiByte( CP_OEMCP, NULL, lpcwszStr, -1, NULL, 0, NULL, FALSE );
		if(dwSize < dwMinSize)
			return FALSE;
		WideCharToMultiByte( CP_OEMCP, NULL, lpcwszStr, -1, lpszStr, dwSize, NULL, FALSE );
		return TRUE;
	}

	//
	// 获取选中的List的自定isubItem的显示字符串
	BOOL ListView_GetSelectStr( HWND hList, int isubItem, OUT PCH retbuf, IN int retbuflen )
	{
		int sfind = ListView_GetSelectionMark( hList );
		if(sfind != -1)
		{
			ListView_GetItemText( hList, sfind, isubItem, retbuf, retbuflen );
			return 1;
		}

		return 0;
	}
	//
	//获取选中的下一条
	BOOL ListView_GetSelectNextStr( HWND hList, int isubItem, OUT PCH retbuf, IN int retbuflen )
	{
		int sfind = ListView_GetSelectionMark( hList );
		if(sfind != -1)
		{
			sfind++;
			ListView_GetItemText( hList, sfind, isubItem, retbuf, retbuflen );
			return 1;
		}

		return 0;
	}

	//
	// ListBox function
	VOID ListBox_InsertString( HWND listwnd, char* buff )
	{
		SendMessageA( listwnd, LB_INSERTSTRING, 0, (LPARAM)buff );
	}

	VOID ListBox_AddString( HWND listwnd, char* buff )
	{
		SendMessageA( listwnd, LB_ADDSTRING, 0, (LPARAM)buff );
	}

	VOID ListBox_InsertDword( HWND listwnd, DWORD db )
	{
		char buft[50];
		wsprintfA( buft, "%08X", db );
		SendMessageA( listwnd, LB_INSERTSTRING, 0, (LPARAM)buft );
	}

	VOID ListBox_DelAll( HWND listwnd )
	{
		SendMessage( listwnd, LB_RESETCONTENT, 0, 0 );
	}

};