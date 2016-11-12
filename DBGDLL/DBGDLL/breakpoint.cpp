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
	DR_USE m_Dr_Use = {0};		//��ǰ���õ�4��Ӳ���ϵ�

	//
	// Ӳ���ϵ�
	//���ص�ǰ���õĵ��ԼĴ���
	int FindFreeDebugRegister( DWORD setbpaddr )
	{
		//���е�ʱ��
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

		//���Dr0-Dr3����ʹ���򷵻�-1
		return -1;
	}

	//����Ӳ���ϵ� ���� ��ַ ���� ����
	//dwAttribute 0��ʾִ�жϵ� 3��ʾ���ʶϵ� 1 ��ʾд��ϵ�
	//dwLength ȡֵ 1 2 4
	//nIndex   �ڼ���dr�Ĵ���
	VOID SetDr7( PCONTEXT pCt, DWORD dwBpAddress, DWORD dwAttribute, DWORD dwLength, int nIndex )
	{
		//��ֵ�����DR7�ṹ��,����ʡȥ��λ�Ʋ����ķ���
		DR7 tagDr7 = {0};
		tagDr7.dwDr7 = pCt->Dr7;

		switch(nIndex)
		{
			case 0:
				//�жϵ�ַ
				pCt->Dr0 = dwBpAddress;
				//�ϵ㳤��
				tagDr7.DRFlag.len0 = dwLength - 1;
				//����
				tagDr7.DRFlag.rw0 = dwAttribute;
				//�ֲ��ϵ�
				tagDr7.DRFlag.L0 = 1;
				//���ñ�־λ��¼���ԼĴ�����ʹ�����
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

		//��ֵ��ȥ
		pCt->Dr7 = tagDr7.dwDr7;
	}

	//���Ӳ��
	//dwAttribute 0��ʾִ�жϵ� 3��ʾ���ʶϵ� 1 ��ʾд��ϵ�
	//dwLength ȡֵ 1 2 4
	void SetHardBP( DWORD dwBpAddress, DWORD dwAttribute, DWORD dwLength, DWORD onlytid )
	{
		//ǿ�ư�ִ�жϵ�ĳ��ȸ�Ϊ1
		if(dwAttribute == 0)
			dwLength = 1;

		if(dwLength != 1 && dwLength != 2 && dwLength != 4)
		{
			os::MsgBox( "�ϵ㳤�����ô���" );
			return;
		}

		//��õ�ǰ���е��Ĵ������
		int nIndex = FindFreeDebugRegister( dwBpAddress );
		if(nIndex == -1)
		{
			os::MsgBox( "��ǰӲ���ϵ�����,��ɾ��������", 0, 0 );
			return;
		}
		if(nIndex == -2)
		{
			os::MsgBox( "��ǰ��ַ[%X]����Ӳ���ϵ�,��ɾ��������", dwBpAddress );
			return;
		}

		td::SetAllThreadsDr( DebugMainThread, dwBpAddress, dwAttribute, dwLength, onlytid );
		debug( "Ӳ���ϵ�[%X]��ӳɹ�", dwBpAddress );
	}

	//
	//ɾ��Ӳ���ϵ�  s1=�ڼ��� 0-3
	VOID DelHardPoint( int idr )
	{
		if(idr < 4)
		{
			debug( "ɾ��Ӳ���ϵ㣺 (%d)", idr );

			switch(idr)
			{
				case 0:
					m_Dr_Use.Dr0 = 0;
					m_Dr_Use.Dr0_addr = 0;
					td::DelAllThreadsDrs( DebugMainThread, idr, WaitRun );		//WaitRun==TRUE��ʱ��ִ����ͣ/�����̵߳Ķ���
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

			//ˢ��һ��
			ListView_SetItemCount( ASMlistWnd, 4 );
		}
	}

	//
	// ��ָ��������ListView�л�ȡ�ϵ�
	VOID ListView_GetBp( NMLVDISPINFO* plvdi )
	{
		int list_iItem = plvdi->item.iItem;			//��
		int list_isub = plvdi->item.iSubItem;		//�е�����

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
				//�Ѵ���
				addr = allatt[list_iItem * 3 + 0];
				att = allatt[list_iItem * 3 + 1];
				len = allatt[list_iItem * 3 + 2];
			}

			switch(list_isub)
			{
				case 0:		//��ַ
					wsprintfA( plvdi->item.pszText, "%08X", addr );
					break;
				case 1:		//����
							//dwAttribute 0��ʾִ�жϵ� 3��ʾ���ʶϵ� 1 ��ʾд��ϵ�
					switch(att)
					{
						case 0:
							wsprintfA( plvdi->item.pszText, "ִ�жϵ�" );
							break;
						case 1:
							wsprintfA( plvdi->item.pszText, "д��ϵ�" );
							break;
						case 3:
							wsprintfA( plvdi->item.pszText, "���ʶϵ�" );
							break;
						default:
							wsprintfA( plvdi->item.pszText, "δʹ��" );
							break;
					}
					break;
				case 2:		//����
					wsprintfA( plvdi->item.pszText, "%d", len );
					break;
			}
		}
	}

	//
	// ��������ϵ� 0xCC
	// ����ϵ�֧��100��
	//
	struct ctSoftBreakPoint
	{
		DWORD addr;
		BYTE  ori_byte;
	};
	ctSoftBreakPoint cc_breakpoint_Array[100] = {0};

	//�ϵ��Ƿ������ݿ���
	BOOL IsInCCRecord( DWORD addr )
	{
		//�ȿ���û����ͬ��
		for(int i = 0; i < 100; i++)
		{
			if(cc_breakpoint_Array[i].addr == addr)
				return TRUE;
		}

		return FALSE;
	}

	//
	//��Ӷϵ�
	BOOL AddCCToRecord( DWORD addr, DWORD nextaddr )
	{
		if(IsInCCRecord( addr ))
			return TRUE;

		//û�еĻ��Ҹ��հ׵ĵط�ֱ��д��ϵ�
		for(int i = 0; i < 100; i++)
		{
			if(cc_breakpoint_Array[i].addr == 0)
			{
				cc_breakpoint_Array[i].addr = addr;
				cc_breakpoint_Array[i].ori_byte = GETPB( addr );
				return TRUE;
			}
		}

		os::MsgBox( "�ϵ�����������" );
		return FALSE;
	}

	//
	//ɾ���ϵ�
	BOOL DelCC( DWORD addr )
	{
		for(int i = 0; i < 100; i++)
		{
			if(cc_breakpoint_Array[i].addr == addr)
			{
				os::ctVirtualProtect( addr, 1 );
				GETPB( addr ) = cc_breakpoint_Array[i].ori_byte;

				//���
				cc_breakpoint_Array[i].addr = 0;
				return TRUE;
			}
		}

		return FALSE;
	}

	//
	//���/ɾ���ϵ�
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
	// �ȴ�����F9
	VOID WaitUserRun( PCONTEXT pCt )
	{
		WaitRun = TRUE;
		ListView_SetItemText( FlagListWnd, 0, 0, "Pause" );

		while(WaitRun == TRUE)
			os::Sleep( 100 );

		//����TFλ����������
		if(bwillTF == 1)
			pCt->EFlags |= TF;

		ListView_SetItemText( FlagListWnd, 0, 0, "Run" );
	}

	//
	//ֱ�Ӽ���
	VOID SetUserRun()
	{
		bwillTF = 0; //��һ��ֱ����
		WaitRun = FALSE;
	}
	//
	//��������
	VOID SetStepUserRun()
	{
		bwillTF = 1; //��һ����������
		WaitRun = FALSE;
	}


	/************************************************************************/
	/* ��ȡ���õ�����
	/************************************************************************/
	BOOL bTJ_inNext = 0;	//����tf�� �¸�Ӳ��ǰ���������ϵ�
	DWORD curtjaddr = 0;	//�¶ϵ��ַ
	BYTE curtjaddr_byte = 0;//�ϵ��ַ���ֽ�
	int curi_tj1 = -1;		//����ǵ�ǰ���õĵ�һ����������� ����Ѱַalltj
	char cur_tj2[5];		//����2  {"==","<","<=",">",">=","!="};
	DWORD cur_zz = 0;		//��ǰ���ʽ��ֵ ����3
	BOOL bLogTj = 0;		//�Ƿ��¼δ���������ĵ���

	//��¼
	VOID LogCurContext( PCONTEXT pCt )
	{
		if(bLogTj)
		{
			////ר�� �������˷���
			//fsdebug( "(RET: %08X)\n\t\t\t+0C: %08x   +10:%08x   +14:%08x   +18:%08x  +1C:%08x\n\t\t\t+20: %08x   +24:%08x   +28:%08x   +2C:%08x",
			//		 GETPD( pCt->Esp ),
			//		 GETPD( pCt->Esp + 0xC ), GETPD( pCt->Esp + 0x10 ), GETPD( pCt->Esp + 0x14 ), GETPD( pCt->Esp + 0x18 ), GETPD( pCt->Esp + 0x1C ),
			//		 GETPD( pCt->Esp + 0x20 ), GETPD( pCt->Esp + 0x24 ), GETPD( pCt->Esp + 0x28 ), GETPD( pCt->Esp + 0x2C ) );

			/*
			fsdebug("[�����ϵ�]  EIP: %08X (S1: %08X  S2: %08X  RET: %08X)   \n\t\t(Eax: %08X  Ebx: %08X  Ecx: %08X  Edx: %08X  Esi: %08X  Edi: %08X)",
			pCt->Eip,GETPD(pCt->Esp+4),GETPD(pCt->Esp+8),GETPD(pCt->Esp),
			pCt->Eax,pCt->Ebx,pCt->Ecx,pCt->Edx,pCt->Esi,pCt->Edi);
			*/
		}
	}

	//ɾ������
	BOOL DelBreakTj()
	{
		if(curtjaddr)
		{
			debug( "�ɹ�ɾ�������ϵ�[%X]", curtjaddr );

			//��ԭcurtjaddr����ַ
			os::ctVirtualProtect( curtjaddr, 1 );
			GETPB( curtjaddr ) = curtjaddr_byte;
			//��Ϊ0
			curtjaddr = 0;

			FsDisasmLastAddr();
			return TRUE;
		}

		return FALSE;
	}

	//ȥ��str�еĿո� ǰ��
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

	//��������
	BOOL SetBreakTj( HWND TjWnd )
	{
#define TJ1_COUNT	19
		char alltj1str[TJ1_COUNT][30] = {"eax","ebx","ecx","edx","esi","edi",
			"[esp]","[esp+4]","[esp+8]","[esp+0xC]","[esp+0x10]","[esp+0x14]",
			"[esp+0x18]","[esp+0x1C]","[esp+0x20]","[esp+0x24]","[esp+0x28]","[esp+0x2C]","[esp+0x30]"};

		PCH tj2str[6] = {"==","<","<=",">",">=","!="};

		//�ֿ�����Ŀ�� 1-3
		char tj1[200], tj2[10] = {0}, tj3[50] = {0}, addr_str[50] = {0};
		GetDlgItemTextA( TjWnd, IDC_EDIT1, tj1, 200 );
		for(int i = 0; i < 6; i++)
		{
			PCH tmp = strstr( tj1, tj2str[i] );
			if(tmp)
			{
				lstrcpyA( tj3, tmp + 2 );		//Ŀ����
				tj2[0] = tmp[0];			//����2
				tj2[1] = tmp[1];
				tj2[2] = 0;

				tmp[0] = 0;					//����1
				break;
			}
		}

		killspace( tj1 );
		killspace( tj2 );
		killspace( tj3 );

		debug( "�õ������� %s%s%s", tj1, tj2, tj3 );
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

					debug( "���������ϵ� (%s) �� %s %s %s   log=%d", addr_str, tj1, tj2, tj3, bLogTj );

					//�ϵ�
					os::ctVirtualProtect( curtjaddr, 1 );
					GETPB( curtjaddr ) = 0xCC;

					FsDisasmLastAddr();
					return TRUE;
				}
			}
		}

		return FALSE;
	}

	//��ȡ�����Ƿ�����
	BOOL BreakTj( PCONTEXT pCt )
	{
		if(curtjaddr == 0 || curi_tj1 == -1)
			return FALSE;

		DWORD alltj1[] = {pCt->Eax,pCt->Ebx,pCt->Ecx,pCt->Edx,pCt->Esi,pCt->Edi,
			GETPD( pCt->Esp + 0 ),GETPD( pCt->Esp + 4 ),GETPD( pCt->Esp + 8 ),GETPD( pCt->Esp + 0xC ),GETPD( pCt->Esp + 0x10 ),GETPD( pCt->Esp + 0x14 ),
			GETPD( pCt->Esp + 0x18 ),GETPD( pCt->Esp + 0x1C ),GETPD( pCt->Esp + 0x20 ),GETPD( pCt->Esp + 0x24 ),GETPD( pCt->Esp + 0x28 ),GETPD( pCt->Esp + 0x2C ),GETPD( pCt->Esp + 0x30 )};


		//���Ƿ�����
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
	// ��ʾ��ǰ�ĵ��ö�ջ
	//	@param1 == ��ջ��ʾ��һ����ַ�ĵ�ַ
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
	// ��ջ����ʾ���list��ѡ�еĵ�ַ
	// ע�⣺ ���list�����item=1��Ϊ��ַ
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
	// ��ʾ��ǰ�ļĴ���
	VOID ShowRegister( PCONTEXT pContext )
	{
		PDWORD	RegisterAddr[] = {&pContext->Eax,&pContext->Ecx,&pContext->Edx,
			&pContext->Ebx,&pContext->Esp,&pContext->Ebp,&pContext->Esi,&pContext->Edi,
			&pContext->Eip,&pContext->Eip};

		//д�뼰�Ĵ�����ֵ
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
	/* �ϵ㴦��
	/************************************************************************/
	//�ȴ��û�������ʾ��ǰ��ջ�Ĵ�����
	VOID WaitUserPressRun( PCONTEXT pCt )
	{
		if(GetCurrentThreadId() != DebugMainThread)
		{
			//fsdebug("����ͣ[TID:%d] [Eip:%08X] ",GetCurrentThreadId(),pCt->Eip);
			debug( "����ͣ[TID:%d] [Eip:%08X] ", GetCurrentThreadId(), pCt->Eip );
			FsDisasm( pCt->Eip );

			td::SuspendAllThreads( TRUE, DebugMainThread );	//��ͣ�����߳�

			ShowRegister( pCt );			//��ʾ�Ĵ�����Ϣ
			ShowStack( pCt->Esp );		//��ʾ��ջ

			WaitUserRun( pCt );				//�ȴ�����...

			td::SuspendAllThreads( FALSE, DebugMainThread );	//�ָ������߳�
		}
	}

	//
	//�ϵ��ܴ���
	int BreakPointFilterProc( PCONTEXT pCt )
	{
		//�Ƿ������õ�int3�ϵ�
		if(IsInCCRecord( pCt->Eip ))
		{
			DelCC( pCt->Eip );			//ɾ���ϵ� [һ����]
			WaitUserPressRun( pCt );
			return EXCEPTION_CONTINUE_EXECUTION;
		}

		//����������ϵ㲢��δ���������Ļ�������TF=1 Ȼ���ٶ�����ʱ����cc�ϵ�
		if(pCt->Eip == curtjaddr)
		{
			if(BreakTj( pCt ))
			{
				//���� - ��ͣ
				WaitUserPressRun( pCt );
				//Ȼ������tfλ  �¸���ʱ��
			}
			else
			{
				LogCurContext( pCt );
			}

			//�����������ϵ�û��ɾ���Ļ���������
			if(curtjaddr)
			{
				GETPB( curtjaddr ) = curtjaddr_byte;		//��ԭ��������Ա��������
				bTJ_inNext = TRUE;					//���öϵ��״̬��־
				pCt->EFlags |= TF;					//����TFλ��������������
			}

			return EXCEPTION_CONTINUE_EXECUTION;
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}

	//
	//�ж��Ƿ��ҵ�Ӳ�� ����80000004�е�dr������ʱ
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
	//Ӳ���ܴ���
	DWORD iHbp_inNext = 0;		//��Ҫ��tf������Ӳ��  1-4
	int HardPointFilterProc( PCONTEXT pCt, int nIndex )
	{
		sys::fsdebug("*(HardPointFilterProc) TID = [%X]   EIP->[%X]  *",GetCurrentThreadId(),pCt->Eip);

		//���µ��Ƿ����ҵĶϵ�
		if(IsMyHardPoint( nIndex, pCt ))
		{
			debug( "Ӳ�϶���" );
			WaitUserPressRun( pCt );

			//���û�б�ɾ���Ļ� ���¸����봦����Ӳ��
			if(IsMyHardPoint( nIndex, pCt ))
			{
				iHbp_inNext = nIndex;				//����һ����־�����ڵ����лָ�			
				pCt->EFlags |= TF;					//����TFλ��������������
			}

			//ʹӲ����Ч
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
	/*  �ܴ�������
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

		//����ϵ�
		if(pEtrd->ExceptionCode == EXCEPTION_BREAKPOINT)
		{
			if(BreakPointFilterProc( pCt ) == EXCEPTION_CONTINUE_EXECUTION)
				return EXCEPTION_CONTINUE_EXECUTION;
		}

		//Ӳ���ϵ�
		if(pEtrd->ExceptionCode == EXCEPTION_SINGLE_STEP)
		{
			//�ж��Ƿ�Ӳ���ϵ� �����ǵ���
			//�ж�Dr6�ĵ�4λ�Ƿ�Ϊ0  <- Ӧ����1,2,4,8
			int nIndex = pCt->Dr6 & 0xf;

			sys::fsdebug( "{TID=%d}  [%d]  [m_Dr_Use.Dr0:%d] Ӳ�ϵ�: eip=%X   [Dr0:%X  1:%X  2:%X  3:%X]",
						  GetCurrentThreadId(), nIndex, m_Dr_Use.Dr0, pCt->Eip, pCt->Dr0, pCt->Dr1, pCt->Dr2, pCt->Dr3 );

			//����
			if(nIndex == 0)
			{
				if(bwillTF == 1)
				{
					debug( "�������� eip=%X", pCt->Eip );
					WaitUserPressRun( pCt );
					return EXCEPTION_CONTINUE_EXECUTION;
				}

				//�Ƿ���Ҫ���������ϵ�
				if(bTJ_inNext == TRUE)
				{
					GETPB( curtjaddr ) = 0xCC;
					bTJ_inNext = FALSE;
					debug( "���������������ϵ�  eip=%X", pCt->Eip );
					return EXCEPTION_CONTINUE_EXECUTION;
				}

				//�Ƿ�����Ӳ��
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

			//Ӳ��
			if(nIndex == 1 || nIndex == 2 || nIndex == 4 || nIndex == 8)
			{
				if(HardPointFilterProc( pCt, nIndex ) == EXCEPTION_CONTINUE_EXECUTION)
					return EXCEPTION_CONTINUE_EXECUTION;
			}

			sys::fsdebug( "WOW!" );
			return EXCEPTION_CONTINUE_SEARCH;
		}

		//�ų�����ǰ�����������߳�
		if(GetCurrentThreadId() == DebugMainThread)
		{
			//����Ҳ������ Isbadreadxx
			//fsdebug("# # # # TID = [%X] EXCEPTION_CONTINUE_SEARCH : EIP->[%X]  pEtrd->ExceptionCode : [%X]# # # #",GetCurrentThreadId(),pCt->Eip,pEtrd->ExceptionCode);
			//MessageBoxA(0,"ERROR!",0,MB_ICONERROR);
			return EXCEPTION_CONTINUE_SEARCH;
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}


#if ANTI_OSAPI_DETECTION
	/************************************************************************/
	/* ��ȡ�ӹ��쳣��ַ
	/************************************************************************/
	DWORD GetPKiUserExceptionDispatcherAddr()
	{
		/*
		(�µ�)�ӹ��쳣��ʽ��  only for Win7x64-Sp1
		Wow64SetupExceptionDispatch������Ŀ�ı��ǽ�32λ�������еĳ���ָ��ɵ�ȫ�ֱ���wow64!Ntdll32KiUserExceptionDispatcher�ɵ�32λNTDLL�е�KiUserExceptionDispatcher��
		00000000`745dc707 8b0d73e80200    mov     ecx,dword ptr [wow64!Ntdll32KiUserExceptionDispatcher (00000000`7460af80)] ds:00000000`7460af80=773c0124				<< ֻ��Ҫ�ĵ����������
		00000000`745dc70d ff151d58ffff    call    qword ptr [wow64!_imp_CpuSetInstructionPointer (00000000`745d1f30)]

		��x86�Ļ���������ģ�
		745dc707 - 8B 0D 73E80200        - mov ecx,[0002E873]

		��Ҫ��������  745dc707 + 5 + 0002E873 + 1  ����  ��Ϊ�ǹ̶���ֵ  ����ֱ��
		wow64!Ntdll32KiUserExceptionDispatcher_addr  =  745dc707 + 2E873 + 6  ;
		*/

		//��ȡ ���������ȡ ��������������
		//����ÿ�������4���ַ��ͺ���  8B0D73E80200 
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
	/* hook KiUserExceptionDispatcher ��������ϵ�
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

			cmp   eax, 1						//EXCEPTION_EXECUTE_HANDLER  ��ԭʼ
			je    _rs
			cmp   eax, 0						//EXCEPTION_CONTINUE_SEARCH  ��ԭʼ
			je    _rs

			//�޸����� ��ȥ������ -1
			mov   ecx, dword ptr[esp + 4]	    // pContext
			push  0
			push  ecx
			call  Ori_ZwContinue			//ZwContinueCall
			retn  8


			_rs:
			jmp   oriKiUserExceptionDispatcherAddr			//����ԭʼ�ĺ�����
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
		// ����ֻ�򵥵Ĳ����£�����ֻ����SEH
		//
		SetUnhandledExceptionFilter( FsExceptionHandler );
		return 1;
	}

#endif
}