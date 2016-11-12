//
//调试的主对话框显示流程
#include "stdafx.h"
#include "breakpoint.h"
#include "myDiasm.h"

HWND MainWnd = 0;
HWND ASMlistWnd = 0;
HWND RegisterListWnd = 0;
HWND FlagListWnd = 0;
HWND StackListWnd = 0;
HWND HexEditWnd = 0;

HINSTANCE hinst = 0;
INT  iListShowType = 0;
BOOL IsRunAWnd = FALSE;

/************************************************************************/
//设置debug输出
//输出到对话框界面中
/************************************************************************/
void debug( const char *fmt, ... )
{
	int nBuf;
	char szBuffer[1000];
	va_list args;

	va_start( args, fmt );
	nBuf = _vsnprintf_s( szBuffer, 1000, fmt, args );
	va_end( args );

	HWND debuglist = GetDlgItem( MainWnd, IDC_LISTLOG );
	if(debuglist)
		SendMessageA( debuglist, LB_INSERTSTRING, 0, (LPARAM)szBuffer );
}

/************************************************************************/
/* 硬件断点针对thread
/************************************************************************/
BOOL CALLBACK ShdThreadWndProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	char buftmp[100] = {0};
	char buf[100];

	switch(uMsg)
	{
		case WM_INITDIALOG:
			SetWindowTextA( hDlg, "Input Tid :" );
			SetTimer( hDlg, 0x888, 50, NULL );
			return 1;

		case WM_TIMER:
			if(GetKeyState( VK_RETURN ) < 0)  //ENTER键被按下
			{
				GetDlgItemTextA( hDlg, IDC_EDIT2, buf, 10 );
				DWORD tid = strtol( buf, 0, 0 );

				if(tid)
				{
					AddHardPoint( ASMlistWnd, 0, 1, tid );

					wsprintfA( buf, "线程[%d]设置硬件断点完成", tid );
					debug( buf );

					DestroyWindow( hDlg );
					return 1;
				}
			}

			break;

		case WM_CLOSE:
			DestroyWindow( hDlg );
			return 1;
	}

	return 0;
}
VOID ShowShdThreadWnd( HWND hDlg )
{
	FsDialogBox( hinst, MAKEINTRESOURCE( IDD_JMPWND ), hDlg, ShdThreadWndProc );
}


/************************************************************************/
/* 条件断点对话框
/************************************************************************/
BOOL bTJ_TIMER_USE = FALSE;
BOOL CALLBACK TJWndProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	char buftmp[100] = {0};
	char buf[100];

	switch(uMsg)
	{
		case WM_INITDIALOG:
			IsRunAWnd = 1;

			//当前选中行的地址
			if(os::ListView_GetSelectStr( ASMlistWnd, 0, buf, 50 ))
				SetDlgItemTextA( hDlg, IDC_EDIT4, buf );

			SendDlgItemMessageA( hDlg, IDC_CHECK1, BM_SETCHECK, BST_CHECKED, 0 );
			SetTimer( hDlg, 0x888, 50, NULL );
			return 1;

		case WM_TIMER:
			if(bTJ_TIMER_USE == FALSE)
			{
				if(GetKeyState( VK_ESCAPE ) < 0)  //ESC键被按下
				{
					IsRunAWnd = 0;
					DestroyWindow( hDlg );
				}
				if(GetKeyState( VK_RETURN ) < 0)  //ENTER键被按下
				{
					bTJ_TIMER_USE = TRUE;

					if(bp::SetBreakTj( hDlg ) == TRUE)
					{
						MessageBoxW( hDlg, L"设置条件断点完成！", L"", MB_ICONINFORMATION );
						IsRunAWnd = 0;
						DestroyWindow( hDlg );
						return 1;
					}
					else
					{
						MessageBoxA( hDlg, "错误的参数", "", MB_ICONSTOP );
						Sleep( 100 );
					}

					bTJ_TIMER_USE = FALSE;
				}
			}

			break;

		case WM_CLOSE:
			IsRunAWnd = 0;
			DestroyWindow( hDlg );
			return 1;
	}

	return 0;
}
VOID ShowTJWnd( HWND hDlg )
{
	bTJ_TIMER_USE = FALSE;
	FsDialogBox( hinst, MAKEINTRESOURCE( IDD_TJ ), hDlg, TJWndProc );
}


