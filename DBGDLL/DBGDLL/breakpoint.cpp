#include "stdafx.h"
#include "thread.h"
#include "breakpoint.h"
#include "myDiasm.h"

DWORD last_StackStartAddr = 0;
BOOL WaitRun = FALSE;

/************************************************************************/
/* BP                                                                  
/************************************************************************/
namespace bp
{
	BOOL bwillTF = FALSE;
	DR_USE m_Dr_Use = {0};		//当前可用的4个硬件断点

	//
	// 硬件断点
	//返回当前可用的调试寄存器
	int FindFreeDebugRegister( DWORD setbpaddr )
	{
		//已有的时候
		if( setbpaddr == m_Dr_Use.Dr1_addr ||
			setbpaddr == m_Dr_Use.Dr2_addr ||
			setbpaddr == m_Dr_Use.Dr3_addr ||
			setbpaddr == m_Dr_Use.Dr0_addr)
		{
			return -2;
		}

		if(!m_Dr_Use.Dr0)
			return 0;
		if(!m_Dr_Use.Dr1)
			return 1;
		if(!m_Dr_Use.Dr2)
			return 2;
		if(!m_Dr_Use.Dr3)
			return 3;

		//如果Dr0-Dr3都被使用则返回-1
		return -1;
	}

	//设置硬件断点 参数 地址 属性 长度
	//dwAttribute 0表示执行断点 3表示访问断点 1 表示写入断点
	//dwLength 取值 1 2 4
	//nIndex   第几个dr寄存器
	VOID SetDr7( PCONTEXT pCt, DWORD dwBpAddress, DWORD dwAttribute, DWORD dwLength, int nIndex )
	{
		//赋值定义的DR7结构体,这样省去了位移操作的繁琐
		DR7 tagDr7 = {0};
		tagDr7.dwDr7 = pCt->Dr7;

		switch(nIndex)
		{
			case 0:
				//中断地址
				pCt->Dr0 = dwBpAddress;
				//断点长度
				tagDr7.DRFlag.len0 = dwLength - 1;
				//属性
				tagDr7.DRFlag.rw0 = dwAttribute;
				//局部断点
				tagDr7.DRFlag.L0 = 1;
				//设置标志位记录调试寄存器的使用情况
				m_Dr_Use.Dr0 = TRUE;
				m_Dr_Use.Dr0_addr = dwBpAddress;
				m_Dr_Use.Dr0_Attribute = dwAttribute;
				m_Dr_Use.Dr0_Len = dwLength;
				break;
			case 1:
				pCt->Dr1 = dwBpAddress;
				tagDr7.DRFlag.len1 = dwLength - 1;
				tagDr7.DRFlag.rw1 = dwAttribute;
				tagDr7.DRFlag.L1 = 1;
				m_Dr_Use.Dr1 = TRUE;
				m_Dr_Use.Dr1_addr = dwBpAddress;
				m_Dr_Use.Dr1_Attribute = dwAttribute;
				m_Dr_Use.Dr1_Len = dwLength;
				break;
			case 2:
				pCt->Dr2 = dwBpAddress;
				tagDr7.DRFlag.len2 = dwLength - 1;
				tagDr7.DRFlag.rw2 = dwAttribute;
				tagDr7.DRFlag.L2 = 1;
				m_Dr_Use.Dr2 = TRUE;
				m_Dr_Use.Dr2_addr = dwBpAddress;
				m_Dr_Use.Dr2_Attribute = dwAttribute;
				m_Dr_Use.Dr2_Len = dwLength;
				break;
			case 3:
				pCt->Dr3 = dwBpAddress;
				tagDr7.DRFlag.len3 = dwLength - 1;
				tagDr7.DRFlag.rw3 = dwAttribute;
				tagDr7.DRFlag.L3 = 1;
				m_Dr_Use.Dr3 = TRUE;
				m_Dr_Use.Dr3_addr = dwBpAddress;
				m_Dr_Use.Dr3_Attribute = dwAttribute;
				m_Dr_Use.Dr3_Len = dwLength;
				break;
		}

		//赋值回去
		pCt->Dr7 = tagDr7.dwDr7;
	}

