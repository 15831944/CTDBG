//实现dll的内存自注入/加载功能
#include "stdafx.h"

#define TESTSLEPP		0

//##############################################################################################################
// 注入代码到目标进程空间 用于加载我的dll
//##############################################################################################################

/************************************************************************/
/* 修改进程入口点 直接加载dll                                                                     
/************************************************************************/
NAKED VOID CopyCode()
{
	/************************************************************************

	0x11111111 装载原始OEP
	0x11111115 装载原始OEP的字节备份
	0x22222222 是加载DLL 的 DOSHEAD
	0x33333333 是加载DLL 的 OEP

	0x44444444 是字符串"user32.dll"的地址  PUNICODE_STRING
	0x55555555 是LdrLoadDll的地址   
	0x66666666 是返回的DllHandle地址

	************************************************************************/
	
	//load my dll
	_asm
	{
			pushfd
			pushad

			//还原入口
			//先VirtualProtect
			mov  eax,0x11111111
			add  eax,0x30
			push eax
			push 0x40
			push 10
			mov  eax,0x11111111
			mov  eax,[eax]
			push eax
			mov  eax,0x66666666
			call eax

			//还原6byte
			mov  eax,0x11111111		//oep指针
			mov  eax,[eax]			//eax == oep
			mov  ecx,0x11111115		//装载原始OEP的字节备份
			mov  edx,[ecx]
			mov  dword ptr[eax],edx
			add  ecx,4
			add  eax,4
			mov  edx,[ecx]
			mov  word ptr[eax],dx

#if TESTSLEPP
			push 0x77777777				//buff +0 == 0xEE1E5D00  +4 == -1   (Sleep x 30s)
			push 0
			mov  eax,0x88888888			//ZwDelayExecution   
			call eax
#endif
			
			mov ecx,0x11111111
			add ecx,0x10
			mov eax,[ecx]
			cmp eax,0
			jne _jmpback

			mov [ecx],1

			//先加载必须的系统dll  
			//进程OEP开始前 只有kernerl32/kernelbase/ntdll这3个系统dll 其他的需要手动加载
			//直接LoadLibraryA 加载user32.dll
			push 0x44444444
			mov  eax,0x55555555
			call eax

			//call mydll oep
			push 0
			push DLL_PROCESS_ATTACH
			mov  eax,0x22222222			//dos head
			mov  eax,[eax]
			push eax				
			mov  eax,0x33333333			//MY-DLL EP
			mov  eax,[eax]
			call eax					//call my-dll ep
			
_jmpback:
			popad
			popfd

			mov   eax, 0x52
			xor   ecx, ecx

			mov   edx,0x11111111		//跳回OEP
			mov   edx,[edx]
			add   edx,7
			jmp   edx
	}
}

NAKED VOID CopyCodeEnd()
{
	_asm
	{
		xor eax,eax
		ret
	}
}

//替换代码专用call
VOID ChangeCopyCode(DWORD ori_code,DWORD nowcode)
{
	int codelen  = 0x100;
	DWORD op;
	VirtualProtect((PVOID)CopyCode,codelen,0x40,&op);

	for(int i=0;i<codelen;i++)
	{
		if( GETPD((DWORD)CopyCode+i) == ori_code )
			GETPD((DWORD)CopyCode+i) = nowcode;
	}
}

