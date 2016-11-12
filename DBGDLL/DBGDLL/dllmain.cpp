// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

// 全局通用
DWORD DebugMainThread = 0;


// 
// 主线程，用于直接启动对话框->用户操作
extern "C" NAKED VOID WINAPIV FsDebugMainThread( PVOID nil )
{
	DebugMainThread = GetCurrentThreadId();
	sys::fsdebug( "Debug Main TID = %d", DebugMainThread );

	//主窗口
	FsDialogBox( hinst, MAKEINTRESOURCEA( IDD_MAIN ), 0, DebugMainWndProc );
}

//
// 分流
DWORD WINAPI InitDll( HMODULE hModule )
{
	/*
	char buf[260];
	PCH  sname = buf;
	GetModuleFileNameA( 0, buf, 260 );
	sys::fsdebug( buf );


	for(int len = lstrlenA( buf ); len>0; len--)
	{
		if(buf[len] == '\\')
		{
			len++;
			sname = buf + len;
			break;
		}
	}
	
	//读取一下当前注入的进程名，如果是的话直接调试处理
	//随便使用了一个文件来判断
	GetPrivateProfileStringA( "1", "pid", "0", buf, 50, "c:\\gEt" );
	DeleteFileA( "c:\\gEt" );
	DWORD pid = strtol( buf, 0, 0 );
	sys::fsdebug( "pid = %d", pid );

	if(GetCurrentProcessId() == pid)
	{
		sys::fsdebug( "in DebugExe [%d]", GetCurrentProcessId() );

		//这个暂时不放代码了
		//意思是如果被检测不让线程启动时候
		//可以直接hook了频繁调用的系统API，等API运行时再调用线程启动
		//
		//HookAndStartMainThread();	

		//直接启动主线程
		os::ctCreateThread( FsDebugMainThread );
		return 1;
	}
	*/

	os::ctCreateThread( FsDebugMainThread );
	return 0;
}

/*************************************** DLL ENTRY **********************************************/
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	if(ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		hinst = hModule;

		//LoadLibraryA("comctl32.dll");
		if(InitDll( hModule ) == 1)				
		{
#if ANTI_OSAPI_DETECTION
			//抹掉PE头部，防止暴力搜索内存
			sys::fsdebug( "Loader Base = %X ( %02X %02X ) Erase..", hModule, GETPB( hModule ), GETPB( (DWORD)hModule + 1 ) );
			os::ctVirtualProtect( (DWORD)hModule, 0x1000 );
			*(PDWORD)(hModule) = 0;
#endif
		}
	}

	return TRUE;
}
