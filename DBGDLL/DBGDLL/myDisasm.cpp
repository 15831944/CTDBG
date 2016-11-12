//��������
#include "stdafx.h"
#include "myDiasm.h"
#include "breakpoint.h"

/************************************************************************/
/* HexEdit��λ
/************************************************************************/
VOID GotoHexEdit( DWORD addr )
{
	SendMessage( HexEditWnd, LVM_DELETEALLITEMS, 0, 0 );


	char buf[200];
	LV_COLUMN colmn = {0};
	LV_ITEM   item = {0};
	item.mask = LVIF_TEXT;
	item.iItem = 0;
	item.pszText = buf;

	//����ָ����λ��
	DWORD addr_moveto = addr - (addr % 0x10);
	int imoved = 0;

	//��ͷ����β����0x1000
	DWORD addr_head = addr - (addr % 0x1000);
	int icount = 0;
	for(int i = 0; i <= 0xFF0; i += 0x10)
	{
		//�Ȳ�����
		item.iItem = icount;
		wsprintfA( buf, "%08X", addr_head + i );
		ListView_InsertItem( HexEditWnd, &item );

		if(addr_moveto == addr_head + i)
		{
			imoved = icount;
		}

		//����0x10������
		for(int i2 = 0; i2 <= 0xF; i2++)
		{
			wsprintfA( buf, "%02X", GETPB( addr_head + i + i2 ) );
			ListView_SetItemText( HexEditWnd, icount, i2 + 1, buf );
		}

		//����ascii��
		char tmp[30] = {0};
		for(int i3 = 0; i3<0x10; i3++)
		{
			BYTE curb = GETPB( addr_head + i + i3 );
			if(curb == 0)
				curb = '.';
			tmp[i3] = curb;
		}
		ListView_SetItemText( HexEditWnd, icount, 0x11, tmp );

		icount++;
	}


	//����ָ����λ��
	if(imoved > 0XC)
		ListView_EnsureVisible( HexEditWnd, imoved + 0xC, TRUE );	//�������� �ѵ�ǰ��addr�ö�
	ListView_SetItemState( HexEditWnd, imoved, LVIS_SELECTED, LVIS_SELECTED );
	ListView_SetSelectionMark( HexEditWnd, imoved );
}











/************************************************************************/
/* ���һ����ַ
/************************************************************************/
BOOL FsAsmWriteAddresss( PCH cmd, OUT PCH errtext )
{
	//��ȡ��ǰѡ�еĵ�ַ
	char bufaddr[50] = {0};
	if(os::ListView_GetSelectStr( ASMlistWnd, 0, bufaddr, 50 ) == 0)
		return FALSE;

	DWORD addr = strtol( bufaddr, 0, 16 );
	if(addr && IsBadReadPtr( (PVOID)addr, 1 ) == 0)
	{
		t_asmmodel tasd = {0};
		Assemble( cmd, addr, &tasd, 0, 0, errtext );
		debug( "Assemble [%08X]: %X %X %X (%d)   err:%s", addr, tasd.code[0], tasd.code[1], tasd.code[2], tasd.length, errtext );

		//�޸Ļ�ൽ�Ǹ���ַ
		os::ctVirtualProtect( addr, tasd.length );
		memcpy( (PVOID)addr, tasd.code, tasd.length );

		//�رպ�ˢ��һ�µ�ǰ��ַ 
		FsDisasm( bufaddr );
		return TRUE;
	}

	return FALSE;
}


/************************************************************************/
/* �Է�������ݵı��� ʹ�õ�������
�������е�ǰҳ����Ҫ��ʾ������
/************************************************************************/
typedef struct _FsDisasmSaveStruct
{
	DWORD addr;
	int   len;
	char  opcode[30];
	char  bytes[15];
	ulong jmpconst;		//����ĵ�ַ  jmp xxx  call xxx
	ulong adrconst;		//push xxxx   push [xxxx] ������������
	ulong immconst;		//mov eax,xxxx  ������
} FsDisasmSaveStruct, *PFsDisasmSaveStruct;

FsDisasmSaveStruct fsDS[0x1500];
DWORD s_LastAsmStart = 0;
DWORD s_LastAsmEnd = 0;

DWORD s_LastAsmAddr = 0;

