//��ǰģ��
#include "stdafx.h"

/************************************************************************/
/* ��ʼ����ǰ������ģ��
/************************************************************************/
typedef struct _FsModuleStruct
{
	DWORD imageBase;
	DWORD imageSize;
	DWORD DllEp;
	CHAR  name[30];
	CHAR  path[260];
} FsModuleStruct, *PFsModuleStruct;

#define MAX_MODULE_COUNT	200
FsModuleStruct fsMS[MAX_MODULE_COUNT] = {0};			//һ��ȫ�ֽṹ

//�����е�module���뵽�ṹ����
int _InitAllMod()
{
	//���һ���ڴ�
	memset( fsMS, 0, sizeof( FsModuleStruct ) * 100 );

	//����MODULE
	MODULEENTRY32 me32 = {0};
	me32.dwSize = sizeof( MODULEENTRY32 );
	HANDLE hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, 0 );
	if(hModuleSnap == INVALID_HANDLE_VALUE)
		return 0;

	int i_modcount = 0;
	if(Module32First( hModuleSnap, &me32 ))
	{
		do
		{
			lstrcpyA( fsMS[i_modcount].name, me32.szModule );
			lstrcpyA( fsMS[i_modcount].path, me32.szExePath );

			//��Ϊ me32.hModule == me32.modBaseAddr  �Ͳ���¼��
			fsMS[i_modcount].imageBase = (DWORD)me32.modBaseAddr;
			fsMS[i_modcount].imageSize = me32.modBaseSize;
			fsMS[i_modcount].DllEp = GetOEP( (DWORD)me32.hModule );

			i_modcount++;

			//sys::fsdebug("[%d] modname : %s",i_modcount,me32.szModule);
		}
		while(Module32Next( hModuleSnap, &me32 ));
	}

	CloseHandle( hModuleSnap );

	sys::fsdebug( "current module : %d", i_modcount );

	//����һ��
	FsModuleStruct tmpMs = {0};
	for(int j = 0; j < i_modcount; j++)
	{
		for(int i = 0; i < i_modcount - j; i++)
		{
			if(fsMS[i + 1].imageBase)
			{
				if(fsMS[i].imageBase > fsMS[i + 1].imageBase)
				{
					memcpy( &tmpMs, &fsMS[i], sizeof( FsModuleStruct ) );
					//�Ѻ���ıȽ�С�ĺ�ǰ�滻��λ��
					memcpy( &fsMS[i], &fsMS[i + 1], sizeof( FsModuleStruct ) );
					memcpy( &fsMS[i + 1], &tmpMs, sizeof( FsModuleStruct ) );
				}
			}
		}
	}
	return i_modcount;
}


//��ʾ����module
VOID FsInitModule()
{
	//�ϴε���ʾģʽ���Ƿ���������³�ʼ�����
	if(iListShowType != ListType_MOD)
	{
		//////////////////////////////////////////////////////////////////////////
		//��֮ǰ�Ķ�ɾ��  ɾ��10�ε�һ�� ���ǰ��ж�ɾ��
		for(int i = 0; i < 10; i++)
			ListView_DeleteColumn( ASMlistWnd, 0 );

		//��ListView��ʼ��
		ListView_SetExtendedListViewStyleEx( ASMlistWnd, 0, LVS_EX_FULLROWSELECT );
		LV_COLUMN colmn = {0};
		colmn.mask = LVCF_WIDTH | LVCF_TEXT;

		colmn.pszText = "·��";
		colmn.cx = 400;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "����";
		colmn.cx = 150;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "���";
		colmn.cx = 80;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "��С";
		colmn.cx = 80;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "��ַ";
		colmn.cx = 80;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );

		//HIMAGELIST himage = ImageList_Create(2,16,ILC_COLOR24|ILC_MASK,1,1);
		//ListView_SetImageList(ASMlistWnd,himage,LVSIL_SMALL);
		//////////////////////////////////////////////////////////////////////////
	}

	int i_modcount = _InitAllMod();

	//�趨��ǰ����ʾģʽ
	iListShowType = ListType_MOD;
	//ˢ��һ��
	ListView_SetItemCount( ASMlistWnd, i_modcount );
}