//注入
VOID InjetCode(HANDLE hprocess,PVOID myDLL_dosHead,DWORD myDLL_OEP)
{
	DWORD pEntryPoint;
	SIZE_T cBytesMoved;
	DWORD dwWr;
	BYTE  oldcode[7];

	//申请1000的内存
	DWORD process_my_memroy = (DWORD)VirtualAllocEx(hprocess,NULL,0x1000,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	
	//先分配装载那些地址的内存
	//MAPADDR		+0 原始OEP		+4 BakCode (byte*7)
	DWORD MAPADDR		=	process_my_memroy;
	ChangeCopyCode(0x11111111,MAPADDR);			//OEP (用作LdrpCallInitRoutine)
	ChangeCopyCode(0x11111115,MAPADDR+4);		//OEP-ori_bytes-6byte
	//DOSHEADADDR	+0 DLL的DosHead  +4 DLL的OEP
	DWORD DOSHEADADDR	=	process_my_memroy+0x200;
	ChangeCopyCode(0x22222222,DOSHEADADDR);
	ChangeCopyCode(0x33333333,DOSHEADADDR+4);
	//CODEADDR +0 NewCode
	DWORD CODEADDR		=	process_my_memroy+0x300;
	//SYSDLLADDR
	DWORD SYSDLLADDR	=	process_my_memroy+0x500;
	//写入DLLNAME  [*STR]
	ChangeCopyCode(0x44444444,SYSDLLADDR);			//"user32.dll"
	ChangeCopyCode(0x55555555,(DWORD)GetProcAddress(GetModuleHandleA("kernel32.dll"),"LoadLibraryA"));
	//0x66666666
	ChangeCopyCode(0x66666666,(DWORD)GetProcAddress(GetModuleHandleA("kernelbase.dll"),"VirtualProtect"));

#if TESTSLEPP
	//TEST Sleep & MsgBox
	ChangeCopyCode(0x77777777,SYSDLLADDR+0x200);
	ChangeCopyCode(0x88888888,(DWORD)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwDelayExecution"));
#endif

	//写入oep的跳转  push xxxx  - ret
	BYTE INJETCODE[6]	=	{ 0x68,0x11,0x22,0x33,0x44,0xC3 };
	GETPD(INJETCODE+1)	=    CODEADDR;

	//开始写入数据
	//获取OEP
	//暂时用ZwCreateFile代替OEP 防止tls回调对oep有完整性检测
	pEntryPoint = (DWORD)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwCreateFile");	

	//保存OEP地址和代码
	//MAPADDR OPE地址
	WriteProcessMemory(hprocess,(PVOID)MAPADDR,&pEntryPoint,4,&dwWr);
	// +4 备份代码 7
	ReadProcessMemory(hprocess,(PVOID)pEntryPoint,oldcode,6,&cBytesMoved);
	WriteProcessMemory(hprocess,(PVOID)(MAPADDR+4),oldcode,6,&dwWr);
	//写入DOSHEADADDR
	WriteProcessMemory(hprocess,(PVOID)DOSHEADADDR,&myDLL_dosHead,4,&dwWr);
	// +4 DLL-OEP
	WriteProcessMemory(hprocess,(PVOID)(DOSHEADADDR+4),&myDLL_OEP,4,&dwWr);
	//写入CODE
	DWORD len = (DWORD)CopyCodeEnd-(DWORD)CopyCode;
	//hook_log("CopyCode-END : %X-%X [%d]",CopyCode,CopyCodeEnd,len);
	WriteProcessMemory(hprocess,(PVOID)CODEADDR,CopyCode,len,&dwWr);
	//写入DLL字符串
	char user32_sstr[0x50] = "c:\\windows\\system32\\user32.dll";
	WriteProcessMemory(hprocess,(PVOID)(SYSDLLADDR),user32_sstr,0x50,&dwWr);

#if TESTSLEPP
	BYTE bytebuff[10] ={0};
	GETPD(bytebuff+0) = 0xEE1E5D00;
	GETPD(bytebuff+4) = 0xFFFFFFFF;
	WriteProcessMemory(hprocess,(PVOID)(SYSDLLADDR+0x200),bytebuff,8,&dwWr);
#endif

	//hook oep!
	WriteProcessMemory(hprocess,(PVOID)pEntryPoint,INJETCODE,6,&dwWr);
	fsdebug("LOADER CODEADDR : %X  pEntryPoint: %X",CODEADDR,pEntryPoint);
	//MessageBoxA(0,0,0,0);
}


/************************************************************************/
/* 注入DLL到进程空间并加载
// 这个函数是把DLL copy到其他进程内 并重定位 然后修改oep让他运行
// s1 == 目标程序路径   s2 == 目标ProcessHanle   s3 == 需要加载的dll
/************************************************************************/
typedef char * (CALLBACK* LPFNDLLFUNC1)();
typedef UINT (CALLBACK * LPENTRYPOINT) (HANDLE hInstance, DWORD Reason, LPVOID Reserved);

#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD)(ptr)+(DWORD)(addValue))
#define GetImgDirEntryRVA( pNTHdr, IDE ) (pNTHdr->OptionalHeader.DataDirectory[IDE].VirtualAddress)
#define GetImgDirEntrySize( pNTHdr, IDE ) (pNTHdr->OptionalHeader.DataDirectory[IDE].Size)

BOOL IsNT()
{
	DWORD dwWinVer;
	dwWinVer = GetVersion();
	dwWinVer = dwWinVer >> 31;
	return (dwWinVer == 0 ? TRUE : FALSE);
}

LPVOID s_lpImageDll = 0;
int    s_lpImageDllLen = 0;

DWORD FsMemoryInjectDll(HANDLE ProcessHandle,LPVOID lpRawDll)
{
	LPENTRYPOINT EntryPoint; // Function pointer
	PBYTE pImageBase;
	LPVOID lpImageDll;
	SYSTEM_INFO sSysInfo;
	PIMAGE_DOS_HEADER old_dosHeader;		//原始dll的dosheader
	PIMAGE_DOS_HEADER new_dosHeader;		//目标进程中的dosheader
	PIMAGE_NT_HEADERS pNTHeader;
	PIMAGE_SECTION_HEADER section;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
	PIMAGE_IMPORT_BY_NAME pOrdinalName;
	PIMAGE_BASE_RELOCATION baseReloc;
	PDWORD lpLink;
	HINSTANCE hDll;
	WORD i;
	DWORD ImagePages,MaxLen,HdrLen;
	char * sDllName;

	if(NULL == lpRawDll) return 1 ;

	old_dosHeader = (PIMAGE_DOS_HEADER)lpRawDll;
	//hook_log("old_dosHeader(lpRawDll) : %X",old_dosHeader);

	// Is this the MZ header?
	if ((TRUE == IsBadReadPtr(old_dosHeader,sizeof (IMAGE_DOS_HEADER))) || (IMAGE_DOS_SIGNATURE != old_dosHeader->e_magic)) 
		return 2;

	// Get the PE header.
	pNTHeader = MakePtr(PIMAGE_NT_HEADERS,old_dosHeader,old_dosHeader->e_lfanew);

	// Is this a real PE image?
	if((TRUE == IsBadReadPtr(pNTHeader,sizeof ( IMAGE_NT_HEADERS))) || ( IMAGE_NT_SIGNATURE != pNTHeader->Signature))
		return 3 ;

	if(( pNTHeader->FileHeader.SizeOfOptionalHeader != sizeof(pNTHeader->OptionalHeader)) ||
		(pNTHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC))
		return 4;

	if (pNTHeader->FileHeader.NumberOfSections < 1) return 5;

	section = IMAGE_FIRST_SECTION( pNTHeader );
	int HeaderSize = sizeof(IMAGE_SECTION_HEADER);

	// 节头长度
	HdrLen = (DWORD)section - (DWORD)old_dosHeader + HeaderSize * pNTHeader->FileHeader.NumberOfSections;

	// 找出最大的节的长度,此节一般是代码所在的节(.text 节)
	MaxLen = HdrLen;
	int  ii=0;						//资源节在第几个节
	char my_rsrc[6] = ".rsrc";		//资源节

	for(i = 0;i<(DWORD)pNTHeader->FileHeader.NumberOfSections;i++)// find MaxLen
	{
		if(MaxLen < section[i].VirtualAddress + section[i].SizeOfRawData)
		{
			MaxLen = section[i].VirtualAddress + section[i].SizeOfRawData;
		}

		if(strcmp((const char *)section[i].Name,my_rsrc) == 0) 
			ii=i;
	}

	GetSystemInfo(&sSysInfo);
	ImagePages = MaxLen / sSysInfo.dwPageSize;
	if (MaxLen % sSysInfo.dwPageSize) ImagePages++;

	//////////////////////////////////////////////////////////////////////////
	//分配所需的内存
	DWORD NeededMemory = ImagePages * sSysInfo.dwPageSize;
	lpImageDll = VirtualAllocEx(ProcessHandle,NULL,NeededMemory, MEM_COMMIT, PAGE_EXECUTE_READWRITE);		//PAGE_READWRITE);  //
	fsdebug("Alloc lpImageDll = %X",lpImageDll);

	//这里在当前进程内也申请一个相同的空间 初始化相同的节 用来本地操作 (*副本)
	LPVOID curp_lpImageDll = VirtualAlloc(NULL,NeededMemory, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

		s_lpImageDllLen = NeededMemory;
	//char buf[200];wsprintfA(buf,"NeededMemory : %X",NeededMemory);MessageBoxA(0,buf,0,0);

	if(lpImageDll == NULL || curp_lpImageDll == NULL ) 
		return 6; // 分配内存失败

	//新的dosHeader 在目标进程中的地址
	new_dosHeader  = (PIMAGE_DOS_HEADER) lpImageDll;		
	//在本地的dosHeader
	PIMAGE_DOS_HEADER curp_dosHeader = (PIMAGE_DOS_HEADER)curp_lpImageDll;

	//////////////////////////////////////////////////////////////////////////
	//写入头到进程
	DWORD retsize = 0;
	WriteProcessMemory(ProcessHandle,lpImageDll,lpRawDll,HdrLen,&retsize);
	//写入头到本地
	MoveMemory( curp_lpImageDll, lpRawDll, HdrLen ); 

	DWORD OrgAddr = 0;
	DWORD NewAddr = 0;
	DWORD Size = 0;

	//////////////////////////////////////////////////////////////////////////
	//写入所有节数据
	for(i = 0;i<pNTHeader->FileHeader.NumberOfSections;i++)
	{
		OrgAddr = (DWORD)lpImageDll + (DWORD)section[i].VirtualAddress;		//内存里应该加载的位置 +VA
		NewAddr = (DWORD)lpRawDll   + (DWORD)section[i].PointerToRawData;	
		Size    = (DWORD)section[i].SizeOfRawData;

		MoveMemory((void *)((DWORD)curp_lpImageDll + (DWORD)section[i].VirtualAddress), (void *)NewAddr, Size );//copy to cur-process
		WriteProcessMemory(ProcessHandle,(PVOID)OrgAddr,(PVOID)NewAddr,Size,&retsize);//copy to process
	}

	//pNTHeader 设置为当前进程内的副本
	pNTHeader      = (PIMAGE_NT_HEADERS) ((DWORD)curp_dosHeader + curp_dosHeader->e_lfanew);
	//section      = (PIMAGE_SECTION_HEADER) ((DWORD)pNTHeader + sizeof(IMAGE_NT_HEADERS));
	pImageBase     = (PBYTE)curp_dosHeader;

	fsdebug("PNTHeader : %X  ImageBase : %X",pNTHeader,curp_dosHeader);

	/*
	//这是在干啥？ 资源节有啥不同吗？ 需了解下
	if((ii!=0) && (IsNT()==TRUE))
	{
		section[ii].VirtualAddress   = section[ii].VirtualAddress   + (DWORD)lpRawDll;
		section[ii].PointerToRawData = section[ii].PointerToRawData + (DWORD)lpRawDll;
	}
	*/
	//////////////////////////////////////////////////////////////////////////
	////重定位导入表
	////这里因为需要操作进程 所以直接使用本地的dll进行计算 只写入计算出来的的地址

	//检查导入表是否存在
	DWORD importsStartRVA;
	// Look up where the imports section is (normally in the .idata section)
	// but not necessarily so. Therefore, grab the RVA from the data dir.
	importsStartRVA = GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_IMPORT);
	if ( !importsStartRVA ) 
	{
		VirtualFree(curp_lpImageDll,0,MEM_RELEASE);
		return 7;
	}

	//获取导入表的VA  ... 这个和importsStartRVA重复了！
	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	if(pImportDesc== 0)
	{
		VirtualFree(curp_lpImageDll,0,MEM_RELEASE);
		return 8;
	}

	//获取当前导入表
	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) ((DWORD)pImportDesc + (DWORD)curp_dosHeader);

	//处理各入口表中的地址
	while(1) 
	{
		// 检查是否遇到了空的 IMAGE_IMPORT_DESCRIPTOR
		if ((pImportDesc->TimeDateStamp==0 ) && (pImportDesc->Name==0)) 
			break;

		//获取所需的dll
		sDllName = (char *) (pImportDesc->Name + (DWORD)pImageBase);
		hDll	 = GetModuleHandleA(sDllName);

		fsdebug("sDllName : %s",sDllName);

		//不加载
		//if( hDll == 0 ) 
		//hDll = LoadLibrary(sDllName);
		if( hDll == 0 )
		{
			//这里如果出现原程序未加载的dll就提示一下
			MessageBoxA(0,sDllName,"Can't find required Dll",0);
			VirtualFree(curp_lpImageDll,0,MEM_RELEASE);
			return 9;
		}

		DWORD *lpFuncNameRef = (DWORD *) (pImportDesc->OriginalFirstThunk + (DWORD)curp_dosHeader);
		//DWORD *lpFuncAddr	 = (DWORD *) (pImportDesc->FirstThunk + (DWORD)new_dosHeader);				//这里是需要修正的函数指针  new_dosHeader 目标进程中的地址
		DWORD lpFuncAddr	 = pImportDesc->FirstThunk + (DWORD)new_dosHeader;		//直接得出需要修改的地址 以后+4 移动就可以了
		DWORD tmpFuncaddr;

		fsdebug("%s ImportDesc::FirstThunk : %X",sDllName,lpFuncAddr);

		//这里处理导入表的地址时候需要获取目标进程函数地址(目标进程系统dll加载地址+函数VA)才行
		//这里假设和当前进程相同  因为只用了kernel32 和 ntdll
		while( *lpFuncNameRef != 0)
		{
			pOrdinalName = (PIMAGE_IMPORT_BY_NAME) (*lpFuncNameRef + (DWORD)curp_dosHeader);
			DWORD pIMAGE_ORDINAL_FLAG = 0x80000000;

			//写入到目标进程获取到的导入表中的函数地址  [不写入副本]
			if(*lpFuncNameRef & pIMAGE_ORDINAL_FLAG)
			{
				tmpFuncaddr = (DWORD) GetProcAddress(hDll, (const char *)(*lpFuncNameRef & 0xFFFF));
				WriteProcessMemory(ProcessHandle,(PVOID)lpFuncAddr,(PVOID)&tmpFuncaddr,4,&retsize);
			}
			else
			{
				tmpFuncaddr = (DWORD) GetProcAddress(hDll, (const char *)pOrdinalName->Name);
				//fsdebug("%s : %X  写入地址-> %X",(const char *)pOrdinalName->Name,tmpFuncaddr,lpFuncAddr);
				WriteProcessMemory(ProcessHandle,(PVOID)lpFuncAddr,(PVOID)&tmpFuncaddr,4,&retsize);
			}

			lpFuncAddr+=4;			//在表里每次递增4
			lpFuncNameRef++;
		}

		pImportDesc++;
	}


	//////////////////////////////////////////////////////////////////////////
	//处理重定位
	//hook_log("***** 开始重定位 *****");
	DWORD TpOffset;
	//获取reloc的VA
	baseReloc = (PIMAGE_BASE_RELOCATION) ((DWORD) pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

	if( baseReloc != 0 )
	{
		baseReloc = (PIMAGE_BASE_RELOCATION) ((DWORD)baseReloc + (DWORD)curp_dosHeader);
		while( baseReloc->VirtualAddress != 0 )
		{
			PWORD lpTypeOffset = (PWORD) ((DWORD)baseReloc + sizeof(IMAGE_BASE_RELOCATION));
			while (lpTypeOffset < (PWORD)((DWORD)baseReloc + (DWORD)baseReloc->SizeOfBlock))
			{
				TpOffset = *lpTypeOffset & 0xF000;
				if( TpOffset == 0x3000 )
				{
					//这个计算出每个需要重定位的地址  - 例如 call [xxxx]  计算出的这个地址就是 [xxxxx] 这里
					lpLink  = (PDWORD) ((DWORD)new_dosHeader + baseReloc->VirtualAddress + (*lpTypeOffset & 0xFFF));		
					
					//先读取过来这个地址上的DWORD
					DWORD get_lpLink = 0;
					ReadProcessMemory(ProcessHandle,(PVOID)lpLink,&get_lpLink,4,&retsize);
					
					//计算出新的地址
					//pNTHeader->OptionalHeader.ImageBase PE创建时固定的装载地址，VA什么的都是根据这个计算
					//dosHeader+当前需重定位地址上的字节码(DWORD)-固定装载地址
					//DWORD curRelocDWORD = (DWORD)new_dosHeader + (*lpLink) - pNTHeader->OptionalHeader.ImageBase;
					DWORD curRelocDWORD = (DWORD)new_dosHeader + get_lpLink - pNTHeader->OptionalHeader.ImageBase;

					//hook_log("address : %X   写入  %08X",lpLink,curRelocDWORD);
					//写入新的地址，完成1次代码重定位  [不写入副本]
					//*lpLink = curRelocDWORD;
					WriteProcessMemory(ProcessHandle,(PVOID)lpLink,(PVOID)&curRelocDWORD,4,&retsize);
				}
				else
				{
					if( TpOffset != 0 )
					{
						VirtualFree(curp_lpImageDll,0,MEM_RELEASE);
						return 10;
					}
				}

				lpTypeOffset++;
			}

			baseReloc = (PIMAGE_BASE_RELOCATION) ((DWORD)baseReloc + (DWORD)baseReloc->SizeOfBlock);
		}
	}

	fsdebug("Write Over");

	//加载完成 这里修改目标程序OEP/其他函数跳到DLL-EntryPoint再跳回就可以了
	EntryPoint = (LPENTRYPOINT) ((DWORD)pNTHeader->OptionalHeader.AddressOfEntryPoint + (DWORD)new_dosHeader);
	InjetCode(ProcessHandle,new_dosHeader,(DWORD)EntryPoint);

	VirtualFree(curp_lpImageDll,0,MEM_RELEASE);
	return 0;
}



/************************************************************************/
//直接注入dll到目标内存并且修改OEP运行他  
//injectdllPath必须是加密的 按照我的方式
/************************************************************************/
extern "C" __declspec(dllexport) 
	DWORD WINAPIV FsInjectDll(HANDLE ProcessHandle,PCH injectdllPath)
{
	HANDLE hFile;
	HANDLE hMapping;
	PVOID pBaseAddr;

	fsdebug("FsInjectDll start...");

	if ((hFile=CreateFile(injectdllPath,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,0))==INVALID_HANDLE_VALUE)
	{
		//hook_log("找不到 %s",injectdllPath);
		return FALSE;
	}
	//创建内存映射文件
	if (!(hMapping=CreateFileMapping(hFile,0,PAGE_READONLY|SEC_COMMIT,0,0,0)))
	{
		CloseHandle(hFile);
		return FALSE;
	}
	//把文件映像存入pBaseAddr
	if (!(pBaseAddr=MapViewOfFile(hMapping,FILE_MAP_READ,0,0,0)))
	{
		CloseHandle(hMapping);
		CloseHandle(hFile);
		return FALSE;
	}

	//已经加载到内存中 直接注入
	int reta = FsMemoryInjectDll(ProcessHandle,pBaseAddr);
	if (0 != reta )
	{
		//hook_log("ERROR INJECT DLL [%d]",reta);
		MessageBoxA(0,"ERORR INJECT DLL!!!",0,MB_ICONSTOP);
	}

	//清除内存映射和关闭文件
	UnmapViewOfFile(pBaseAddr);
	CloseHandle(hMapping);
	CloseHandle(hFile);
	return reta;
}