//���뵱ǰ�����޸���ɫ
//��,��
VOID ChangeDisasmColor( int iline, int iSubItem, LPNMLVCUSTOMDRAW lplvcd )
{
	//�ϵ�
	if(bp::IsInCCRecord( fsDS[iline].addr ))
	{
		lplvcd->clrTextBk = RGB( 237, 28, 36 );//������ɫ
		return;
	}

	if(iSubItem == 2)
	{
		//��������
		if(strstr( fsDS[iline].opcode, "call" ))
		{
			lplvcd->clrText = RGB( 163, 51, 50 );//�ı�������ɫ 
												 //lplvcd->clrTextBk= RGB(0, 255, 255);//������ɫ
		}
		else if(strstr( fsDS[iline].opcode, "jmp" ) || strstr( fsDS[iline].opcode, "ret" ))
		{
			lplvcd->clrText = RGB( 151, 125, 23 );//�ı�������ɫ 
												  //lplvcd->clrTextBk= RGB(0, 255, 255);//������ɫ
		}
		else if(strstr( fsDS[iline].opcode, "je" ) || strstr( fsDS[iline].opcode, "jne" ) || strstr( fsDS[iline].opcode, "jz" )
				 || strstr( fsDS[iline].opcode, "jnz" ) || strstr( fsDS[iline].opcode, "jle" ) || strstr( fsDS[iline].opcode, "jg" ) || strstr( fsDS[iline].opcode, "ja" )
				 || strstr( fsDS[iline].opcode, "jl" ) || strstr( fsDS[iline].opcode, "jb" ))
		{
			lplvcd->clrText = RGB( 58, 120, 41 );//�ı�������ɫ 
												 //lplvcd->clrTextBk= RGB(0, 255, 255);//������ɫ
		}
		else if(strstr( fsDS[iline].opcode, "push" ) || strstr( fsDS[iline].opcode, "pop" ))
		{
			lplvcd->clrText = RGB( 30, 64, 181 );//�ı�������ɫ 
		}

		return;
	}

	lplvcd->clrTextBk = RGB( 255, 251, 240 );		//������ɫ
}