/************************************************************************/
/* 修改当前选中地址的值 寄存器/堆栈
/************************************************************************/
DWORD ChangeAddress = 0;
BOOL CALLBACK ChangeWndProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	DWORD tmp;
	char buf[100] = {0};

	switch(uMsg)
	{
		case WM_INITDIALOG:
			IsRunAWnd = 1;
			wsprintfA( buf, "修改地址 ： %X", ChangeAddress );
			SetWindowTextA( hDlg, buf );
			return 1;

		case WM_COMMAND:
			if(LOWORD( wParam ) == IDOK)
			{
				GetDlgItemTextA( hDlg, IDC_EDIT1, buf, 100 );
				if(buf[0])
				{
					tmp = strtol( buf, 0, 16 );
					GETPD( ChangeAddress ) = tmp;

					//修改完刷新一次stacklist
					bp::ShowStack( last_StackStartAddr );

					IsRunAWnd = 0;
					DestroyWindow( hDlg );
				}
			}
			else if(LOWORD( wParam ) == IDCANCEL)
			{
				IsRunAWnd = 0;
				DestroyWindow( hDlg );
			}
			return 1;

		case WM_CLOSE:
			IsRunAWnd = 0;
			DestroyWindow( hDlg );
			return 1;
	}

	return 0;
}
VOID ShowChangeValueWnd( HWND hlist )
{
	//先获取要修改的地址
	//如果修改的是寄存器就得在断下的时候修改！
	char bufaddr[50] = {0};
	DWORD addr = 0;

	if(hlist == RegisterListWnd)
	{
		//去修改寄存器  只能暂停状态下进行
		//...
	}
	else if(hlist == StackListWnd)
	{
		int sfind = ListView_GetSelectionMark( StackListWnd );
		if(sfind != -1)
			addr = last_StackStartAddr + sfind * 4;
	}

	if(IsRunAWnd == 0)
	{
		if(addr == 0 || IsBadReadPtr( (PVOID)addr, 1 ) == TRUE)
			return;

		ChangeAddress = addr;
		FsDialogBox( hinst, MAKEINTRESOURCE( IDD_CHANGBOX ), MainWnd, ChangeWndProc );
	}
}

/************************************************************************/
/* 汇编当前地址对话框
/************************************************************************/
BOOL CALLBACK AsmWndProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	char buftmp[100] = {0};
	char buf[100];

	switch(uMsg)
	{
		case WM_INITDIALOG:
			IsRunAWnd = 1;
			return 1;

		case WM_COMMAND:
			if(LOWORD( wParam ) == IDOK)
			{
				GetDlgItemTextA( hDlg, IDC_EDIT2, buf, 100 );
				if(FsAsmWriteAddresss( buf, buftmp ) == FALSE)
				{
					SetDlgItemTextA( hDlg, IDC_ASMLOG, buftmp );
				}
				else
				{
					IsRunAWnd = 0;
					DestroyWindow( hDlg );
				}
			}
			else if(LOWORD( wParam ) == IDCANCEL)
			{
				IsRunAWnd = 0;
				DestroyWindow( hDlg );
			}
			return 1;

		case WM_CLOSE:
			IsRunAWnd = 0;
			DestroyWindow( hDlg );
			return 1;
	}

	return 0;
}
VOID ShowASMWnd( HWND hDlg )
{
	if(IsRunAWnd == 0)
	{
		FsDialogBox( hinst, MAKEINTRESOURCE( IDD_ASMWND ), hDlg, AsmWndProc );
	}
}

/************************************************************************/
/* 跳转到对话框
/************************************************************************/
#define JMP_ASMTYPE		0
#define JMP_STACKTYPE	1
#define JMP_MEMORYTYPE	2
int JmpWndType;