	//添加硬断
	//dwAttribute 0表示执行断点 3表示访问断点 1 表示写入断点
	//dwLength 取值 1 2 4
	void SetHardBP( DWORD dwBpAddress, DWORD dwAttribute, DWORD dwLength, DWORD onlytid )
	{
		//强制把执行断点的长度改为1
		if(dwAttribute == 0)
			dwLength = 1;

		if(dwLength != 1 && dwLength != 2 && dwLength != 4)
		{
			os::MsgBox( "断点长度设置错误" );
			return;
		}

		//获得当前空闲调寄存器编号
		int nIndex = FindFreeDebugRegister( dwBpAddress );
		if(nIndex == -1)
		{
			os::MsgBox( "当前硬件断点已满,请删除在设置", 0, 0 );
			return;
		}
		if(nIndex == -2)
		{
			os::MsgBox( "当前地址[%X]已有硬件断点,请删除在设置", dwBpAddress );
			return;
		}

		td::SetAllThreadsDr( DebugMainThread, dwBpAddress, dwAttribute, dwLength, onlytid );
		debug( "硬件断点[%X]添加成功", dwBpAddress );
	}

	//
	//删除硬件断点  s1=第几个 0-3
	VOID DelHardPoint( int idr )
	{
		if(idr < 4)
		{
			debug( "删除硬件断点： (%d)", idr );

			switch(idr)
			{
				case 0:
					m_Dr_Use.Dr0 = 0;
					m_Dr_Use.Dr0_addr = 0;
					td::DelAllThreadsDrs( DebugMainThread, idr, WaitRun );		//WaitRun==TRUE的时候不执行暂停/启动线程的动作
					break;
				case 1:
					m_Dr_Use.Dr1 = 0;
					m_Dr_Use.Dr1_addr = 0;
					td::DelAllThreadsDrs( DebugMainThread, idr, WaitRun );
					break;
				case 2:
					m_Dr_Use.Dr2 = 0;
					m_Dr_Use.Dr2_addr = 0;
					td::DelAllThreadsDrs( DebugMainThread, idr, WaitRun );
					break;
				case 3:
					m_Dr_Use.Dr3 = 0;
					m_Dr_Use.Dr3_addr = 0;
					td::DelAllThreadsDrs( DebugMainThread, idr, WaitRun );
					break;
			}

			//刷新一下
			ListView_SetItemCount( ASMlistWnd, 4 );
		}
	}

	//
	// 在指定的虚拟ListView中获取断点
	VOID ListView_GetBp( NMLVDISPINFO* plvdi )
	{
		int list_iItem = plvdi->item.iItem;			//行
		int list_isub = plvdi->item.iSubItem;		//行的子项

		BOOL HardPointFlags[4] = {m_Dr_Use.Dr0,m_Dr_Use.Dr1,m_Dr_Use.Dr2,m_Dr_Use.Dr3};

		DWORD allatt[] = {m_Dr_Use.Dr0_addr,m_Dr_Use.Dr0_Attribute,m_Dr_Use.Dr0_Len,
			m_Dr_Use.Dr1_addr,m_Dr_Use.Dr1_Attribute,m_Dr_Use.Dr1_Len,
			m_Dr_Use.Dr2_addr,m_Dr_Use.Dr2_Attribute,m_Dr_Use.Dr2_Len,
			m_Dr_Use.Dr3_addr,m_Dr_Use.Dr3_Attribute,m_Dr_Use.Dr3_Len};

		if(list_iItem < 4)
		{
			DWORD addr = 0, att = -1, len = -1;
			if(HardPointFlags[list_iItem] == TRUE)
			{
				//已存在
				addr = allatt[list_iItem * 3 + 0];
				att = allatt[list_iItem * 3 + 1];
				len = allatt[list_iItem * 3 + 2];
			}

			switch(list_isub)
			{
				case 0:		//地址
					wsprintfA( plvdi->item.pszText, "%08X", addr );
					break;
				case 1:		//属性
							//dwAttribute 0表示执行断点 3表示访问断点 1 表示写入断点
					switch(att)
					{
						case 0:
							wsprintfA( plvdi->item.pszText, "执行断点" );
							break;
						case 1:
							wsprintfA( plvdi->item.pszText, "写入断点" );
							break;
						case 3:
							wsprintfA( plvdi->item.pszText, "访问断点" );
							break;
						default:
							wsprintfA( plvdi->item.pszText, "未使用" );
							break;
					}
					break;
				case 2:		//长度
					wsprintfA( plvdi->item.pszText, "%d", len );
					break;
			}
		}
	}