//���������ݷ���������
BOOL FsDisasm( PCH bufaddr )
{
	debug( "FsDisasm : %s", bufaddr );

	//�ϴε���ʾģʽ���Ƿ���������³�ʼ�����
	if(iListShowType != ListType_ASM)
	{
		//////////////////////////////////////////////////////////////////////////
		//��֮ǰ�Ķ�ɾ��  ɾ��10�ε�һ�� ���ǰ��ж�ɾ��
		for(int i = 0; i<10; i++)
			ListView_DeleteColumn( ASMlistWnd, 0 );

		//��ListView��ʼ��
		ListView_SetExtendedListViewStyleEx( ASMlistWnd, 0, LVS_EX_FULLROWSELECT );
		LV_COLUMN colmn = {0};
		colmn.mask = LVCF_WIDTH | LVCF_TEXT;

		colmn.pszText = "Comment";
		colmn.cx = 320;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "Opcode";
		colmn.cx = 450;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "Bytes";
		colmn.cx = 200;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "Address";
		colmn.cx = 120;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );

		//////////////////////////////////////////////////////////////////////////
		HIMAGELIST himage = ImageList_Create( 2, 20, ILC_COLOR24 | ILC_MASK, 1, 1 );
		ListView_SetImageList( ASMlistWnd, himage, LVSIL_SMALL );
	}



	//////////////////////////////////////////////////////////////////////////
	//��ʼ����࡭��
	DWORD StartAddr = 0;
	//���ж��Ƿ�����
	if((bufaddr[0] >= 'A' && bufaddr[0] <= 'Z') ||
		(bufaddr[0] >= 'a' && bufaddr[0] <= 'z'))
	{
		StartAddr = FsGetProcaddressByName( bufaddr );
	}
	else
	{
		StartAddr = strtol( bufaddr, 0, 16 );
	}

	if(StartAddr == 0 || IsBadReadPtr( (PVOID)StartAddr, 1 ) == TRUE)
	{
		debug( "StartAddr ����ĵ�ַ [%08X]", StartAddr );
		return FALSE;
	}

	//���ñ���Ϊ��ǰ��ַ����ģ��
	PCH modname = GetAddressModuleName( StartAddr );
	char buf[200];
	wsprintfA( buf, "FsDebug  - %s", modname );
	SetWindowTextA( MainWnd, buf );
	debug( buf );

	//����һ�������ĵ�ַ 
	s_LastAsmAddr = StartAddr;

	//�ӿ�ʼ��ַ����ǰ�ܲ����� max-0x500
	int backlen = 0;
	for(int i = 0x100; i <= 0x500; i += 0x100)
	{
		if(IsBadReadPtr( (PVOID)(StartAddr - i), 0x100 ) == FALSE)
			backlen = i;
		else
			break;
	}

	//ȷ����ǰ�ߺ�ĵ�ַ
	DWORD ori_StartAddr = StartAddr;
	int   ori_countline = 0;		//��listview������ڼ��� ����ٹ�����ȥ
									//������µĿ�ʼ��ַ
	StartAddr -= backlen;

	//�ȿ�Ҫ�����ĵ�ַ�Ƿ���֮ǰ�ڴ��ַ��
	if(StartAddr >= s_LastAsmStart && StartAddr <= s_LastAsmEnd)
	{
		//ֱ������ѡ�оͿ�����
		//...
	}

	//��ʼ�µķ����
	//���һ���ڴ�
	memset( fsDS, 0, sizeof( FsDisasmSaveStruct ) * 0x1500 );
	//�������¼���Ŀ�ʼ��ַ
	s_LastAsmStart = StartAddr;

	//��ʼ�����
	PCHAR DisAsmAddrS1 = (PCH)StartAddr;
	ulong asmlen = 0;
	ulong srcip = StartAddr;
	for(int i = 0; i<0x1500; i++, srcip += asmlen, DisAsmAddrS1 += asmlen)
	{
		if(IsBadReadPtr( DisAsmAddrS1, 20 ) == TRUE)
			break;

		if(srcip == ori_StartAddr)
			ori_countline = i;

		t_disasm _tasm = {0};
		asmlen = Disasm( DisAsmAddrS1, 20, srcip, &_tasm, DISASM_CODE, 0 );		//���÷��������

		fsDS[i].addr = srcip;
		fsDS[i].len = asmlen;
		lstrcpyA( fsDS[i].bytes, _tasm.dump );
		lstrcpyA( fsDS[i].opcode, _strlwr( _tasm.result ) );
		fsDS[i].jmpconst = _tasm.jmpconst;
		fsDS[i].adrconst = _tasm.adrconst;
		fsDS[i].immconst = _tasm.immconst;

		//�����������ĵ�ַ
		s_LastAsmEnd = srcip;
	}


	//����List��ʾģʽ
	iListShowType = ListType_ASM;
	debug( "FsDisasm END" );

	//////////////////////////////////////////////////////////////////////////
	//ˢ��һ��
	ListView_SetItemCount( ASMlistWnd, 0x1500 );
	//����ָ����λ��
	ListView_EnsureVisible( ASMlistWnd, ori_countline + 18, TRUE );		//�������ƶ�һ�� ���ֲ�����������ײ�
	ListView_EnsureVisible( ASMlistWnd, ori_countline, TRUE );
	//����ѡ�е���ת���ĵ�ַ	
	ListView_SetItemState( ASMlistWnd, ori_countline, LVIS_SELECTED, LVIS_SELECTED );
	ListView_SetSelectionMark( ASMlistWnd, ori_countline );
	//////////////////////////////////////////////////////////////////////////

	return TRUE;
}

//ֱ�ӵ�ַ��ʽ
BOOL FsDisasm( DWORD addr )
{
	char bufaddr[100];
	wsprintfA( bufaddr, "%08X", addr );
	return FsDisasm( bufaddr );
}


//ֱ��ʹ�����һ�εĵ�ַ
BOOL FsDisasmLastAddr()
{
	char buf[50];
	wsprintfA( buf, "%08X", s_LastAsmAddr );
	FsDisasm( buf );

	return TRUE;
}

