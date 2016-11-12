//
//���Ե����Ի�����ʾ����
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
//����debug���
//������Ի��������
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
/* Ӳ���ϵ����thread
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
			if(GetKeyState( VK_RETURN ) < 0)  //ENTER��������
			{
				GetDlgItemTextA( hDlg, IDC_EDIT2, buf, 10 );
				DWORD tid = strtol( buf, 0, 0 );

				if(tid)
				{
					AddHardPoint( ASMlistWnd, 0, 1, tid );

					wsprintfA( buf, "�߳�[%d]����Ӳ���ϵ����", tid );
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
/* �����ϵ�Ի���
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

			//��ǰѡ���еĵ�ַ
			if(os::ListView_GetSelectStr( ASMlistWnd, 0, buf, 50 ))
				SetDlgItemTextA( hDlg, IDC_EDIT4, buf );

			SendDlgItemMessageA( hDlg, IDC_CHECK1, BM_SETCHECK, BST_CHECKED, 0 );
			SetTimer( hDlg, 0x888, 50, NULL );
			return 1;

		case WM_TIMER:
			if(bTJ_TIMER_USE == FALSE)
			{
				if(GetKeyState( VK_ESCAPE ) < 0)  //ESC��������
				{
					IsRunAWnd = 0;
					DestroyWindow( hDlg );
				}
				if(GetKeyState( VK_RETURN ) < 0)  //ENTER��������
				{
					bTJ_TIMER_USE = TRUE;

					if(bp::SetBreakTj( hDlg ) == TRUE)
					{
						MessageBoxW( hDlg, L"���������ϵ���ɣ�", L"", MB_ICONINFORMATION );
						IsRunAWnd = 0;
						DestroyWindow( hDlg );
						return 1;
					}
					else
					{
						MessageBoxA( hDlg, "����Ĳ���", "", MB_ICONSTOP );
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
/* �޸ĵ�ǰѡ�е�ַ��ֵ �Ĵ���/��ջ
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
			wsprintfA( buf, "�޸ĵ�ַ �� %X", ChangeAddress );
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

					//�޸���ˢ��һ��stacklist
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
	//�Ȼ�ȡҪ�޸ĵĵ�ַ
	//����޸ĵ��ǼĴ����͵��ڶ��µ�ʱ���޸ģ�
	char bufaddr[50] = {0};
	DWORD addr = 0;

	if(hlist == RegisterListWnd)
	{
		//ȥ�޸ļĴ���  ֻ����ͣ״̬�½���
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
/* ��൱ǰ��ַ�Ի���
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
/* ��ת���Ի���
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
					SetWindowTextA( hDlg, "��ת�������" );
					break;
				case JMP_STACKTYPE:
					SetWindowTextA( hDlg, "��ת����ջ��" );
					break;
				case JMP_MEMORYTYPE:
					SetWindowTextA( hDlg, "��ת���ڴ���" );
					break;
			}

			//����һ��timer����ȡenter��Ϣ
			SetTimer( hDlg, 888, 50, NULL );
			return 1;

		case WM_TIMER:
			if(GetKeyState( VK_ESCAPE ) < 0)  //ESC��������
			{
				IsRunAWnd = 0;
				DestroyWindow( hDlg );
			}
			if(GetKeyState( VK_RETURN ) < 0)  //ENTER��������
			{
				buf[0] = 0;
				GetDlgItemTextA( hDlg, IDC_EDIT2, buf, 100 );

				if(buf[0])
				{
					//��ʾ
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

					//�ر�
					if(bclose)
					{
						IsRunAWnd = 0;
						DestroyWindow( hDlg );
					}
				}

				SetDlgItemTextA( hDlg, IDC_JMPLOG, "��ת����ַ����" );
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
/* ��ʾ�ϵ��list����
/************************************************************************/
VOID ShowAllBreakPoint()
{
	//�ϴε���ʾģʽ���Ƿ���������³�ʼ�����
	if(iListShowType != ListType_BP)
	{
		//��֮ǰ�Ķ�ɾ��  ɾ��10�ε�һ�� ���ǰ��ж�ɾ��
		for(int i = 0; i < 10; i++)
			ListView_DeleteColumn( ASMlistWnd, 0 );

		//��ListView��ʼ��
		ListView_SetExtendedListViewStyleEx( ASMlistWnd, 0, LVS_EX_FULLROWSELECT );
		LV_COLUMN colmn = {0};
		colmn.mask = LVCF_WIDTH | LVCF_TEXT;

		colmn.pszText = "����";
		colmn.cx = 200;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "����";
		colmn.cx = 300;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "��ַ (DEL ɾ���ϵ�)";
		colmn.cx = 200;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
	}

	//�趨��ǰ����ʾģʽ
	iListShowType = ListType_BP;
	//ˢ��һ��
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
/* ��س�ʼ��
/************************************************************************/
//Debug
VOID InitDebugSomething()
{
	//�ӹ��쳣����
	bp::HookKiUserExceptionDispatcher();
}

//s1==TRUEʱ��һ���� $+xx ������pos �����ǵ�ַ
BOOL bLastPos = TRUE;
BOOL bInitStackList = FALSE;

VOID InitStackList()
{
	//�Ƿ���ʾλ�� Ĭ�ϵ�һ���ǲ���ʾ
	bLastPos = !bLastPos;

	LV_COLUMN colmn = {0};
	LV_ITEM item = {0};
	char buf[200];

	if(bInitStackList == FALSE)
	{
		bInitStackList = TRUE;

		//��StackListWnd�ĳ�ʼ��
		HIMAGELIST himage = ImageList_Create( 0, 18, ILC_COLOR24 | ILC_MASK, 1, 1 );
		ListView_SetImageList( StackListWnd, himage, LVSIL_SMALL );				//�ĸ��Ӹ߶�

		ListView_SetExtendedListViewStyleEx( StackListWnd, 0, LVS_EX_FULLROWSELECT );
		colmn.mask = LVCF_WIDTH | LVCF_TEXT;
		colmn.pszText = "Value";
		colmn.cx = 140;
		ListView_InsertColumn( StackListWnd, 0, &colmn );
		colmn.pszText = "Addr";
		colmn.cx = 80;
		ListView_InsertColumn( StackListWnd, 0, &colmn );

		//ȫ��ʼ����0
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
				//��ʾλ��
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
				//��ԭ�ɵ�ַlastsfind
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

//�Ի���
VOID InitMainDlgSomething()
{
	LV_COLUMN colmn = {0};
	LV_ITEM item = {0};
	char buf[100];

	//��hexedit��ʼ��
	//HIMAGELIST himage = ImageList_Create(2,14,ILC_COLOR24|ILC_MASK,1,1);
	//ListView_SetImageList(HexEditWnd,himage,LVSIL_SMALL);				//�ĸ��Ӹ߶�

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

	//��FlagListWnd�ĳ�ʼ��
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

	//��RegisterWnd�ĳ�ʼ��
	ListView_SetExtendedListViewStyleEx( RegisterListWnd, 0, LVS_EX_FULLROWSELECT );
	colmn.mask = LVCF_WIDTH | LVCF_TEXT;
	colmn.pszText = "Register";
	colmn.cx = 165;
	ListView_InsertColumn( RegisterListWnd, 0, &colmn );
	colmn.pszText = "Name";
	colmn.cx = 70;
	ListView_InsertColumn( RegisterListWnd, 0, &colmn );
	HIMAGELIST himage = ImageList_Create( 2, 20, ILC_COLOR24 | ILC_MASK, 1, 1 );
	ListView_SetImageList( RegisterListWnd, himage, LVSIL_SMALL );				//�ĸ��Ӹ߶�

	PCH	 RegisterName[] = {"EAX","ECX","EDX","EBX","ESP","EBP","ESI","EDI","","EIP"};
	item.mask = LVIF_TEXT;
	item.iItem = 0;
	for(int i = 9; i >= 0; i--)	//д��Ĵ�����Name
	{
		item.pszText = RegisterName[i];
		ListView_InsertItem( RegisterListWnd, &item );
	}
	CONTEXT tmpcontext = {0};
	bp::ShowRegister( &tmpcontext );		//�Ĵ�����ʼ��ַ

										//��STACK LIST��ʼ��
	InitStackList();
}

/************************************************************************/
/*  ���Ի�����
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

			//��ʼ��
			InitMainDlgSomething();
			InitDebugSomething();
			_InitAllMod();

			//ע���ȼ�
			//RegisterHotKey(hDlg,1001,MOD_CONTROL,'G');	//��ת��

			//ASM��ʼ��ַ
			wsprintfA( buf, "%X", os::ctGetProcAddress( GetModuleHandleA( "user32.dll" ), "MessageBoxA" ) );
			FsDisasm( buf );
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD( wParam ))
			{
				//���˵�
				case ID_40005:	//��ת��
					ShowJmpWnd( hDlg );
					break;
				case ID_40011:	//int3
					bp::SoftBreakPoint();
					break;
				case ID_40012:	//�����ϵ�
					if(bp::DelBreakTj() == FALSE)	//�������true����ɾ���ɹ���
						ShowTJWnd( hDlg );
					break;
				case ID_40008:	//Ӳ������
					AddHardPoint( ASMlistWnd, 3, 1 );
					break;
				case ID_40009:	//д��
					AddHardPoint( ASMlistWnd, 1, 1 );
					break;
				case ID_40010:	//ִ��
					AddHardPoint( ASMlistWnd, 0, 1 );
					break;

				case ID_40014:  //��ȡ�ϵ� ������̣߳�
					ShowShdThreadWnd( hDlg );
					break;

					//�˵�1
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

					//�������� ����
				case IDC_BUTTON_MH1:
					//ShowMHWnd(hDlg);
					break;

					//�Ϲ����� ����
				case IDC_BUTTON_SG:
					//ShowSGWnd(hDlg);
					break;

					//dnf
					//�Զ�����dxf
				case IDC_BUTTON_DXF:
					//AutoUpdateForDXF();
					break;
					//����TEST
				case IDC_BUTTON_DXF2:
					//ShowDNFWnd(hDlg);
					break;

			}
			return 1;


			//listbox ��ɫ
		case WM_CTLCOLORLISTBOX:
			return (LONG)CreateSolidBrush( RGB( 255, 251, 240 ) );


		case WM_NOTIFY:
			//�ȴ����Ի�
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

						//�ؼ���
					case (CDDS_ITEMPREPAINT | CDDS_SUBITEM):
						{
							switch(LOWORD( wParam ))
							{
								//lplvcd->nmcd.dwItemSpec;//������ ;
								//lplvcd->iSubItem;//������ ;
								case IDC_LISTFLAG:
									if(WaitRun == TRUE)
										lplvcd->clrTextBk = RGB( 227, 233, 22 );//��ɫ����ɫ
									else
										lplvcd->clrTextBk = RGB( 55, 179, 200 );//��ɫ����ɫ
									break;

								case IDC_LISTREGISTER:
									lplvcd->clrTextBk = RGB( 255, 251, 240 );		//������ɫ
									break;

								case IDC_LISTSTACK:
									if(lplvcd->nmcd.hdc)
									{
										HPEN hpen = CreatePen( PS_SOLID, 1, RGB( 160, 160, 160 ) );
										SelectObject( lplvcd->nmcd.hdc, hpen );

										RECT rt;
										ListView_GetSubItemRect( StackListWnd, 0, 1, LVIR_BOUNDS, &rt );	//��ȡ�ڶ���item Ȼ����left������
										MoveToEx( lplvcd->nmcd.hdc, rt.left - 1, 0, NULL );		//���-1
										LineTo( lplvcd->nmcd.hdc, rt.left - 1, 560 );

										DeleteObject( hpen );

										lplvcd->clrTextBk = RGB( 255, 251, 240 );		//������ɫ
									}
									break;

								case IDC_LISTHEXEDIT:
									if(lplvcd->nmcd.hdc)
									{
										HPEN hpen = CreatePen( PS_SOLID, 1, RGB( 160, 160, 160 ) );
										SelectObject( lplvcd->nmcd.hdc, hpen );
										RECT rt;
										ListView_GetSubItemRect( HexEditWnd, 0, 1, LVIR_BOUNDS, &rt );	//��ȡ�ڶ���item Ȼ����left������
										MoveToEx( lplvcd->nmcd.hdc, rt.left - 1, 0, NULL );		//���-1
										LineTo( lplvcd->nmcd.hdc, rt.left - 1, 560 );
										DeleteObject( hpen );

										//��̬����ѡȡ��
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

											lplvcd->clrTextBk = RGB( 255, 251, 240 );		//������ɫ
										}
									}
									break;

								case IDC_LISTASM:
									{
										switch(iListShowType)
										{
											case ListType_ASM:
												ChangeDisasmColor( lplvcd->nmcd.dwItemSpec, lplvcd->iSubItem, lplvcd );

												//����ֻ������listviewasm���
												// The bottom of the header corresponds to the top of the line
												if(lplvcd  && lplvcd->nmcd.hdc)
												{
													HPEN hpen = CreatePen( PS_SOLID, 1, RGB( 160, 160, 160 ) );
													SelectObject( lplvcd->nmcd.hdc, hpen );

													RECT rt;
													ListView_GetSubItemRect( ASMlistWnd, 0, 1, LVIR_BOUNDS, &rt );	//��ȡ�ڶ���item Ȼ����left������
													MoveToEx( lplvcd->nmcd.hdc, rt.left - 1, 0, NULL );		//���-1
													LineTo( lplvcd->nmcd.hdc, rt.left - 1, 520 );

													ListView_GetSubItemRect( ASMlistWnd, 0, 2, LVIR_BOUNDS, &rt );	//��ȡ�ڶ���item Ȼ����left������
													MoveToEx( lplvcd->nmcd.hdc, rt.left - 1, 0, NULL );		//���-1
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
			//����
			//////////////////////////////////////////////////////////////////////////
			if(LOWORD( wParam ) == IDC_LISTHEXEDIT)
			{
				switch(((LPNMHDR)lParam)->code)
				{
					case NM_CLICK:	//����
						{
							LVHITTESTINFO plvhti = {0};
							GetCursorPos( &plvhti.pt );
							ScreenToClient( HexEditWnd, &plvhti.pt );
							ListView_SubItemHitTest( HexEditWnd, &plvhti );

							hexedit_clickitem = plvhti.iItem;
							hexedit_clicksubitem = plvhti.iSubItem;

							debug( "ѡ�У� %d::%d", plvhti.iItem, plvhti.iSubItem );

							//�ػ�һ�α��
							RECT rt;
							GetClientRect( HexEditWnd, &rt );
							InvalidateRect( HexEditWnd, &rt, TRUE );
							break;
						}

					case LVN_KEYDOWN:		//������CreateDialog�����ķ�ģʽ�Ի���ſ�����ӦENTER��
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
			else if(LOWORD( wParam ) == IDC_LISTREGISTER || LOWORD( wParam ) == IDC_LISTSTACK)			//��ͣ/���� ��ʾ��
			{
				if(((LPNMHDR)lParam)->code == NM_DBLCLK && LOWORD( wParam ) == IDC_LISTSTACK)		//˫��
				{
					//�����Ϊ�Ѷ�ջ���Ե��ڳ�˫��λ�õ���� $+xx   POS <-> ADDR ת��
					InitStackList();
				}
				else if(((LPNMHDR)lParam)->code == NM_RCLICK)	//�һ�
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
			else if(LOWORD( wParam ) == IDC_LISTASM)		//������LISTVIEW
			{
				switch(((LPNMHDR)lParam)->code)
				{
					case NM_RCLICK:	//�һ�
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

					case LVN_KEYDOWN:		//������CreateDialog�����ķ�ģʽ�Ի���ſ�����ӦENTER��
						{
							LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)lParam;
							switch(pLVKeyDow->wVKey)
							{
								case 'G':
									if(GetKeyState( VK_CONTROL ) < 0)
										ShowJmpWnd( hDlg );
									break;

								case VK_RETURN:				//��ת��
									ListJmpToChoiceAsmCode();
									break;
								case VK_SPACE:				//���˴�
									ShowASMWnd( hDlg );
									break;
								case VK_F2:					//cc�ϵ�
									bp::SoftBreakPoint();
									break;
								case VK_F3:					//����cc�ϵ�
									if(bp::DelBreakTj() == FALSE)	//�������true����ɾ���ɹ���
										ShowTJWnd( hDlg );
									break;
								case VK_F7:
									//���õ�ǰ�߳�TFλ������
									if(WaitRun == TRUE)
										bp::SetStepUserRun();			//��������
									break;
								case VK_F9:					//���º�����
									bp::SetUserRun();
									break;

								case VK_DELETE:				//ɾ���ϵ�
									if(iListShowType == ListType_BP)
										bp::DelHardPoint( ListView_GetSelectionMark( ASMlistWnd ) );
									break;
							}

							return TRUE;
						}

					case LVN_GETDISPINFO:	//��̬��ȡ
						{
							NMLVDISPINFO* plvdi = (NMLVDISPINFO*)lParam;
							if(plvdi->item.mask & LVIF_TEXT)
							{
								//��   �� plvdi->item.iItem
								//���� �� plvdi->item.iSubItem
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
// �����Ի���
VOID FsDialogBox( HINSTANCE hinst, LPSTR lptmp, HWND parentWnd, DLGPROC lpWndProc )
{
#if ANTI_OSAPI_DETECTION
	//��ԭDLLͷ��2���ֽ�  �������� ���������
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