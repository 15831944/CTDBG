//当前模块
#include "stdafx.h"

/************************************************************************/
/* 初始化当前的所有模块
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
FsModuleStruct fsMS[MAX_MODULE_COUNT] = {0};			//一个全局结构

//将所有的module载入到结构体里
int _InitAllMod()
{
	//清空一下内存
	memset( fsMS, 0, sizeof( FsModuleStruct ) * 100 );

	//遍历MODULE
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

			//因为 me32.hModule == me32.modBaseAddr  就不记录了
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

	//排序一下
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
					//把后面的比较小的和前面换下位置
					memcpy( &fsMS[i], &fsMS[i + 1], sizeof( FsModuleStruct ) );
					memcpy( &fsMS[i + 1], &tmpMs, sizeof( FsModuleStruct ) );
				}
			}
		}
	}
	return i_modcount;
}


//显示所有module
VOID FsInitModule()
{
	//上次的显示模式不是反汇编则重新初始化这个
	if(iListShowType != ListType_MOD)
	{
		//////////////////////////////////////////////////////////////////////////
		//把之前的都删掉  删除10次第一列 就是把列都删掉
		for(int i = 0; i < 10; i++)
			ListView_DeleteColumn( ASMlistWnd, 0 );

		//把ListView初始化
		ListView_SetExtendedListViewStyleEx( ASMlistWnd, 0, LVS_EX_FULLROWSELECT );
		LV_COLUMN colmn = {0};
		colmn.mask = LVCF_WIDTH | LVCF_TEXT;

		colmn.pszText = "路径";
		colmn.cx = 400;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "名称";
		colmn.cx = 150;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "入口";
		colmn.cx = 80;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "大小";
		colmn.cx = 80;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );
		colmn.pszText = "基址";
		colmn.cx = 80;
		ListView_InsertColumn( ASMlistWnd, 0, &colmn );

		//HIMAGELIST himage = ImageList_Create(2,16,ILC_COLOR24|ILC_MASK,1,1);
		//ListView_SetImageList(ASMlistWnd,himage,LVSIL_SMALL);
		//////////////////////////////////////////////////////////////////////////
	}

	int i_modcount = _InitAllMod();

	//设定当前的显示模式
	iListShowType = ListType_MOD;
	//刷新一下
	ListView_SetItemCount( ASMlistWnd, i_modcount );
}


/************************************************************************/
/* 传入当前行来修改颜色
/************************************************************************/
//行,列
VOID ChangeModuleColor( int iline, int iSubItem, LPNMLVCUSTOMDRAW lplvcd )
{
	if(strstr( fsMS[iline].path, "syswow64" ) || strstr( fsMS[iline].path, "WOW64" ))
	{
		lplvcd->clrText = RGB( 30, 64, 181 );	//蓝色
	}
	else if(strstr( fsMS[iline].path, "system32" ))
	{
		lplvcd->clrText = RGB( 58, 120, 41 ); //绿色
	}
	else if(strstr( fsMS[iline].path, "exe" ))
	{
		lplvcd->clrText = RGB( 163, 51, 50 ); //RED
	}
}

/************************************************************************/
/* 获取一个函数的地址
/************************************************************************/
DWORD FsGetProcaddressByName( PCH funcName )
{
	DWORD funcaddr = 0;

	//先判断是不是传入的带dll名的函数
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
		//不是的话去每个模块中寻找一次
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
/* 获取一个地址在哪个module 返回一个带有mod名字的地址或0
/************************************************************************/
PCH GetAddressModuleName( DWORD addr )
{
	//在当前所有mod里找
	for(int i = 0; i < MAX_MODULE_COUNT; i++)
	{
		if(addr >= (DWORD)fsMS[i].imageBase  && addr <= (DWORD)(fsMS[i].imageBase + fsMS[i].imageSize))
		{
			return fsMS[i].name;
		}
	}

	//找不到的话去重新init一下所有当前mod  *暂时不弄  一般不会有这种
	//_InitAllMod();
	return NULL;
}

/************************************************************************/
/* 获取一个地址是否一个函数
/************************************************************************/
char retfuncname[260];
#define GetImgDirEntryRVA( pNTHdr, IDE ) (pNTHdr->OptionalHeader.DataDirectory[IDE].VirtualAddress)
PCH GetFunctionName( DWORD addr )
{
	if(addr == 0 || IsBadReadPtr( (PVOID)addr, 4 ) == TRUE)
		return NULL;

	//在当前所有mod里找当前地址的mod
	for(int i = 0; i < MAX_MODULE_COUNT; i++)
	{
		if(addr >= (DWORD)fsMS[i].imageBase  && addr <= (DWORD)(fsMS[i].imageBase + fsMS[i].imageSize))
		{
			//如果找到所在mod  就去遍历导出表 找这个地址上的函数名
			PIMAGE_DOS_HEADER pdosheader = (PIMAGE_DOS_HEADER)fsMS[i].imageBase;
			PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pdosheader + pdosheader->e_lfanew);

			//获取导出表
			DWORD exportsStartRVA = GetImgDirEntryRVA( pNTHeader, IMAGE_DIRECTORY_ENTRY_EXPORT );
			if(!exportsStartRVA)		//是否存在导出表
				return NULL;

			//遍历导出表 看对应的有没有这个地址
			PIMAGE_EXPORT_DIRECTORY pExportDesc = (PIMAGE_EXPORT_DIRECTORY)((DWORD)pdosheader + exportsStartRVA);
			//先使用序号的方式遍历
			DWORD dwRvaFunAddr = pExportDesc->AddressOfFunctions;
			for(int j = 0; j < pExportDesc->NumberOfFunctions; j++)
			{
				//转换一下地址
				DWORD curfunctionRva = GETPD( (DWORD)pdosheader + dwRvaFunAddr + j * 4 );
				//判断是否这个地址
				if(addr == curfunctionRva + (DWORD)pdosheader)
				{
					//判断这个地址的函数是否有名字
					BOOL bHaveName = FALSE;
					int  k = 0;
					//使用名称方式遍历
					for(; k < pExportDesc->NumberOfNames; k++)
					{
						//是否和序号方式遍历出的一样
						WORD dwNameOrd = GETPW( pExportDesc->AddressOfNameOrdinals + (DWORD)pdosheader + 2 * k );
						//debug("j:[%d]  dwNameOrd = %d",j,dwNameOrd);
						if(dwNameOrd == j)
						{
							bHaveName = TRUE;
							break;
						}
					}

					//生成名字
					if(bHaveName)
					{
						//读取名字
						DWORD dwRvaFunNameAddr = GETPD( (DWORD)pdosheader + pExportDesc->AddressOfNames + 4 * k );
						wsprintfA( retfuncname, "%s.%s", fsMS[i].name, (PCH)((DWORD)pdosheader + dwRvaFunNameAddr) );
						//debug("%s",retfuncname);
						return retfuncname;
					}
					else
					{
						//只记录mod名
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
/* 在指定的虚拟ListView中...
/************************************************************************/
//这个listview调用
VOID ListView_GetModule( NMLVDISPINFO* plvdi )
{
	int list_iItem = plvdi->item.iItem;			//行
	int list_isub = plvdi->item.iSubItem;		//行的子项

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
/* 获取module(file)的相关信息
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