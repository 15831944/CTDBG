//
// 一些Win32系统调用的封装
//
// 1：为了调用某些简化的系统api
// 2：防止被调试进程的检测
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
	// 将window置前
	void SetActive( HWND m_hWnd );


	//
	// 获取可执行文件的oep                                     
	DWORD GetExeEntryPoint( LPCTSTR szFileName );


	//
	// VirtualProtect
	BOOL WINAPI ctVirtualProtect( DWORD lpAddress, DWORD dwSize );

	//
	// 使用0xE9改变运行流程
	VOID ctHookJmp( DWORD hookAddr, void* hkProc );


	//
	// 启动线程函数
	// 返回线程HANDLE   bCloseHandle=0 时
	// 返回线程tid	   bCloseHandle=1 时
	HANDLE ctCreateThread( PVOID StartAddress, BOOL bCloseHandle = FALSE );


	//
	// 实现GetProcaddress
	DWORD ctGetProcAddress( HMODULE hModule, char *FuncName );

	// 
	// 调用LdrLoadDll实现加载dll
	HMODULE ctLoadDll( PCH lpcbFileName );

	//
	// Hook os-api by addr
	//	@param-3： 需要拷贝的原始函数字节码，默认是5
	//	@return： 返回可以调用原始函数的地址
	//
	PBYTE FsHookFunc( PBYTE ori_func, PBYTE hkFunc, int hookbytes );
	//
	// Hook os-api by name
	//	@param-3： 需要拷贝的原始函数字节码，默认是5
	//	@return： 返回可以调用原始函数的地址
	//
	PBYTE FsHookFunc( PCH dllname, PCH funcname, PBYTE hkFunc, int hookbytes );

	//
	// 宽字符-窄字符 互相转换 
	BOOL MByteToWChar( LPCSTR lpcszStr, LPWSTR lpwszStr, DWORD dwSize );
	BOOL WCharToMByte( LPCWSTR lpcwszStr, LPSTR lpszStr, DWORD dwSize );

	//
	// 获取选中的List的自定isubItem的显示字符串
	BOOL ListView_GetSelectStr( HWND hList, int isubItem, OUT PCH retbuf, IN int retbuflen );
	//
	// 获取选中的下一条
	BOOL ListView_GetSelectNextStr( HWND hList, int isubItem, OUT PCH retbuf, IN int retbuflen );

	//
	// ListBox function
	VOID ListBox_InsertString( HWND listwnd, char* buff );
	VOID ListBox_AddString( HWND listwnd, char* buff );
	VOID ListBox_InsertDword( HWND listwnd, DWORD db );
	VOID ListBox_DelAll( HWND listwnd );

};