BOOL CALLBACK JmpWndProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	char buf[100];
	BOOL bclose = 0;
	DWORD tmpAddr = 0;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			IsRunAWnd = 1;

			switch(JmpWndType)
			{
				case JMP_ASMTYPE:
					SetWindowTextA( hDlg, "跳转到汇编中" );
					break;
				case JMP_STACKTYPE:
					SetWindowTextA( hDlg, "跳转到堆栈中" );
					break;
				case JMP_MEMORYTYPE:
					SetWindowTextA( hDlg, "跳转到内存中" );
					break;
			}

			//设置一个timer来获取enter消息
			SetTimer( hDlg, 888, 50, NULL );
			return 1;

		case WM_TIMER:
			if(GetKeyState( VK_ESCAPE ) < 0)  //ESC键被按下
			{
				IsRunAWnd = 0;
				DestroyWindow( hDlg );
			}
			if(GetKeyState( VK_RETURN ) < 0)  //ENTER键被按下
			{
				buf[0] = 0;
				GetDlgItemTextA( hDlg, IDC_EDIT2, buf, 100 );

				if(buf[0])
				{
					//显示
					switch(JmpWndType)
					{
						case JMP_ASMTYPE:
							bclose = FsDisasm( buf );
							break;
						case JMP_STACKTYPE:
							tmpAddr = strtol( buf, 0, 16 );
							if(tmpAddr != 0 && IsBadReadPtr( (PVOID)tmpAddr, 1 ) == FALSE)
							{
								bclose = 1;
								bp::ShowStack( tmpAddr );
							}
							break;
						case JMP_MEMORYTYPE:
							tmpAddr = strtol( buf, 0, 16 );
							if(tmpAddr != 0 && IsBadReadPtr( (PVOID)tmpAddr, 1 ) == FALSE)
							{
								bclose = 1;
								GotoHexEdit( tmpAddr );
							}
							break;
					}

					//关闭
					if(bclose)
					{
						IsRunAWnd = 0;
						DestroyWindow( hDlg );
					}
				}

				SetDlgItemTextA( hDlg, IDC_JMPLOG, "跳转到地址出错" );
			}
			break;

		case WM_CLOSE:
			IsRunAWnd = 0;
			DestroyWindow( hDlg );
			return 1;
	}

	return 0;
}
VOID ShowJmpWnd( HWND hDlg )
{
	if(GetFocus() == ASMlistWnd)
		JmpWndType = JMP_ASMTYPE;
	else if(GetFocus() == StackListWnd)
		JmpWndType = JMP_STACKTYPE;
	else if(GetFocus() == HexEditWnd)
		JmpWndType = JMP_MEMORYTYPE;
	else
		return;

	FsDialogBox( hinst, MAKEINTRESOURCE( IDD_JMPWND ), hDlg, JmpWndProc );
}


/************************************************************************/
/* 显示断点的list处理
/************************************************************************/
VOID ShowAllBreakPoint()
{
	//上次的显示模式不是反汇编则重新初始化这个
	if(iListShowType != ListType_BP)
	{
		//把之前的都删掉  删除10次第一列 就是把列都删掉
		for(int i = 0; i < 10; i++)
			ListView_DeleteColumn( ASMlistWnd, 0 );

		//把ListView初始化
		ListView_SetExtendedListViewStyleEx( ASMlistWnd, 0, LVS_EX_FULLROWSELECT );
		LV_COLUMN colmn = {0};
		colmn.mask = LVCF_WIDTH | LVCF_TEXT;

		colmn.pszText = "长度";
		colmn.cx = 200;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "属性";
		colmn.cx = 300;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "地址 (DEL 删除断点)";
		colmn.cx = 200;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
	}

	//设定当前的显示模式
	iListShowType = ListType_BP;
	//刷新一下
	ListView_SetItemCount( ASMlistWnd, 4 );
}


VOID AddHardPoint( HWND hList, DWORD att, DWORD len, DWORD onlytid )
{
	char bufaddr[20] = {0};
	if(os::ListView_GetSelectStr( ASMlistWnd, 0, bufaddr, 20 ) == 0)
		return;

	DWORD addr = strtol( bufaddr, 0, 16 );
	bp::SetHardBP( addr, att, len, onlytid );
}