	//
	// 设置软件断点 0xCC
	// 软件断点支持100个
	//
	struct ctSoftBreakPoint
	{
		DWORD addr;
		BYTE  ori_byte;
	};
	ctSoftBreakPoint cc_breakpoint_Array[100] = {0};

	//断点是否在数据库中
	BOOL IsInCCRecord( DWORD addr )
	{
		//先看有没有相同的
		for(int i = 0; i < 100; i++)
		{
			if(cc_breakpoint_Array[i].addr == addr)
				return TRUE;
		}

		return FALSE;
	}

	//
	//添加断点
	BOOL AddCCToRecord( DWORD addr, DWORD nextaddr )
	{
		if(IsInCCRecord( addr ))
			return TRUE;

		//没有的话找个空白的地方直接写入断点
		for(int i = 0; i < 100; i++)
		{
			if(cc_breakpoint_Array[i].addr == 0)
			{
				cc_breakpoint_Array[i].addr = addr;
				cc_breakpoint_Array[i].ori_byte = GETPB( addr );
				return TRUE;
			}
		}

		os::MsgBox( "断点数量已满！" );
		return FALSE;
	}

	//
	//删除断点
	BOOL DelCC( DWORD addr )
	{
		for(int i = 0; i < 100; i++)
		{
			if(cc_breakpoint_Array[i].addr == addr)
			{
				os::ctVirtualProtect( addr, 1 );
				GETPB( addr ) = cc_breakpoint_Array[i].ori_byte;

				//清空
				cc_breakpoint_Array[i].addr = 0;
				return TRUE;
			}
		}

		return FALSE;
	}

	//
	//添加/删除断点
	BOOL SoftBreakPoint()
	{
		char bufaddr[20] = {0};
		if(os::ListView_GetSelectStr( ASMlistWnd, 0, bufaddr, 20 ) == 0)
			return FALSE;
		char bufnextaddr[20] = {0};
		if(os::ListView_GetSelectNextStr( ASMlistWnd, 0, bufnextaddr, 20 ) == 0)
			return FALSE;

		DWORD addr = strtol( bufaddr, 0, 16 );
		DWORD nextaddr = strtol( bufnextaddr, 0, 16 );

		if(addr && IsBadReadPtr( (PVOID)addr, 1 ) == FALSE)
		{
			if(IsInCCRecord( addr ))
			{
				DelCC( addr );
				debug( "Del CC : %X", addr );
			}
			else
			{
				if(AddCCToRecord( addr, nextaddr ))
				{
					os::ctVirtualProtect( addr, 1 );
					GETPB( addr ) = 0xCC;

					debug( "Add CC : %X", addr );
				}
			}


			FsDisasm( addr );
		}

		return TRUE;
	}

	//
	// 等待操作F9
	VOID WaitUserRun( PCONTEXT pCt )
	{
		WaitRun = TRUE;
		ListView_SetItemText( FlagListWnd, 0, 0, "Pause" );

		while(WaitRun == TRUE)
			os::Sleep( 100 );

		//设置TF位让他单步走
		if(bwillTF == 1)
			pCt->EFlags |= TF;

		ListView_SetItemText( FlagListWnd, 0, 0, "Run" );
	}

	//
	//直接继续
	VOID SetUserRun()
	{
		bwillTF = 0; //下一步直接走
		WaitRun = FALSE;
	}
	//
	//单步继续
	VOID SetStepUserRun()
	{
		bwillTF = 1; //下一步继续断下
		WaitRun = FALSE;
	}


	/************************************************************************/
	/* 读取设置的条件
	/************************************************************************/
	BOOL bTJ_inNext = 0;	//正在tf中 下个硬断前重置条件断点
	DWORD curtjaddr = 0;	//下断点地址
	BYTE curtjaddr_byte = 0;//断点地址的字节
	int curi_tj1 = -1;		//这个是当前设置的第一个条件的序号 用来寻址alltj
	char cur_tj2[5];		//条件2  {"==","<","<=",">",">=","!="};
	DWORD cur_zz = 0;		//当前表达式的值 条件3
	BOOL bLogTj = 0;		//是否记录未满足条件的调用