/************************************************************************/
/* ���뵱ǰ�����޸���ɫ
/************************************************************************/
//��,��
VOID ChangeModuleColor( int iline, int iSubItem, LPNMLVCUSTOMDRAW lplvcd )
{
	if(strstr( fsMS[iline].path, "syswow64" ) || strstr( fsMS[iline].path, "WOW64" ))
	{
		lplvcd->clrText = RGB( 30, 64, 181 );	//��ɫ
	}
	else if(strstr( fsMS[iline].path, "system32" ))
	{
		lplvcd->clrText = RGB( 58, 120, 41 ); //��ɫ
	}
	else if(strstr( fsMS[iline].path, "exe" ))
	{
		lplvcd->clrText = RGB( 163, 51, 50 ); //RED
	}
}

/************************************************************************/
/* ��ȡһ�������ĵ�ַ
/************************************************************************/
DWORD FsGetProcaddressByName( PCH funcName )
{
	DWORD funcaddr = 0;

	//���ж��ǲ��Ǵ���Ĵ�dll���ĺ���
	PCH tmpfuncname = strstr( funcName, "." );
	if(tmpfuncname)
	{
		tmpfuncname[0] = 0;
		tmpfuncname += 1;

		char tmpdllname[20];
		lstrcpyA( tmpdllname, funcName );
		lstrcatA( tmpdllname, ".dll" );

		debug( "GetProcAddress[dll+func] : %s + %s", tmpdllname, tmpfuncname );
		return (DWORD)GetProcAddress( GetModuleHandleA( tmpdllname ), tmpfuncname );
	}
	else
	{
		//���ǵĻ�ȥÿ��ģ����Ѱ��һ��
		MODULEENTRY32 me32 = {0};
		me32.dwSize = sizeof( MODULEENTRY32 );
		HANDLE hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, 0 );
		if(hModuleSnap == INVALID_HANDLE_VALUE)
			return 0;

		if(Module32First( hModuleSnap, &me32 ))
		{
			do
			{
				funcaddr = (DWORD)GetProcAddress( me32.hModule, funcName );
				if(funcaddr)
				{
					debug( "GetProcAddress[func] : %s + %s", me32.szModule, funcName );
					break;
				}
			}
			while(Module32Next( hModuleSnap, &me32 ));
		}

		CloseHandle( hModuleSnap );
	}

	return funcaddr;
}
/************************************************************************/
/* ��ȡһ����ַ���ĸ�module ����һ������mod���ֵĵ�ַ��0
/************************************************************************/
PCH GetAddressModuleName( DWORD addr )
{
	//�ڵ�ǰ����mod����
	for(int i = 0; i < MAX_MODULE_COUNT; i++)
	{
		if(addr >= (DWORD)fsMS[i].imageBase  && addr <= (DWORD)(fsMS[i].imageBase + fsMS[i].imageSize))
		{
			return fsMS[i].name;
		}
	}

	//�Ҳ����Ļ�ȥ����initһ�����е�ǰmod  *��ʱ��Ū  һ�㲻��������
	//_InitAllMod();
	return NULL;
}

