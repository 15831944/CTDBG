//
//一些通用函数的定义
#include "stdafx.h"

//test
#define FsDebugLogPath  "d:\\dbg_log.txt"

namespace sys
{
	//
	// fsdebug LOG 记录到文件
	void __cdecl fsdebug( const char * fmt, ... )
	{
		va_list		va_alist;
		char		buf[256];
		char		logbuf[1024];
		FILE*		file;
		struct tm*	current_tm;
		time_t		current_time;

		PCH  sname = buf;
		GetModuleFileNameA( 0, buf, 260 );
		for(int len = lstrlenA( buf ); len > 0; len--)
		{
			if(buf[len] == '\\')
			{
				len++;
				sname = buf + len;
				break;
			}
		}

		time( &current_time );
		current_tm = localtime( &current_time );
		sprintf( logbuf, "//%02d:%02d:%02d [%04d] %s :", current_tm->tm_hour,
				 current_tm->tm_min, current_tm->tm_sec, GetCurrentProcessId(), sname );
		va_start( va_alist, fmt );
		vsprintf( buf, fmt, va_alist );
		va_end( va_alist );

		strcat( logbuf, buf );
		strcat( logbuf, "\n" );

		if((file = fopen( FsDebugLogPath, "a+" )) != NULL)
		{
			fputs( logbuf, file );
			fclose( file );
		}
	}
}