/************************************************************************/
/* 相关初始化
/************************************************************************/
//Debug
VOID InitDebugSomething()
{
	//接管异常处理
	bp::HookKiUserExceptionDispatcher();
}

//s1==TRUE时第一项是 $+xx 这样的pos 而不是地址
BOOL bLastPos = TRUE;
BOOL bInitStackList = FALSE;

VOID InitStackList()
{
	//是否显示位置 默认第一次是不显示
	bLastPos = !bLastPos;

	LV_COLUMN colmn = {0};
	LV_ITEM item = {0};
	char buf[200];

	if(bInitStackList == FALSE)
	{
		bInitStackList = TRUE;

		//对StackListWnd的初始化
		HIMAGELIST himage = ImageList_Create( 0, 18, ILC_COLOR24 | ILC_MASK, 1, 1 );
		ListView_SetImageList( StackListWnd, himage, LVSIL_SMALL );				//改格子高度

		ListView_SetExtendedListViewStyleEx( StackListWnd, 0, LVS_EX_FULLROWSELECT );
		colmn.mask = LVCF_WIDTH | LVCF_TEXT;
		colmn.pszText = "Value";
		colmn.cx = 140;
		ListView_InsertColumn( StackListWnd, 0, &colmn );
		colmn.pszText = "Addr";
		colmn.cx = 80;
		ListView_InsertColumn( StackListWnd, 0, &colmn );

		//全初始化成0
		for(int i = STACK_SIZE; i >= 0; i--)
		{
			item.mask = LVIF_TEXT;
			item.iItem = 0;
			item.pszText = buf;

			wsprintfA( buf, "%08X", 0 );
			ListView_InsertItem( StackListWnd, &item );
		}
	}
	else
	{
		if(bLastPos)
		{
			if(last_StackStartAddr)
			{
				//显示位置
				int sfind = ListView_GetSelectionMark( StackListWnd );
				if(sfind != -1)
				{
					for(int i = 0; i <= STACK_SIZE; i++)
					{
						item.mask = LVIF_TEXT;
						item.iItem = 0;
						item.pszText = buf;

						if(i < sfind)	    wsprintfA( buf, "$ - %X", (sfind - i) * 4 );
						if(i == sfind)	wsprintfA( buf, "$ ==>" );
						if(i > sfind)	    wsprintfA( buf, "$ + %X", (i - sfind) * 4 );

						ListView_SetItemText( StackListWnd, i, 0, buf );
					}
				}
			}
		}
		else
		{
			if(last_StackStartAddr)
			{
				//还原成地址lastsfind
				for(int i = 0; i <= STACK_SIZE; i++)
				{
					item.mask = LVIF_TEXT;
					item.iItem = 0;
					item.pszText = buf;

					wsprintfA( buf, "%08X", last_StackStartAddr + i * 4 );
					ListView_SetItemText( StackListWnd, i, 0, buf );
				}
			}
		}
	}
}

