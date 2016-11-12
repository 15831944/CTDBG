//反汇编相关
#include "stdafx.h"
#include "myDiasm.h"
#include "breakpoint.h"

/************************************************************************/
/* HexEdit定位
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

	//拉到指定的位置
	DWORD addr_moveto = addr - (addr % 0x10);
	int imoved = 0;

	//从头部到尾部共0x1000
	DWORD addr_head = addr - (addr % 0x1000);
	int icount = 0;
	for(int i = 0; i <= 0xFF0; i += 0x10)
	{
		//先插入行
		item.iItem = icount;
		wsprintfA( buf, "%08X", addr_head + i );
		ListView_InsertItem( HexEditWnd, &item );

		if(addr_moveto == addr_head + i)
		{
			imoved = icount;
		}

		//设置0x10个数据
		for(int i2 = 0; i2 <= 0xF; i2++)
		{
			wsprintfA( buf, "%02X", GETPB( addr_head + i + i2 ) );
			ListView_SetItemText( HexEditWnd, icount, i2 + 1, buf );
		}

		//插入ascii码
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


	//拉到指定的位置
	if(imoved > 0XC)
		ListView_EnsureVisible( HexEditWnd, imoved + 0xC, TRUE );	//往下拉点 把当前的addr置顶
	ListView_SetItemState( HexEditWnd, imoved, LVIS_SELECTED, LVIS_SELECTED );
	ListView_SetSelectionMark( HexEditWnd, imoved );
}











/************************************************************************/
/* 汇编一个地址
/************************************************************************/
BOOL FsAsmWriteAddresss( PCH cmd, OUT PCH errtext )
{
	//获取当前选中的地址
	char bufaddr[50] = {0};
	if(os::ListView_GetSelectStr( ASMlistWnd, 0, bufaddr, 50 ) == 0)
		return FALSE;

	DWORD addr = strtol( bufaddr, 0, 16 );
	if(addr && IsBadReadPtr( (PVOID)addr, 1 ) == 0)
	{
		t_asmmodel tasd = {0};
		Assemble( cmd, addr, &tasd, 0, 0, errtext );
		debug( "Assemble [%08X]: %X %X %X (%d)   err:%s", addr, tasd.code[0], tasd.code[1], tasd.code[2], tasd.length, errtext );

		//修改汇编到那个地址
		os::ctVirtualProtect( addr, tasd.length );
		memcpy( (PVOID)addr, tasd.code, tasd.length );

		//关闭后刷新一下当前地址 
		FsDisasm( bufaddr );
		return TRUE;
	}

	return FALSE;
}


/************************************************************************/
/* 对反汇编数据的保存 使用单向链表
保存所有当前页面需要显示的数据
/************************************************************************/
typedef struct _FsDisasmSaveStruct
{
	DWORD addr;
	int   len;
	char  opcode[30];
	char  bytes[15];
	ulong jmpconst;		//跳向的地址  jmp xxx  call xxx
	ulong adrconst;		//push xxxx   push [xxxx] 含有立即数的
	ulong immconst;		//mov eax,xxxx  立即数
} FsDisasmSaveStruct, *PFsDisasmSaveStruct;

FsDisasmSaveStruct fsDS[0x1500];
DWORD s_LastAsmStart = 0;
DWORD s_LastAsmEnd = 0;

DWORD s_LastAsmAddr = 0;

//传入当前行来修改颜色
//行,列
VOID ChangeDisasmColor( int iline, int iSubItem, LPNMLVCUSTOMDRAW lplvcd )
{
	//断点
	if(bp::IsInCCRecord( fsDS[iline].addr ))
	{
		lplvcd->clrTextBk = RGB( 237, 28, 36 );//背景颜色
		return;
	}

	if(iSubItem == 2)
	{
		//其他代码
		if(strstr( fsDS[iline].opcode, "call" ))
		{
			lplvcd->clrText = RGB( 163, 51, 50 );//改变文字颜色 
												 //lplvcd->clrTextBk= RGB(0, 255, 255);//背景颜色
		}
		else if(strstr( fsDS[iline].opcode, "jmp" ) || strstr( fsDS[iline].opcode, "ret" ))
		{
			lplvcd->clrText = RGB( 151, 125, 23 );//改变文字颜色 
												  //lplvcd->clrTextBk= RGB(0, 255, 255);//背景颜色
		}
		else if(strstr( fsDS[iline].opcode, "je" ) || strstr( fsDS[iline].opcode, "jne" ) || strstr( fsDS[iline].opcode, "jz" )
				 || strstr( fsDS[iline].opcode, "jnz" ) || strstr( fsDS[iline].opcode, "jle" ) || strstr( fsDS[iline].opcode, "jg" ) || strstr( fsDS[iline].opcode, "ja" )
				 || strstr( fsDS[iline].opcode, "jl" ) || strstr( fsDS[iline].opcode, "jb" ))
		{
			lplvcd->clrText = RGB( 58, 120, 41 );//改变文字颜色 
												 //lplvcd->clrTextBk= RGB(0, 255, 255);//背景颜色
		}
		else if(strstr( fsDS[iline].opcode, "push" ) || strstr( fsDS[iline].opcode, "pop" ))
		{
			lplvcd->clrText = RGB( 30, 64, 181 );//改变文字颜色 
		}

		return;
	}

	lplvcd->clrTextBk = RGB( 255, 251, 240 );		//背景颜色
}