	//记录
	VOID LogCurContext( PCONTEXT pCt )
	{
		if(bLogTj)
		{
			////专用 怪物猎人发包
			//fsdebug( "(RET: %08X)\n\t\t\t+0C: %08x   +10:%08x   +14:%08x   +18:%08x  +1C:%08x\n\t\t\t+20: %08x   +24:%08x   +28:%08x   +2C:%08x",
			//		 GETPD( pCt->Esp ),
			//		 GETPD( pCt->Esp + 0xC ), GETPD( pCt->Esp + 0x10 ), GETPD( pCt->Esp + 0x14 ), GETPD( pCt->Esp + 0x18 ), GETPD( pCt->Esp + 0x1C ),
			//		 GETPD( pCt->Esp + 0x20 ), GETPD( pCt->Esp + 0x24 ), GETPD( pCt->Esp + 0x28 ), GETPD( pCt->Esp + 0x2C ) );

			/*
			fsdebug("[条件断点]  EIP: %08X (S1: %08X  S2: %08X  RET: %08X)   \n\t\t(Eax: %08X  Ebx: %08X  Ecx: %08X  Edx: %08X  Esi: %08X  Edi: %08X)",
			pCt->Eip,GETPD(pCt->Esp+4),GETPD(pCt->Esp+8),GETPD(pCt->Esp),
			pCt->Eax,pCt->Ebx,pCt->Ecx,pCt->Edx,pCt->Esi,pCt->Edi);
			*/
		}
	}

	//删除条件
	BOOL DelBreakTj()
	{
		if(curtjaddr)
		{
			debug( "成功删除条件断点[%X]", curtjaddr );

			//还原curtjaddr处地址
			os::ctVirtualProtect( curtjaddr, 1 );
			GETPB( curtjaddr ) = curtjaddr_byte;
			//设为0
			curtjaddr = 0;

			FsDisasmLastAddr();
			return TRUE;
		}

		return FALSE;
	}

	//去掉str中的空格 前后
	VOID killspace( PCH buf )
	{
		char *p = buf;
		while(*p != '\0')
		{
			if(*p == ' ')
			{
				while(*p != '\0')
				{
					*p = *(p + 1);
					p++;
				}
			}
			else p++;
		}
	}

	//设置条件
	BOOL SetBreakTj( HWND TjWnd )
	{
#define TJ1_COUNT	19
		char alltj1str[TJ1_COUNT][30] = {"eax","ebx","ecx","edx","esi","edi",
			"[esp]","[esp+4]","[esp+8]","[esp+0xC]","[esp+0x10]","[esp+0x14]",
			"[esp+0x18]","[esp+0x1C]","[esp+0x20]","[esp+0x24]","[esp+0x28]","[esp+0x2C]","[esp+0x30]"};

		PCH tj2str[6] = {"==","<","<=",">",">=","!="};

		//分开几个目标 1-3
		char tj1[200], tj2[10] = {0}, tj3[50] = {0}, addr_str[50] = {0};
		GetDlgItemTextA( TjWnd, IDC_EDIT1, tj1, 200 );
		for(int i = 0; i < 6; i++)
		{
			PCH tmp = strstr( tj1, tj2str[i] );
			if(tmp)
			{
				lstrcpyA( tj3, tmp + 2 );		//目标数
				tj2[0] = tmp[0];			//条件2
				tj2[1] = tmp[1];
				tj2[2] = 0;

				tmp[0] = 0;					//条件1
				break;
			}
		}

		killspace( tj1 );
		killspace( tj2 );
		killspace( tj3 );

		debug( "得到条件： %s%s%s", tj1, tj2, tj3 );
		GetDlgItemTextA( TjWnd, IDC_EDIT4, addr_str, 50 );
		if(tj1[0] && tj2[0] && tj3[0])
		{
			for(int i = 0; i < TJ1_COUNT; i++)
			{
				if(lstrcmpiA( alltj1str[i], tj1 ) == 0)
				{
					curi_tj1 = i;
					lstrcpyA( cur_tj2, tj2 );
					cur_zz = strtol( tj3, 0, 0 );

					curtjaddr = strtol( addr_str, 0, 16 );
					curtjaddr_byte = GETPB( curtjaddr );
					bLogTj = IsDlgButtonChecked( TjWnd, IDC_CHECK1 );

					debug( "增加条件断点 (%s) ： %s %s %s   log=%d", addr_str, tj1, tj2, tj3, bLogTj );

					//断点
					os::ctVirtualProtect( curtjaddr, 1 );
					GETPB( curtjaddr ) = 0xCC;

					FsDisasmLastAddr();
					return TRUE;
				}
			}
		}

		return FALSE;
	}