//对话框
VOID InitMainDlgSomething()
{
	LV_COLUMN colmn = {0};
	LV_ITEM item = {0};
	char buf[100];

	//对hexedit初始化
	//HIMAGELIST himage = ImageList_Create(2,14,ILC_COLOR24|ILC_MASK,1,1);
	//ListView_SetImageList(HexEditWnd,himage,LVSIL_SMALL);				//改格子高度

	colmn.mask = LVCF_WIDTH | LVCF_TEXT;
	colmn.pszText = "ASCII";
	colmn.cx = 510;
	ListView_InsertColumn( HexEditWnd, 0, &colmn );

	for(int i = 0xF; i >= 0; i--)
	{
		wsprintfA( buf, "%02X", i );
		colmn.pszText = buf;
		colmn.cx = 32;
		ListView_InsertColumn( HexEditWnd, 0, &colmn );
	}

	colmn.pszText = "Address";
	colmn.cx = 70;
	ListView_InsertColumn( HexEditWnd, 0, &colmn );

	GotoHexEdit( (DWORD)GetModuleHandleA( NULL ) + 0x100 );

	//对FlagListWnd的初始化
	lstrcpyA( buf, "Ready" );
	ListView_SetExtendedListViewStyleEx( FlagListWnd, 0, LVS_EX_FULLROWSELECT );
	colmn.mask = LVCF_WIDTH | LVCF_TEXT;
	colmn.pszText = "Flag";
	colmn.cx = 100;
	ListView_InsertColumn( FlagListWnd, 0, &colmn );
	item.mask = LVIF_TEXT;
	item.pszText = buf;
	item.iItem = 0;
	ListView_InsertItem( FlagListWnd, &item );

	//对RegisterWnd的初始化
	ListView_SetExtendedListViewStyleEx( RegisterListWnd, 0, LVS_EX_FULLROWSELECT );
	colmn.mask = LVCF_WIDTH | LVCF_TEXT;
	colmn.pszText = "Register";
	colmn.cx = 165;
	ListView_InsertColumn( RegisterListWnd, 0, &colmn );
	colmn.pszText = "Name";
	colmn.cx = 70;
	ListView_InsertColumn( RegisterListWnd, 0, &colmn );
	HIMAGELIST himage = ImageList_Create( 2, 20, ILC_COLOR24 | ILC_MASK, 1, 1 );
	ListView_SetImageList( RegisterListWnd, himage, LVSIL_SMALL );				//改格子高度

	PCH	 RegisterName[] = {"EAX","ECX","EDX","EBX","ESP","EBP","ESI","EDI","","EIP"};
	item.mask = LVIF_TEXT;
	item.iItem = 0;
	for(int i = 9; i >= 0; i--)	//写入寄存器的Name
	{
		item.pszText = RegisterName[i];
		ListView_InsertItem( RegisterListWnd, &item );
	}
	CONTEXT tmpcontext = {0};
	bp::ShowRegister( &tmpcontext );		//寄存器开始地址

										//对STACK LIST初始化
	InitStackList();
}

/************************************************************************/
/*  主对话框处理
/************************************************************************/
int hexedit_clickitem = -1;
int hexedit_clicksubitem = -1;