//将所有数据放入链表中
BOOL FsDisasm( PCH bufaddr )
{
	debug( "FsDisasm : %s", bufaddr );

	//上次的显示模式不是反汇编则重新初始化这个
	if(iListShowType != ListType_ASM)
	{
		//////////////////////////////////////////////////////////////////////////
		//把之前的都删掉  删除10次第一列 就是把列都删掉
		for(int i = 0; i<10; i++)
			ListView_DeleteColumn( ASMlistWnd, 0 );

		//把ListView初始化
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
	//开始反汇编……
	DWORD StartAddr = 0;
	//先判断是否函数名
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
		debug( "StartAddr 错误的地址 [%08X]", StartAddr );
		return FALSE;
	}

	//设置标题为当前地址所在模块
	PCH modname = GetAddressModuleName( StartAddr );
	char buf[200];
	wsprintfA( buf, "FsDebug  - %s", modname );
	SetWindowTextA( MainWnd, buf );
	debug( buf );

	//保存一下最后汇编的地址 
	s_LastAsmAddr = StartAddr;

	//从开始地址看往前能不能走 max-0x500
	int backlen = 0;
	for(int i = 0x100; i <= 0x500; i += 0x100)
	{
		if(IsBadReadPtr( (PVOID)(StartAddr - i), 0x100 ) == FALSE)
			backlen = i;
		else
			break;
	}

	//确定往前走后的地址
	DWORD ori_StartAddr = StartAddr;
	int   ori_countline = 0;		//在listview的物理第几行 最后再滚动过去
									//计算出新的开始地址
	StartAddr -= backlen;

	//先看要反汇编的地址是否在之前内存地址里
	if(StartAddr >= s_LastAsmStart && StartAddr <= s_LastAsmEnd)
	{
		//直接设置选中就可以了
		//...
	}

	//开始新的反汇编
	//清空一下内存
	memset( fsDS, 0, sizeof( FsDisasmSaveStruct ) * 0x1500 );
	//保存重新计算的开始地址
	s_LastAsmStart = StartAddr;

	//开始反汇编
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
		asmlen = Disasm( DisAsmAddrS1, 20, srcip, &_tasm, DISASM_CODE, 0 );		//调用反汇编引擎

		fsDS[i].addr = srcip;
		fsDS[i].len = asmlen;
		lstrcpyA( fsDS[i].bytes, _tasm.dump );
		lstrcpyA( fsDS[i].opcode, _strlwr( _tasm.result ) );
		fsDS[i].jmpconst = _tasm.jmpconst;
		fsDS[i].adrconst = _tasm.adrconst;
		fsDS[i].immconst = _tasm.immconst;

		//保存最后结束的地址
		s_LastAsmEnd = srcip;
	}


	//设置List显示模式
	iListShowType = ListType_ASM;
	debug( "FsDisasm END" );

	//////////////////////////////////////////////////////////////////////////
	//刷新一下
	ListView_SetItemCount( ASMlistWnd, 0x1500 );
	//拉到指定的位置
	ListView_EnsureVisible( ASMlistWnd, ori_countline + 18, TRUE );		//先往后移动一下 保持不让这行在最底部
	ListView_EnsureVisible( ASMlistWnd, ori_countline, TRUE );
	//设置选中到跳转到的地址	
	ListView_SetItemState( ASMlistWnd, ori_countline, LVIS_SELECTED, LVIS_SELECTED );
	ListView_SetSelectionMark( ASMlistWnd, ori_countline );
	//////////////////////////////////////////////////////////////////////////

	return TRUE;
}

//直接地址方式
BOOL FsDisasm( DWORD addr )
{
	char bufaddr[100];
	wsprintfA( bufaddr, "%08X", addr );
	return FsDisasm( bufaddr );
}


//直接使用最后一次的地址
BOOL FsDisasmLastAddr()
{
	char buf[50];
	wsprintfA( buf, "%08X", s_LastAsmAddr );
	FsDisasm( buf );

	return TRUE;
}

/************************************************************************/
/* 跳转到选中的命令行中的跳转中去
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

	//可以往里跳入的字节码
	//E8 E9  FF15    ( call  jmp   call [xxxxx] )
	if((bufbyte[0] == 'E'  && bufbyte[1] == '8') ||
		(bufbyte[0] == 'E'  && bufbyte[1] == '9'))
	{
		if(strstr( bufasm, "call" ))
			FsDisasm( bufasm + 5 );		//call 123456   +5 就到 "123456"
		else if(strstr( bufasm, "jmp" ))
			FsDisasm( bufasm + 4 );		//jmp 123456   +4 就到 "123456"
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
/* 在指定的虚拟ListView中显示当前地址的反汇编
/************************************************************************/
//这个listview调用
VOID ListView_GetDisasm( NMLVDISPINFO* plvdi )
{
	int list_iItem = plvdi->item.iItem;			//行
	int list_isub = plvdi->item.iSubItem;		//行的子项

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
					//这里是注释 写入mod名func名等   还需要增加一些其他的  比如 处理带有立即数<adrconst> []的代码  里面是数据还是str 显示出来
					if( fsDS[list_iItem].immconst )			//mov eax,xxxx
					{
					if( strstr(fsDS[list_iItem].opcode,"[") == 0 )
					{
					if( IsBadReadPtr((PVOID)fsDS[list_iItem].immconst,4) == FALSE )
					{
					//判断是否unicode
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
					//带有立即数的opcode
					//这里增加判断 带 "["  "]"的 带的话就去读地址
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
					//带有跳转立即数的opcode
					wsprintfA(plvdi->item.pszText,"%s",GetFunctionName(fsDS[list_iItem].jmpconst));
					}
					*/
					break;
				}
		}
	}
}