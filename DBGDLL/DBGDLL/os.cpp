//
// һЩWin32ϵͳ���õķ�װ
//
// 1��Ϊ�˵���ĳЩ�򻯵�ϵͳapi
// 2����ֹ�����Խ��̵ļ��
//
#include "stdafx.h"
#include <stdio.h>
#include <windows.h>

//ȫ��
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
	// ��window��ǰ
	void SetActive( HWND m_hWnd )
	{
		SetWindowPos( m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE ); //����Z-Order
		SetWindowPos( m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE ); //��ԭZ-Order
		SetForegroundWindow( m_hWnd );
		Sleep( 50 );
	}


	//
	// ��ȡ��ִ���ļ���oep                                     
	DWORD GetExeEntryPoint( LPCTSTR szFileName )
	{
		HANDLE hFile;
		HANDLE hMapping;
		PVOID pBaseAddr;
		if((hFile = CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0 )) == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}
		//�����ڴ�ӳ���ļ�
		if(!(hMapping = CreateFileMapping( hFile, 0, PAGE_READONLY | SEC_COMMIT, 0, 0, 0 )))
		{
			CloseHandle( hFile );
			return FALSE;
		}
		//���ļ�ӳ�����pBaseAddr
		if(!(pBaseAddr = MapViewOfFile( hMapping, FILE_MAP_READ, 0, 0, 0 )))
		{
			CloseHandle( hMapping );
			CloseHandle( hFile );
			return FALSE;
		}

		IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)pBaseAddr;
		IMAGE_NT_HEADERS *nt_header = (IMAGE_NT_HEADERS *)((DWORD)pBaseAddr + dos_header->e_lfanew);

		//��ڵ�ַ+ģ���ַ���ǵ�ǰģ���OEP
		DWORD dwOEP = nt_header->OptionalHeader.AddressOfEntryPoint + nt_header->OptionalHeader.ImageBase;

		//����ڴ�ӳ��͹ر��ļ�
		UnmapViewOfFile( pBaseAddr );
		CloseHandle( hMapping );
		CloseHandle( hFile );
		return dwOEP;
	}

	//
	// virtualprotect��ʵ��
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
	// ʹ��0xE9�ı���������
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
	// �����̺߳���
	// �����߳�HANDLE   bCloseHandle=0 ʱ  Ĭ��
	// �����߳�tid	   bCloseHandle=1 ʱ
	HANDLE ctCreateThread( PVOID StartAddress, BOOL bCloseHandle )
	{
		DWORD tid = 0;
#if ANTI_OSAPI_DETECTION
		//��thread��ʼ��ַ���������ط����˴�������ntdll�еĿռ��� (only win7x64)
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
		//�޸���ת
		memcpy( (PVOID)_v_start, jmpcodes, 7 );
		//���԰ѵ���FsCreateThread��������ŵ�ntdll�ռ���ȥ����
		//�����Ļ� ���÷�ҲҪ��ϵͳdll�ռ��ڵ�
		HANDLE ht = CreateThread( NULL, NULL, (LPTHREAD_START_ROUTINE)_v_start, NULL, 0, &tid );
		Sleep( 350 );
		//��ԭ��ת
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
	// ʵ��GetProcaddress
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
	// ����LdrLoadDllʵ�ּ���dll
	HMODULE ctLoadDll( PCH lpcbFileName )
	{
#if ANTI_OSAPI_DETECTION
		WCHAR lpFileName[200];
		MByteToWChar( lpcbFileName, lpFileName, 200 );

		DWORD _LdrLoadDll = (DWORD)ctGetProcAddress( GetModuleHandleA( "ntdll.dll" ), "LdrLoadDll" );

		//������PBYTE ����һ��PUNICODE_STRING
		BYTE bytebuff[200];
		GETPW( bytebuff + 0 ) = wcslen( lpFileName ) * 2;
		GETPW( bytebuff + 2 ) = wcslen( lpFileName ) * 2;
		GETPD( bytebuff + 4 ) = (DWORD)bytebuff + 8;			//ֱ�Ӹ���һ����ַ+4��str
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
	//	@param-3�� ��Ҫ������ԭʼ�����ֽ��룬Ĭ����5
	//	@return�� ���ؿ��Ե���ԭʼ�����ĵ�ַ
	//
	PBYTE FsHookFunc( PBYTE ori_func, PBYTE hkFunc, int hookbytes )
	{
		if(ori_func && !IsBadReadPtr( ori_func, 5 ))
		{
			PBYTE pbackfunc = (PBYTE)VirtualAlloc( NULL, 20, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
			if(pbackfunc)
			{
				//����һ����ת��ȥ��
				//copy  ͷx���ֽ�  һ�����еĺ������ǿ��Ե� 
				memcpy( pbackfunc, (PVOID)ori_func, hookbytes );

				//ֱ������+xλ��ȥ
				pbackfunc[hookbytes] = 0xE9;
				//jmp���ĵ�ַ = ��ǰ��ַ + GETPD(��ǰ��ַ+1) +5
				//��֮ GETPD(��ǰ��ַ+1) = jmp���ĵ�ַ - ��ǰ��ַ - 5
				GETPD( pbackfunc + hookbytes + 1 ) = ((DWORD)ori_func + hookbytes) - ((DWORD)pbackfunc + hookbytes) - 5;

				ctHookJmp( (DWORD)ori_func, hkFunc );
				return pbackfunc;
			}
		}

		return 0;
	}
	//
	// Hook os-api by name
	//	@param-3�� ��Ҫ������ԭʼ�����ֽ��룬Ĭ����5
	//	@return�� ���ؿ��Ե���ԭʼ�����ĵ�ַ
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
				os::MsgBox( "cannot load module�� %s", dllname );
				return 0;
			}
		}

		return FsHookFunc( (PBYTE)ctGetProcAddress( hm, funcname ), hkFunc, hookbytes );
	}

	//
	// ���ַ�-խ�ַ� ����ת�� 
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
	// ��ȡѡ�е�List���Զ�isubItem����ʾ�ַ���
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
	//��ȡѡ�е���һ��
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