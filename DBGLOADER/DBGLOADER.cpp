// DBGLOADER.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <windows.h>

int main()
{
	char* dllname = "D:\\����\\CtDbg\\Debug\\DBGDLL.dll";
	puts( dllname );

	LoadLibraryA( dllname );
	
	MessageBoxA( 0, "1", 0, 0 );
	MessageBoxA( 0, "2", 0, 0 );
	MessageBoxA( 0, "3", 0, 0 );
	Sleep( INFINITE );
    return 0;
}

