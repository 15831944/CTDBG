// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"

// ȫ��ͨ��
DWORD DebugMainThread = 0;


// 
// ���̣߳�����ֱ�������Ի���->�û�����
extern "C" NAKED VOID WINAPIV FsDebugMainThread( PVOID nil )
{
	DebugMainThread = GetCurrentThreadId();
	sys::fsdebug( "Debug Main TID = %d", DebugMainThread );

	//������
	FsDialogBox( hinst, MAKEINTRESOURCEA( IDD_MAIN ), 0, DebugMainWndProc );
}

//
// ����
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
	
	//��ȡһ�µ�ǰע��Ľ�����������ǵĻ�ֱ�ӵ��Դ���
	//���ʹ����һ���ļ����ж�
	GetPrivateProfileStringA( "1", "pid", "0", buf, 50, "c:\\gEt" );
	DeleteFileA( "c:\\gEt" );
	DWORD pid = strtol( buf, 0, 0 );
	sys::fsdebug( "pid = %d", pid );

	if(GetCurrentProcessId() == pid)
	{
		sys::fsdebug( "in DebugExe [%d]", GetCurrentProcessId() );

		//�����ʱ���Ŵ�����
		//��˼���������ⲻ���߳�����ʱ��
		//����ֱ��hook��Ƶ�����õ�ϵͳAPI����API����ʱ�ٵ����߳�����
		//
		//HookAndStartMainThread();	

		//ֱ���������߳�
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
			//Ĩ��PEͷ������ֹ���������ڴ�
			sys::fsdebug( "Loader Base = %X ( %02X %02X ) Erase..", hModule, GETPB( hModule ), GETPB( (DWORD)hModule + 1 ) );
			os::ctVirtualProtect( (DWORD)hModule, 0x1000 );
			*(PDWORD)(hModule) = 0;
#endif
		}
	}

	return TRUE;
}