	//读取条件是否满足
	BOOL BreakTj( PCONTEXT pCt )
	{
		if(curtjaddr == 0 || curi_tj1 == -1)
			return FALSE;

		DWORD alltj1[] = {pCt->Eax,pCt->Ebx,pCt->Ecx,pCt->Edx,pCt->Esi,pCt->Edi,
			GETPD( pCt->Esp + 0 ),GETPD( pCt->Esp + 4 ),GETPD( pCt->Esp + 8 ),GETPD( pCt->Esp + 0xC ),GETPD( pCt->Esp + 0x10 ),GETPD( pCt->Esp + 0x14 ),
			GETPD( pCt->Esp + 0x18 ),GETPD( pCt->Esp + 0x1C ),GETPD( pCt->Esp + 0x20 ),GETPD( pCt->Esp + 0x24 ),GETPD( pCt->Esp + 0x28 ),GETPD( pCt->Esp + 0x2C ),GETPD( pCt->Esp + 0x30 )};


		//看是否满足
		if(lstrcmpiA( cur_tj2, "==" ) == 0)
			return (alltj1[curi_tj1] == cur_zz);
		if(lstrcmpiA( cur_tj2, "<" ) == 0)
			return (alltj1[curi_tj1] < cur_zz);
		if(lstrcmpiA( cur_tj2, "<=" ) == 0)
			return (alltj1[curi_tj1] <= cur_zz);
		if(lstrcmpiA( cur_tj2, ">" ) == 0)
			return (alltj1[curi_tj1] > cur_zz);
		if(lstrcmpiA( cur_tj2, ">=" ) == 0)
			return (alltj1[curi_tj1] >= cur_zz);
		if(lstrcmpiA( cur_tj2, "!=" ) == 0)
			return (alltj1[curi_tj1] != cur_zz);

		return FALSE;
	}


	//
	// 显示当前的调用堆栈
	//	@param1 == 堆栈显示第一个地址的地址
	VOID ShowStack( DWORD StartAddr )
	{
		last_StackStartAddr = StartAddr;

		for(int i = 0; i <= STACK_SIZE; i++)
		{
			char buf[20];

			wsprintfA( buf, "%08X", StartAddr + i * 4 );
			ListView_SetItemText( StackListWnd, i, 0, buf );

			if(IsBadReadPtr( (PVOID)(StartAddr + i * 4), 4 ) == FALSE)
				wsprintfA( buf, "%08X", GETPD( StartAddr + i * 4 ) );
			else
				wsprintfA( buf, "???" );

			ListView_SetItemText( StackListWnd, i, 1, buf );
		}

		ListView_EnsureVisible( StackListWnd, 0, TRUE );
	}

	//
	// 堆栈里显示这个list里选中的地址
	// 注意： 这个list里必须item=1的为地址
	VOID ShowStackForListWnd( HWND ListWnd )
	{
		char bufaddr[50] = {0};
		if(os::ListView_GetSelectStr( ListWnd, 1, bufaddr, 50 ) == 0)
			return;

		DWORD addr = strtol( bufaddr, 0, 16 );
		if(addr && IsBadReadPtr( (PVOID)addr, 1 ) == FALSE)
		{
			ShowStack( addr );
		}
	}

	//
	// 显示当前的寄存器
	VOID ShowRegister( PCONTEXT pContext )
	{
		PDWORD	RegisterAddr[] = {&pContext->Eax,&pContext->Ecx,&pContext->Edx,
			&pContext->Ebx,&pContext->Esp,&pContext->Ebp,&pContext->Esi,&pContext->Edi,
			&pContext->Eip,&pContext->Eip};

		//写入及寄存器的值
		for(int i = 9; i >= 0; i--)
		{
			if(i == 8)
			{
				ListView_SetItemText( RegisterListWnd, i, 1, " " );
			}
			else
			{
				char buf[20];
				wsprintfA( buf, "%08X", GETPD( RegisterAddr[i] ) );
				ListView_SetItemText( RegisterListWnd, i, 1, buf );
			}
		}
	}