/************************************************************************/
/* ��ת��ѡ�е��������е���ת��ȥ
/************************************************************************/
VOID ListJmpToChoiceAsmCode()
{
	if(iListShowType != ListType_ASM)
		return;

	char bufasm[50] = {0};
	if(os::ListView_GetSelectStr( ASMlistWnd, 2, bufasm, 50 ) == 0)
		return;

	char bufbyte[50];
	if(os::ListView_GetSelectStr( ASMlistWnd, 1, bufbyte, 50 ) == 0)
		return;

	//��������������ֽ���
	//E8 E9  FF15    ( call  jmp   call [xxxxx] )
	if((bufbyte[0] == 'E'  && bufbyte[1] == '8') ||
		(bufbyte[0] == 'E'  && bufbyte[1] == '9'))
	{
		if(strstr( bufasm, "call" ))
			FsDisasm( bufasm + 5 );		//call 123456   +5 �͵� "123456"
		else if(strstr( bufasm, "jmp" ))
			FsDisasm( bufasm + 4 );		//jmp 123456   +4 �͵� "123456"
	}
	else if(bufbyte[0] == 'F'  && bufbyte[1] == 'F' && (bufbyte[2] == '1' || bufbyte[2] == '2') && bufbyte[3] == '5')
	{
		//FF15 56341200     call    dword ptr [123456]
		PCH startstraddr = strstr( bufasm, "[" );
		startstraddr += 1;
		PCH tmpaddr = strstr( startstraddr, "]" );
		if(tmpaddr)
		{
			tmpaddr[0] = 0;

			DWORD jmptoaddr = strtol( startstraddr, 0, 16 );
			if(IsBadReadPtr( (PVOID)jmptoaddr, 4 ) == FALSE)
			{
				wsprintfA( bufasm, "%X", GETPD( jmptoaddr ) );
				FsDisasm( bufasm );
			}
		}
	}
}
/************************************************************************/
/* ��ָ��������ListView����ʾ��ǰ��ַ�ķ����
/************************************************************************/
//���listview����
VOID ListView_GetDisasm( NMLVDISPINFO* plvdi )
{
	int list_iItem = plvdi->item.iItem;			//��
	int list_isub = plvdi->item.iSubItem;		//�е�����

	if(fsDS[list_iItem].addr)
	{
		switch(list_isub)
		{
			case 0:
				wsprintfA( plvdi->item.pszText, "%08X", fsDS[list_iItem].addr );
				break;
			case 1:
				wsprintfA( plvdi->item.pszText, "%s", fsDS[list_iItem].bytes );
				break;
			case 2:
				wsprintfA( plvdi->item.pszText, "%s", fsDS[list_iItem].opcode );
				break;

			case 3:
				{
					/*
					//������ע�� д��mod��func����   ����Ҫ����һЩ������  ���� �������������<adrconst> []�Ĵ���  ���������ݻ���str ��ʾ����
					if( fsDS[list_iItem].immconst )			//mov eax,xxxx
					{
					if( strstr(fsDS[list_iItem].opcode,"[") == 0 )
					{
					if( IsBadReadPtr((PVOID)fsDS[list_iItem].immconst,4) == FALSE )
					{
					//�ж��Ƿ�unicode
					if( IsUnicodeStr((PBYTE)fsDS[list_iItem].immconst) )
					{
					int wlen = wcslen((PWCH)fsDS[list_iItem].immconst);
					if( wlen )
					{
					char buf[200];
					WCharToMByte((LPWSTR)fsDS[list_iItem].immconst,buf,wlen*2);
					wsprintfA(plvdi->item.pszText,"UNICODE %s",buf);
					}
					}
					else
					{
					if( lstrlenA((PCH)fsDS[list_iItem].immconst) )
					wsprintfA(plvdi->item.pszText,"ASCII %s",(PSTR)fsDS[list_iItem].immconst);
					}
					}

					}
					}
					else if( fsDS[list_iItem].adrconst )	//push xxxx
					{
					//������������opcode
					//���������ж� �� "["  "]"�� ���Ļ���ȥ����ַ
					if( strstr(fsDS[list_iItem].opcode,"[") )
					{
					if( IsBadReadPtr((PVOID)fsDS[list_iItem].adrconst,4)==FALSE )
					{
					PCH funcName = GetFunctionName(  GETPD( fsDS[list_iItem].adrconst) );
					if( funcName )
					wsprintfA(plvdi->item.pszText,"%s",funcName);
					else
					wsprintfA(plvdi->item.pszText,"0x%x",GETPD( fsDS[list_iItem].adrconst));
					}
					}
					else
					{
					PCH funcName = GetFunctionName(  fsDS[list_iItem].adrconst );
					if(funcName)
					wsprintfA(plvdi->item.pszText,"%s",funcName);
					}
					}
					else if( fsDS[list_iItem].jmpconst )		//call xxx  jmp xxxx
					{
					//������ת��������opcode
					wsprintfA(plvdi->item.pszText,"%s",GetFunctionName(fsDS[list_iItem].jmpconst));
					}
					*/
					break;
				}
		}
	}
}