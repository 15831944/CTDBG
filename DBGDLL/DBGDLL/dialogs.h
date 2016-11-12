#pragma once

extern DWORD DebugMainThread;

extern HINSTANCE hinst;
extern HWND MainWnd;
extern HWND ASMlistWnd;
extern HWND RegisterListWnd;
extern HWND FlagListWnd;
extern HWND StackListWnd;
extern HWND HexEditWnd;

extern BOOL IsRunAWnd;
extern BOOL InDebugProcess;

extern INT  iListShowType;			//����ǵ�ǰLIstView��ʾ������ 
#define ListType_ASM	1			//�����
#define ListType_MOD	2			//ģ��
#define ListType_BP		3			//�ϵ�

BOOL CALLBACK DebugMainWndProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
VOID FsDialogBox( HINSTANCE hinst, LPSTR lptmp, HWND parentWnd, DLGPROC lpWndProc );
void debug( const char *fmt, ... );
VOID AddHardPoint( HWND hList, DWORD att, DWORD len, DWORD onlytid = 0 );