	/************************************************************************/
	/* 断点处理
	/************************************************************************/
	//等待用户处理并显示当前堆栈寄存器等
	VOID WaitUserPressRun( PCONTEXT pCt )
	{
		if(GetCurrentThreadId() != DebugMainThread)
		{
			//fsdebug("已暂停[TID:%d] [Eip:%08X] ",GetCurrentThreadId(),pCt->Eip);
			debug( "已暂停[TID:%d] [Eip:%08X] ", GetCurrentThreadId(), pCt->Eip );
			FsDisasm( pCt->Eip );

			td::SuspendAllThreads( TRUE, DebugMainThread );	//暂停所有线程

			ShowRegister( pCt );			//显示寄存器信息
			ShowStack( pCt->Esp );		//显示堆栈

			WaitUserRun( pCt );				//等待按键...

			td::SuspendAllThreads( FALSE, DebugMainThread );	//恢复所有线程
		}
	}

	//
	//断点总处理
	int BreakPointFilterProc( PCONTEXT pCt )
	{
		//是否我设置的int3断点
		if(IsInCCRecord( pCt->Eip ))
		{
			DelCC( pCt->Eip );			//删除断点 [一次性]
			WaitUserPressRun( pCt );
			return EXCEPTION_CONTINUE_EXECUTION;
		}

		//如果是条件断点并且未满足条件的话可以设TF=1 然后再断下来时重置cc断点
		if(pCt->Eip == curtjaddr)
		{
			if(BreakTj( pCt ))
			{
				//满足 - 暂停
				WaitUserPressRun( pCt );
				//然后设置tf位  下个的时候
			}
			else
			{
				LogCurContext( pCt );
			}

			//如果这个条件断点没被删除的话就重置他
			if(curtjaddr)
			{
				GETPB( curtjaddr ) = curtjaddr_byte;		//还原这个代码以便继续运行
				bTJ_inNext = TRUE;					//重置断点的状态标志
				pCt->EFlags |= TF;					//设置TF位继续让他单步走
			}

			return EXCEPTION_CONTINUE_EXECUTION;
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}

	//
	//判断是否我的硬断 传入80000004中的dr几断下时
	BOOL IsMyHardPoint( int nIndex, PCONTEXT pCt )
	{
		if(
			(nIndex == 1 && m_Dr_Use.Dr0_addr == pCt->Dr0) ||
			(nIndex == 2 && m_Dr_Use.Dr1_addr == pCt->Dr1) ||
			(nIndex == 4 && m_Dr_Use.Dr2_addr == pCt->Dr2) ||
			(nIndex == 8 && m_Dr_Use.Dr3_addr == pCt->Dr3)
			)
		{
			return TRUE;
		}

		return FALSE;
	}

	//
	//硬断总处理
	DWORD iHbp_inNext = 0;		//需要在tf里重置硬断  1-4
	int HardPointFilterProc( PCONTEXT pCt, int nIndex )
	{
		sys::fsdebug("*(HardPointFilterProc) TID = [%X]   EIP->[%X]  *",GetCurrentThreadId(),pCt->Eip);

		//断下的是否是我的断点
		if(IsMyHardPoint( nIndex, pCt ))
		{
			debug( "硬断断下" );
			WaitUserPressRun( pCt );

			//如果没有被删除的话 则下个代码处重设硬断
			if(IsMyHardPoint( nIndex, pCt ))
			{
				iHbp_inNext = nIndex;				//设置一个标志让他在单步中恢复			
				pCt->EFlags |= TF;					//设置TF位继续让他单步走
			}

			//使硬断无效
			DR7 tagDr7 = {0};
			tagDr7.dwDr7 = pCt->Dr7;
			switch(nIndex)
			{
				case 1:
					tagDr7.DRFlag.L0 = 0;
					break;
				case 2:
					tagDr7.DRFlag.L1 = 0;
					break;
				case 4:
					tagDr7.DRFlag.L2 = 0;
					break;
				case 8:
					tagDr7.DRFlag.L3 = 0;
					break;
			}
			pCt->Dr7 = tagDr7.dwDr7;

			return EXCEPTION_CONTINUE_EXECUTION;
		}
		else
		{
			sys::fsdebug( "the exception is not my!" );
		}


		return EXCEPTION_CONTINUE_SEARCH;
	}

	/************************************************************************/
	/*  总处理流程
	/************************************************************************/
#if ANTI_OSAPI_DETECTION
	LONG __cdecl Fs_HPFilter_Start( PEXCEPTION_RECORD pEtrd, PCONTEXT pCt )
	{
#else
	LONG WINAPI FsExceptionHandler( PEXCEPTION_POINTERS pExceptionInfo )
	{
		PEXCEPTION_RECORD pEtrd = pExceptionInfo->ExceptionRecord;
		PCONTEXT pCt = pExceptionInfo->ContextRecord;
#endif
		sys::fsdebug("# TID = [%X] EXCEPTION_CONTINUE_SEARCH : EIP->[%X]  pEtrd->ExceptionCode : [%X]",GetCurrentThreadId(),pCt->Eip,pEtrd->ExceptionCode);

		//软件断点
		if(pEtrd->ExceptionCode == EXCEPTION_BREAKPOINT)
		{
			if(BreakPointFilterProc( pCt ) == EXCEPTION_CONTINUE_EXECUTION)
				return EXCEPTION_CONTINUE_EXECUTION;
		}

		//硬件断点
		if(pEtrd->ExceptionCode == EXCEPTION_SINGLE_STEP)
		{
			//判断是否硬件断点 而不是单步
			//判断Dr6的低4位是否为0  <- 应该是1,2,4,8
			int nIndex = pCt->Dr6 & 0xf;

			sys::fsdebug( "{TID=%d}  [%d]  [m_Dr_Use.Dr0:%d] 硬断点: eip=%X   [Dr0:%X  1:%X  2:%X  3:%X]",
						  GetCurrentThreadId(), nIndex, m_Dr_Use.Dr0, pCt->Eip, pCt->Dr0, pCt->Dr1, pCt->Dr2, pCt->Dr3 );

			//单步
			if(nIndex == 0)
			{
				if(bwillTF == 1)
				{
					debug( "单步断下 eip=%X", pCt->Eip );
					WaitUserPressRun( pCt );
					return EXCEPTION_CONTINUE_EXECUTION;
				}

				//是否需要重置条件断点
				if(bTJ_inNext == TRUE)
				{
					GETPB( curtjaddr ) = 0xCC;
					bTJ_inNext = FALSE;
					debug( "单步中重置条件断点  eip=%X", pCt->Eip );
					return EXCEPTION_CONTINUE_EXECUTION;
				}

				//是否重置硬断
				if(iHbp_inNext)
				{
					DR7 tagDr7 = {0};
					tagDr7.dwDr7 = pCt->Dr7;
					switch(iHbp_inNext)
					{
						case 1:
							tagDr7.DRFlag.L0 = 1;
							break;
						case 2:
							tagDr7.DRFlag.L1 = 1;
							break;
						case 3:
							tagDr7.DRFlag.L2 = 1;
							break;
						case 4:
							tagDr7.DRFlag.L3 = 1;
							break;
					}

					pCt->Dr7 = tagDr7.dwDr7;
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}

			//硬断
			if(nIndex == 1 || nIndex == 2 || nIndex == 4 || nIndex == 8)
			{
				if(HardPointFilterProc( pCt, nIndex ) == EXCEPTION_CONTINUE_EXECUTION)
					return EXCEPTION_CONTINUE_EXECUTION;
			}

			sys::fsdebug( "WOW!" );
			return EXCEPTION_CONTINUE_SEARCH;
		}

		//排除掉当前调试器的主线程
		if(GetCurrentThreadId() == DebugMainThread)
		{
			//这里也可能是 Isbadreadxx
			//fsdebug("# # # # TID = [%X] EXCEPTION_CONTINUE_SEARCH : EIP->[%X]  pEtrd->ExceptionCode : [%X]# # # #",GetCurrentThreadId(),pCt->Eip,pEtrd->ExceptionCode);
			//MessageBoxA(0,"ERROR!",0,MB_ICONERROR);
			return EXCEPTION_CONTINUE_SEARCH;
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}


#if ANTI_OSAPI_DETECTION
	/************************************************************************/
	/* 获取接管异常地址
	/************************************************************************/
	DWORD GetPKiUserExceptionDispatcherAddr()
	{
		/*
		(新的)接管异常方式：  only for Win7x64-Sp1
		Wow64SetupExceptionDispatch函数的目的便是将32位上下文中的程序指针飞到全局变量wow64!Ntdll32KiUserExceptionDispatcher飞到32位NTDLL中的KiUserExceptionDispatcher。
		00000000`745dc707 8b0d73e80200    mov     ecx,dword ptr [wow64!Ntdll32KiUserExceptionDispatcher (00000000`7460af80)] ds:00000000`7460af80=773c0124				<< 只需要改掉这里就行了
		00000000`745dc70d ff151d58ffff    call    qword ptr [wow64!_imp_CpuSetInstructionPointer (00000000`745d1f30)]

		在x86的汇编是这样的：
		745dc707 - 8B 0D 73E80200        - mov ecx,[0002E873]

		需要经过换算  745dc707 + 5 + 0002E873 + 1  即可  因为是固定的值  所以直接
		wow64!Ntdll32KiUserExceptionDispatcher_addr  =  745dc707 + 2E873 + 6  ;
		*/

		//获取 用特征码获取 把这个特征码加密
		//这里每个抽出来4个字符就好了  8B0D73E80200 
		char tzall[50];
		char tzs1[20] = "8b0d02010503"; char tzs2[20] = "73e8e8f8g8h8j8"; char tzs3[20] = "0200000000000002";
		tzs1[4] = 0; tzs2[4] = 0; tzs3[4] = 0;
		wsprintfA( tzall, "%s%s%s", tzs1, tzs2, tzs3 );

		DWORD Ntdll32KiUserExceptionDispatcher_Addr = _SeachUpdate( tzall, FIND_PY, 0, 1, 0x50000000 );		//8b0d73e80200 
		if(Ntdll32KiUserExceptionDispatcher_Addr == 0)
		{
			os::MsgBox("error get addr");
			return 0;
		}

		Ntdll32KiUserExceptionDispatcher_Addr = Ntdll32KiUserExceptionDispatcher_Addr + 0x2E873 + 6;		//add offset
		return Ntdll32KiUserExceptionDispatcher_Addr;
	}

	/************************************************************************/
	/* hook KiUserExceptionDispatcher 用来处理断点
	/************************************************************************/
	DWORD		ZwContinueCall = 0;
	DWORD		oriKiUserExceptionDispatcherAddr = 0;

	NAKED VOID Ori_ZwContinue( PCONTEXT pContext, DWORD nil )
	{
		_asm
		{
			mov  eax, 0x40
			mov  ecx, ZwContinueCall
			add  ecx, 5
			jmp  ecx
		}
	}
	NAKED VOID NTAPI MyKiUserExceptionDispatcher( PEXCEPTION_RECORD pExcptRec, PCONTEXT pContext )
	{
		_asm
		{
			cld
			mov   ecx, dword ptr[esp + 4]	    // pContext
			mov   ebx, dword ptr[esp]		// pExcptRec

			push  ecx
			push  ebx
			call  Fs_HPFilter_Start		//my proc
			add   esp, 8

			cmp   eax, 1						//EXCEPTION_EXECUTE_HANDLER  走原始
			je    _rs
			cmp   eax, 0						//EXCEPTION_CONTINUE_SEARCH  走原始
			je    _rs

			//修复完了 回去就行了 -1
			mov   ecx, dword ptr[esp + 4]	    // pContext
			push  0
			push  ecx
			call  Ori_ZwContinue			//ZwContinueCall
			retn  8


			_rs:
			jmp   oriKiUserExceptionDispatcherAddr			//跳到原始的函数里
		}
	}
	BOOL HookKiUserExceptionDispatcher()
	{
		return TRUE;


		ZwContinueCall = os::ctGetProcAddress( GetModuleHandleA( "ntdll.dll" ), ("ZwContinue") );
		if(ZwContinueCall)
		{
			DWORD pKiUserExceptionDispatcherAddr = GetPKiUserExceptionDispatcherAddr();
			oriKiUserExceptionDispatcherAddr = GETPD( pKiUserExceptionDispatcherAddr );
			GETPD( pKiUserExceptionDispatcherAddr ) = (DWORD)MyKiUserExceptionDispatcher;			//HOOK!
			return TRUE;
		}
		return FALSE;
	}
}
#else

	BOOL HookKiUserExceptionDispatcher()
	{
		// 这里只简单的测试下，所以只挂了SEH
		//
		SetUnhandledExceptionFilter( FsExceptionHandler );
		return 1;
	}

#endif
}