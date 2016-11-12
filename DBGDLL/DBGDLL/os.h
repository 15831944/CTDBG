//
// һЩWin32ϵͳ���õķ�װ
//
// 1��Ϊ�˵���ĳЩ�򻯵�ϵͳapi
// 2����ֹ�����Խ��̵ļ��
//
#pragma once

#include <stdio.h>
#include <windows.h>

//
#define GETPD(x)			(*(PDWORD)(x))
#define GETPF(x)			(*(PFLOAT)(x))
#define GETPW(x)			(*(PWORD)(x))
#define GETPB(x)			(*(PBYTE)(x))
#define NAKED				 __declspec(naked)

//
namespace os
{
	//
	// Win32.Sleep
	VOID Sleep( DWORD dwMilliseconds );

	//
	// Win32.MessageBoxA
	VOID MsgBox( const char *fmt, ... );

	//
	// ��window��ǰ
	void SetActive( HWND m_hWnd );


	//
	// ��ȡ��ִ���ļ���oep                                     
	DWORD GetExeEntryPoint( LPCTSTR szFileName );


	//
	// VirtualProtect
	BOOL WINAPI ctVirtualProtect( DWORD lpAddress, DWORD dwSize );

	//
	// ʹ��0xE9�ı���������
	VOID ctHookJmp( DWORD hookAddr, void* hkProc );


	//
	// �����̺߳���
	// �����߳�HANDLE   bCloseHandle=0 ʱ
	// �����߳�tid	   bCloseHandle=1 ʱ
	HANDLE ctCreateThread( PVOID StartAddress, BOOL bCloseHandle = FALSE );


	//
	// ʵ��GetProcaddress
	DWORD ctGetProcAddress( HMODULE hModule, char *FuncName );

	// 
	// ����LdrLoadDllʵ�ּ���dll
	HMODULE ctLoadDll( PCH lpcbFileName );

	//
	// Hook os-api by addr
	//	@param-3�� ��Ҫ������ԭʼ�����ֽ��룬Ĭ����5
	//	@return�� ���ؿ��Ե���ԭʼ�����ĵ�ַ
	//
	PBYTE FsHookFunc( PBYTE ori_func, PBYTE hkFunc, int hookbytes );
	//
	// Hook os-api by name
	//	@param-3�� ��Ҫ������ԭʼ�����ֽ��룬Ĭ����5
	//	@return�� ���ؿ��Ե���ԭʼ�����ĵ�ַ
	//
	PBYTE FsHookFunc( PCH dllname, PCH funcname, PBYTE hkFunc, int hookbytes );

	//
	// ���ַ�-խ�ַ� ����ת�� 
	BOOL MByteToWChar( LPCSTR lpcszStr, LPWSTR lpwszStr, DWORD dwSize );
	BOOL WCharToMByte( LPCWSTR lpcwszStr, LPSTR lpszStr, DWORD dwSize );

	//
	// ��ȡѡ�е�List���Զ�isubItem����ʾ�ַ���
	BOOL ListView_GetSelectStr( HWND hList, int isubItem, OUT PCH retbuf, IN int retbuflen );
	//
	// ��ȡѡ�е���һ��
	BOOL ListView_GetSelectNextStr( HWND hList, int isubItem, OUT PCH retbuf, IN int retbuflen );

	//
	// ListBox function
	VOID ListBox_InsertString( HWND listwnd, char* buff );
	VOID ListBox_AddString( HWND listwnd, char* buff );
	VOID ListBox_InsertDword( HWND listwnd, DWORD db );
	VOID ListBox_DelAll( HWND listwnd );

};