BOOL CALLBACK DebugMainWndProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	char buf[200];

	switch(uMsg)
	{
		case WM_INITDIALOG:
			MainWnd = hDlg;
			ASMlistWnd = GetDlgItem( hDlg, IDC_LISTASM );
			RegisterListWnd = GetDlgItem( hDlg, IDC_LISTREGISTER );
			FlagListWnd = GetDlgItem( hDlg, IDC_LISTFLAG );
			StackListWnd = GetDlgItem( hDlg, IDC_LISTSTACK );
			HexEditWnd = GetDlgItem( hDlg, IDC_LISTHEXEDIT );

			//初始化
			InitMainDlgSomething();
			InitDebugSomething();
			_InitAllMod();

			//注册热键
			//RegisterHotKey(hDlg,1001,MOD_CONTROL,'G');	//跳转到

			//ASM开始地址
			wsprintfA( buf, "%X", os::ctGetProcAddress( GetModuleHandleA( "user32.dll" ), "MessageBoxA" ) );
			FsDisasm( buf );
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD( wParam ))
			{
				//主菜单
				case ID_40005:	//跳转到
					ShowJmpWnd( hDlg );
					break;
				case ID_40011:	//int3
					bp::SoftBreakPoint();
					break;
				case ID_40012:	//条件断点
					if(bp::DelBreakTj() == FALSE)	//如果返回true就是删除成功了
						ShowTJWnd( hDlg );
					break;
				case ID_40008:	//硬件访问
					AddHardPoint( ASMlistWnd, 3, 1 );
					break;
				case ID_40009:	//写入
					AddHardPoint( ASMlistWnd, 1, 1 );
					break;
				case ID_40010:	//执行
					AddHardPoint( ASMlistWnd, 0, 1 );
					break;

				case ID_40014:  //读取断点 （针对线程）
					ShowShdThreadWnd( hDlg );
					break;

					//菜单1
				case ID_MCHANGE:
					ShowChangeValueWnd( GetFocus() );
					break;
				case ID_MSHOWSTACK:
					bp::ShowStackForListWnd( GetFocus() );
					break;

					//button
				case IDC_BUTTON_C:
					FsDisasmLastAddr();
					break;
				case IDC_BUTTON_E:
					FsInitModule();
					break;
				case IDC_BUTTON_B:
					ShowAllBreakPoint();
					break;

					//run call
				case IDC_BUTTON_RUN:
					//ShowCallWnd(hDlg);
					break;

					//怪物猎人 遍历
				case IDC_BUTTON_MH1:
					//ShowMHWnd(hDlg);
					break;

					//上古世纪 功能
				case IDC_BUTTON_SG:
					//ShowSGWnd(hDlg);
					break;

					//dnf
					//自动更新dxf
				case IDC_BUTTON_DXF:
					//AutoUpdateForDXF();
					break;
					//功能TEST
				case IDC_BUTTON_DXF2:
					//ShowDNFWnd(hDlg);
					break;

			}
			return 1;


			//listbox 颜色
		case WM_CTLCOLORLISTBOX:
			return (LONG)CreateSolidBrush( RGB( 255, 251, 240 ) );


		case WM_NOTIFY:
			//先处理自绘
			if(((LPNMHDR)lParam)->code == NM_CUSTOMDRAW)
			{
				LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
				switch(lplvcd->nmcd.dwDrawStage)
				{
					case CDDS_PREPAINT:
						SetWindowLong( hDlg, DWL_MSGRESULT, (LONG)(CDRF_NOTIFYITEMDRAW) );
						return 1;
					case CDDS_ITEMPREPAINT:
						SetWindowLong( hDlg, DWL_MSGRESULT, (LONG)(CDRF_NOTIFYSUBITEMDRAW) );
						return 1;

						//关键！
					case (CDDS_ITEMPREPAINT | CDDS_SUBITEM):
						{
							switch(LOWORD( wParam ))
							{
								//lplvcd->nmcd.dwItemSpec;//行索引 ;
								//lplvcd->iSubItem;//列索引 ;
								case IDC_LISTFLAG:
									if(WaitRun == TRUE)
										lplvcd->clrTextBk = RGB( 227, 233, 22 );//黄色背景色
									else
										lplvcd->clrTextBk = RGB( 55, 179, 200 );//黄色背景色
									break;

								case IDC_LISTREGISTER:
									lplvcd->clrTextBk = RGB( 255, 251, 240 );		//背景颜色
									break;

								case IDC_LISTSTACK:
									if(lplvcd->nmcd.hdc)
									{
										HPEN hpen = CreatePen( PS_SOLID, 1, RGB( 160, 160, 160 ) );
										SelectObject( lplvcd->nmcd.hdc, hpen );

										RECT rt;
										ListView_GetSubItemRect( StackListWnd, 0, 1, LVIR_BOUNDS, &rt );	//获取第二个item 然后用left来画线
										MoveToEx( lplvcd->nmcd.hdc, rt.left - 1, 0, NULL );		//宽度-1
										LineTo( lplvcd->nmcd.hdc, rt.left - 1, 560 );

										DeleteObject( hpen );

										lplvcd->clrTextBk = RGB( 255, 251, 240 );		//背景颜色
									}
									break;

								case IDC_LISTHEXEDIT:
									if(lplvcd->nmcd.hdc)
									{
										HPEN hpen = CreatePen( PS_SOLID, 1, RGB( 160, 160, 160 ) );
										SelectObject( lplvcd->nmcd.hdc, hpen );
										RECT rt;
										ListView_GetSubItemRect( HexEditWnd, 0, 1, LVIR_BOUNDS, &rt );	//获取第二个item 然后用left来画线
										MoveToEx( lplvcd->nmcd.hdc, rt.left - 1, 0, NULL );		//宽度-1
										LineTo( lplvcd->nmcd.hdc, rt.left - 1, 560 );
										DeleteObject( hpen );

										//动态设置选取的
										int iline = lplvcd->nmcd.dwItemSpec;
										int isub = lplvcd->iSubItem;

										//debug("hexedit_clickitem = %d",hexedit_clickitem);
										if(iline == hexedit_clickitem && isub == hexedit_clicksubitem)
										{
											lplvcd->clrText = RGB( 0xFF, 0xFF, 0xFF );
											lplvcd->clrTextBk = RGB( 0x00, 0x00, 0x99 );
										}
										else
										{
											lplvcd->clrText = RGB( 0x00, 0x00, 0x00 );
											lplvcd->clrTextBk = RGB( 0xFF, 0xFF, 0xFF );

											lplvcd->clrTextBk = RGB( 255, 251, 240 );		//背景颜色
										}
									}
									break;

								case IDC_LISTASM:
									{
										switch(iListShowType)
										{
											case ListType_ASM:
												ChangeDisasmColor( lplvcd->nmcd.dwItemSpec, lplvcd->iSubItem, lplvcd );

												//这里只画的是listviewasm里的
												// The bottom of the header corresponds to the top of the line
												if(lplvcd  && lplvcd->nmcd.hdc)
												{
													HPEN hpen = CreatePen( PS_SOLID, 1, RGB( 160, 160, 160 ) );
													SelectObject( lplvcd->nmcd.hdc, hpen );

													RECT rt;
													ListView_GetSubItemRect( ASMlistWnd, 0, 1, LVIR_BOUNDS, &rt );	//获取第二个item 然后用left来画线
													MoveToEx( lplvcd->nmcd.hdc, rt.left - 1, 0, NULL );		//宽度-1
													LineTo( lplvcd->nmcd.hdc, rt.left - 1, 520 );

													ListView_GetSubItemRect( ASMlistWnd, 0, 2, LVIR_BOUNDS, &rt );	//获取第二个item 然后用left来画线
													MoveToEx( lplvcd->nmcd.hdc, rt.left - 1, 0, NULL );		//宽度-1
													LineTo( lplvcd->nmcd.hdc, rt.left - 1, 520 );

													DeleteObject( hpen );
												}

												break;
											case ListType_MOD:
												ChangeModuleColor( lplvcd->nmcd.dwItemSpec, lplvcd->iSubItem, lplvcd );
												break;
										}
									}
									break;
							}

							SetWindowLong( hDlg, DWL_MSGRESULT, (LONG)(CDRF_DODEFAULT) );
							return 1;
						}
				}

				SetWindowLong( hDlg, DWL_MSGRESULT, (LONG)(CDRF_DODEFAULT) );
				return 1;
			}
			//其他
			//////////////////////////////////////////////////////////////////////////
			if(LOWORD( wParam ) == IDC_LISTHEXEDIT)
			{
				switch(((LPNMHDR)lParam)->code)
				{
					case NM_CLICK:	//单击
						{
							LVHITTESTINFO plvhti = {0};
							GetCursorPos( &plvhti.pt );
							ScreenToClient( HexEditWnd, &plvhti.pt );
							ListView_SubItemHitTest( HexEditWnd, &plvhti );

							hexedit_clickitem = plvhti.iItem;
							hexedit_clicksubitem = plvhti.iSubItem;

							debug( "选中： %d::%d", plvhti.iItem, plvhti.iSubItem );

							//重绘一次表格
							RECT rt;
							GetClientRect( HexEditWnd, &rt );
							InvalidateRect( HexEditWnd, &rt, TRUE );
							break;
						}

					case LVN_KEYDOWN:		//必须用CreateDialog创建的非模式对话框才可以相应ENTER键
						{
							LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)lParam;
							switch(pLVKeyDow->wVKey)
							{
								case 'G':
									if(GetKeyState( VK_CONTROL ) < 0)
										ShowJmpWnd( hDlg );
									break;
							}

							break;
						}
				}

				return 1;
			}
			else if(LOWORD( wParam ) == IDC_LISTREGISTER || LOWORD( wParam ) == IDC_LISTSTACK)			//暂停/运行 显示栏
			{
				if(((LPNMHDR)lParam)->code == NM_DBLCLK && LOWORD( wParam ) == IDC_LISTSTACK)		//双击
				{
					//这个改为把堆栈可以调节成双击位置的序号 $+xx   POS <-> ADDR 转换
					InitStackList();
				}
				else if(((LPNMHDR)lParam)->code == NM_RCLICK)	//右击
				{
#if ANTI_OSAPI_DETECTION
					*(PDWORD)hinst = 0x5A4D;
#endif

					POINT pt;
					GetCursorPos( &pt );
					TrackPopupMenu( GetSubMenu( LoadMenuA( hinst, MAKEINTRESOURCE( IDR_RMENU1 ) ), 0 ), TPM_LEFTALIGN, pt.x, pt.y, 0, MainWnd, 0 );

#if ANTI_OSAPI_DETECTION
					*(PDWORD)hinst = 0;
#endif
				}

				return 1;
			}
			else if(LOWORD( wParam ) == IDC_LISTASM)		//反汇编的LISTVIEW
			{
				switch(((LPNMHDR)lParam)->code)
				{
					case NM_RCLICK:	//右击
						{
#if ANTI_OSAPI_DETECTION
							*(PDWORD)hinst = 0x5A4D;
#endif
							POINT pt;
							GetCursorPos( &pt );
							TrackPopupMenu( GetSubMenu( LoadMenuA( hinst, MAKEINTRESOURCE( IDR_MENU1 ) ), 0 ), TPM_LEFTALIGN, pt.x, pt.y, 0, MainWnd, 0 );
#if ANTI_OSAPI_DETECTION
							*(PDWORD)hinst = 0;
#endif
							break;
						}

					case LVN_KEYDOWN:		//必须用CreateDialog创建的非模式对话框才可以相应ENTER键
						{
							LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)lParam;
							switch(pLVKeyDow->wVKey)
							{
								case 'G':
									if(GetKeyState( VK_CONTROL ) < 0)
										ShowJmpWnd( hDlg );
									break;

								case VK_RETURN:				//跳转到
									ListJmpToChoiceAsmCode();
									break;
								case VK_SPACE:				//汇编此处
									ShowASMWnd( hDlg );
									break;
								case VK_F2:					//cc断点
									bp::SoftBreakPoint();
									break;
								case VK_F3:					//条件cc断点
									if(bp::DelBreakTj() == FALSE)	//如果返回true就是删除成功了
										ShowTJWnd( hDlg );
									break;
								case VK_F7:
									//设置当前线程TF位继续走
									if(WaitRun == TRUE)
										bp::SetStepUserRun();			//允许运行
									break;
								case VK_F9:					//断下后运行
									bp::SetUserRun();
									break;

								case VK_DELETE:				//删除断点
									if(iListShowType == ListType_BP)
										bp::DelHardPoint( ListView_GetSelectionMark( ASMlistWnd ) );
									break;
							}

							return TRUE;
						}

					case LVN_GETDISPINFO:	//动态读取
						{
							NMLVDISPINFO* plvdi = (NMLVDISPINFO*)lParam;
							if(plvdi->item.mask & LVIF_TEXT)
							{
								//行   ： plvdi->item.iItem
								//子项 ： plvdi->item.iSubItem
								//wsprintfA(buf,"%d::%d",plvdi->item.iItem,plvdi->item.iSubItem);
								//lstrcpyA(plvdi->item.pszText,buf);
								switch(iListShowType)
								{
									case ListType_ASM:
										ListView_GetDisasm( plvdi );
										break;
									case ListType_MOD:
										ListView_GetModule( plvdi );
										break;
									case ListType_BP:
										bp::ListView_GetBp( plvdi );
										break;
								}
							}

							return TRUE;
						}
				}
			}
			return 1;

		case WM_CLOSE:
			//UnregisterHotKey(hDlg,1001);
			//DestroyWindow
			ExitProcess( 0 );
			return 1;
	}

	return 0;
}


//
// 创建对话框
VOID FsDialogBox( HINSTANCE hinst, LPSTR lptmp, HWND parentWnd, DLGPROC lpWndProc )
{
#if ANTI_OSAPI_DETECTION
	//还原DLL头的2个字节  创建窗口 完了再清空
	*(PDWORD)hinst = 0x5A4D;
#endif

	ShowWindow( CreateDialogA( hinst, lptmp, parentWnd, lpWndProc ), 5 );

#if ANTI_OSAPI_DETECTION
	*(PDWORD)hinst = 0;
#endif

	MSG msg;
	while(GetMessage( &msg, NULL, 0, 0 ))
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
}