/************************************************************************/
/* ��ȡһ����ַ�Ƿ�һ������
/************************************************************************/
char retfuncname[260];
#define GetImgDirEntryRVA( pNTHdr, IDE ) (pNTHdr->OptionalHeader.DataDirectory[IDE].VirtualAddress)
PCH GetFunctionName( DWORD addr )
{
	if(addr == 0 || IsBadReadPtr( (PVOID)addr, 4 ) == TRUE)
		return NULL;

	//�ڵ�ǰ����mod���ҵ�ǰ��ַ��mod
	for(int i = 0; i < MAX_MODULE_COUNT; i++)
	{
		if(addr >= (DWORD)fsMS[i].imageBase  && addr <= (DWORD)(fsMS[i].imageBase + fsMS[i].imageSize))
		{
			//����ҵ�����mod  ��ȥ���������� �������ַ�ϵĺ�����
			PIMAGE_DOS_HEADER pdosheader = (PIMAGE_DOS_HEADER)fsMS[i].imageBase;
			PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pdosheader + pdosheader->e_lfanew);

			//��ȡ������
			DWORD exportsStartRVA = GetImgDirEntryRVA( pNTHeader, IMAGE_DIRECTORY_ENTRY_EXPORT );
			if(!exportsStartRVA)		//�Ƿ���ڵ�����
				return NULL;

			//���������� ����Ӧ����û�������ַ
			PIMAGE_EXPORT_DIRECTORY pExportDesc = (PIMAGE_EXPORT_DIRECTORY)((DWORD)pdosheader + exportsStartRVA);
			//��ʹ����ŵķ�ʽ����
			DWORD dwRvaFunAddr = pExportDesc->AddressOfFunctions;
			for(int j = 0; j < pExportDesc->NumberOfFunctions; j++)
			{
				//ת��һ�µ�ַ
				DWORD curfunctionRva = GETPD( (DWORD)pdosheader + dwRvaFunAddr + j * 4 );
				//�ж��Ƿ������ַ
				if(addr == curfunctionRva + (DWORD)pdosheader)
				{
					//�ж������ַ�ĺ����Ƿ�������
					BOOL bHaveName = FALSE;
					int  k = 0;
					//ʹ�����Ʒ�ʽ����
					for(; k < pExportDesc->NumberOfNames; k++)
					{
						//�Ƿ����ŷ�ʽ��������һ��
						WORD dwNameOrd = GETPW( pExportDesc->AddressOfNameOrdinals + (DWORD)pdosheader + 2 * k );
						//debug("j:[%d]  dwNameOrd = %d",j,dwNameOrd);
						if(dwNameOrd == j)
						{
							bHaveName = TRUE;
							break;
						}
					}

					//��������
					if(bHaveName)
					{
						//��ȡ����
						DWORD dwRvaFunNameAddr = GETPD( (DWORD)pdosheader + pExportDesc->AddressOfNames + 4 * k );
						wsprintfA( retfuncname, "%s.%s", fsMS[i].name, (PCH)((DWORD)pdosheader + dwRvaFunNameAddr) );
						//debug("%s",retfuncname);
						return retfuncname;
					}
					else
					{
						//ֻ��¼mod��
						wsprintfA( retfuncname, "%s.%x", fsMS[i].name, addr );
						//debug("%s",retfuncname);
						return retfuncname;
					}
				}
			}

		}
	}

	return NULL;
}

/************************************************************************/
/* ��ָ��������ListView��...
/************************************************************************/
//���listview����
VOID ListView_GetModule( NMLVDISPINFO* plvdi )
{
	int list_iItem = plvdi->item.iItem;			//��
	int list_isub = plvdi->item.iSubItem;		//�е�����

	if(fsMS[list_iItem].imageBase)
	{
		switch(list_isub)
		{
			case 0:
				wsprintfA( plvdi->item.pszText, "%08X", fsMS[list_iItem].imageBase );
				break;
			case 1:
				wsprintfA( plvdi->item.pszText, "%08X", fsMS[list_iItem].imageSize );
				break;
			case 2:
				wsprintfA( plvdi->item.pszText, "%08X", fsMS[list_iItem].DllEp );
				break;
			case 3:
				wsprintfA( plvdi->item.pszText, "%s", fsMS[list_iItem].name );
				break;
			case 4:
				wsprintfA( plvdi->item.pszText, "%s", fsMS[list_iItem].path );
				break;
		}
	}
}


/************************************************************************/
/* ��ȡmodule(file)�������Ϣ
/************************************************************************/
#define OEP        40
#define BaseOfCode 44
#define LenOfCode  28

DWORD GetModCodeBase( DWORD ModBase )
{
	IMAGE_DOS_HEADER * dos_head = (IMAGE_DOS_HEADER *)ModBase;
	int nEntryPos = dos_head->e_lfanew + BaseOfCode;
	DWORD dwBaseCode = *(PDWORD)(ModBase + nEntryPos);
	dwBaseCode += ModBase;

	return dwBaseCode;
}

DWORD GetModCodeLen( DWORD ModBase )
{
	IMAGE_DOS_HEADER * dos_head = (IMAGE_DOS_HEADER *)ModBase;
	int nEntryPos = dos_head->e_lfanew + LenOfCode;
	DWORD dwBaseCodeLen = *(PLONG)(ModBase + nEntryPos);

	return dwBaseCodeLen;
}

DWORD GetOEP( DWORD ModBase )
{
	IMAGE_DOS_HEADER * dos_head = (IMAGE_DOS_HEADER *)ModBase;
	int nEntryPos = dos_head->e_lfanew + OEP;
	DWORD dwOEP = *(PLONG)(ModBase + nEntryPos);
	dwOEP += ModBase;

	return dwOEP;
}