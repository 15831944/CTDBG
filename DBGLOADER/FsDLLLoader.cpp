//ʵ��dll���ڴ���ע��/���ع���
#include "stdafx.h"

#define TESTSLEPP		0

//##############################################################################################################
// ע����뵽Ŀ����̿ռ� ���ڼ����ҵ�dll
//##############################################################################################################

/************************************************************************/
/* �޸Ľ�����ڵ� ֱ�Ӽ���dll                                                                     
/************************************************************************/
NAKED VOID CopyCode()
{
	/************************************************************************

	0x11111111 װ��ԭʼOEP
	0x11111115 װ��ԭʼOEP���ֽڱ���
	0x22222222 �Ǽ���DLL �� DOSHEAD
	0x33333333 �Ǽ���DLL �� OEP

	0x44444444 ���ַ���"user32.dll"�ĵ�ַ  PUNICODE_STRING
	0x55555555 ��LdrLoadDll�ĵ�ַ   
	0x66666666 �Ƿ��ص�DllHandle��ַ

	************************************************************************/
	
	//load my dll
	_asm
	{
			pushfd
			pushad

			//��ԭ���
			//��VirtualProtect
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

			//��ԭ6byte
			mov  eax,0x11111111		//oepָ��
			mov  eax,[eax]			//eax == oep
			mov  ecx,0x11111115		//װ��ԭʼOEP���ֽڱ���
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

			//�ȼ��ر����ϵͳdll  
			//����OEP��ʼǰ ֻ��kernerl32/kernelbase/ntdll��3��ϵͳdll ��������Ҫ�ֶ�����
			//ֱ��LoadLibraryA ����user32.dll
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

			mov   edx,0x11111111		//����OEP
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

//�滻����ר��call
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

//ע��
VOID InjetCode(HANDLE hprocess,PVOID myDLL_dosHead,DWORD myDLL_OEP)
{
	DWORD pEntryPoint;
	SIZE_T cBytesMoved;
	DWORD dwWr;
	BYTE  oldcode[7];

	//����1000���ڴ�
	DWORD process_my_memroy = (DWORD)VirtualAllocEx(hprocess,NULL,0x1000,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	
	//�ȷ���װ����Щ��ַ���ڴ�
	//MAPADDR		+0 ԭʼOEP		+4 BakCode (byte*7)
	DWORD MAPADDR		=	process_my_memroy;
	ChangeCopyCode(0x11111111,MAPADDR);			//OEP (����LdrpCallInitRoutine)
	ChangeCopyCode(0x11111115,MAPADDR+4);		//OEP-ori_bytes-6byte
	//DOSHEADADDR	+0 DLL��DosHead  +4 DLL��OEP
	DWORD DOSHEADADDR	=	process_my_memroy+0x200;
	ChangeCopyCode(0x22222222,DOSHEADADDR);
	ChangeCopyCode(0x33333333,DOSHEADADDR+4);
	//CODEADDR +0 NewCode
	DWORD CODEADDR		=	process_my_memroy+0x300;
	//SYSDLLADDR
	DWORD SYSDLLADDR	=	process_my_memroy+0x500;
	//д��DLLNAME  [*STR]
	ChangeCopyCode(0x44444444,SYSDLLADDR);			//"user32.dll"
	ChangeCopyCode(0x55555555,(DWORD)GetProcAddress(GetModuleHandleA("kernel32.dll"),"LoadLibraryA"));
	//0x66666666
	ChangeCopyCode(0x66666666,(DWORD)GetProcAddress(GetModuleHandleA("kernelbase.dll"),"VirtualProtect"));

#if TESTSLEPP
	//TEST Sleep & MsgBox
	ChangeCopyCode(0x77777777,SYSDLLADDR+0x200);
	ChangeCopyCode(0x88888888,(DWORD)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwDelayExecution"));
#endif

	//д��oep����ת  push xxxx  - ret
	BYTE INJETCODE[6]	=	{ 0x68,0x11,0x22,0x33,0x44,0xC3 };
	GETPD(INJETCODE+1)	=    CODEADDR;

	//��ʼд������
	//��ȡOEP
	//��ʱ��ZwCreateFile����OEP ��ֹtls�ص���oep�������Լ��
	pEntryPoint = (DWORD)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwCreateFile");	

	//����OEP��ַ�ʹ���
	//MAPADDR OPE��ַ
	WriteProcessMemory(hprocess,(PVOID)MAPADDR,&pEntryPoint,4,&dwWr);
	// +4 ���ݴ��� 7
	ReadProcessMemory(hprocess,(PVOID)pEntryPoint,oldcode,6,&cBytesMoved);
	WriteProcessMemory(hprocess,(PVOID)(MAPADDR+4),oldcode,6,&dwWr);
	//д��DOSHEADADDR
	WriteProcessMemory(hprocess,(PVOID)DOSHEADADDR,&myDLL_dosHead,4,&dwWr);
	// +4 DLL-OEP
	WriteProcessMemory(hprocess,(PVOID)(DOSHEADADDR+4),&myDLL_OEP,4,&dwWr);
	//д��CODE
	DWORD len = (DWORD)CopyCodeEnd-(DWORD)CopyCode;
	//hook_log("CopyCode-END : %X-%X [%d]",CopyCode,CopyCodeEnd,len);
	WriteProcessMemory(hprocess,(PVOID)CODEADDR,CopyCode,len,&dwWr);
	//д��DLL�ַ���
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
/* ע��DLL�����̿ռ䲢����
// ��������ǰ�DLL copy������������ ���ض�λ Ȼ���޸�oep��������
// s1 == Ŀ�����·��   s2 == Ŀ��ProcessHanle   s3 == ��Ҫ���ص�dll
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
	PIMAGE_DOS_HEADER old_dosHeader;		//ԭʼdll��dosheader
	PIMAGE_DOS_HEADER new_dosHeader;		//Ŀ������е�dosheader
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

	// ��ͷ����
	HdrLen = (DWORD)section - (DWORD)old_dosHeader + HeaderSize * pNTHeader->FileHeader.NumberOfSections;

	// �ҳ����Ľڵĳ���,�˽�һ���Ǵ������ڵĽ�(.text ��)
	MaxLen = HdrLen;
	int  ii=0;						//��Դ���ڵڼ�����
	char my_rsrc[6] = ".rsrc";		//��Դ��

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
	//����������ڴ�
	DWORD NeededMemory = ImagePages * sSysInfo.dwPageSize;
	lpImageDll = VirtualAllocEx(ProcessHandle,NULL,NeededMemory, MEM_COMMIT, PAGE_EXECUTE_READWRITE);		//PAGE_READWRITE);  //
	fsdebug("Alloc lpImageDll = %X",lpImageDll);

	//�����ڵ�ǰ������Ҳ����һ����ͬ�Ŀռ� ��ʼ����ͬ�Ľ� �������ز��� (*����)
	LPVOID curp_lpImageDll = VirtualAlloc(NULL,NeededMemory, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

		s_lpImageDllLen = NeededMemory;
	//char buf[200];wsprintfA(buf,"NeededMemory : %X",NeededMemory);MessageBoxA(0,buf,0,0);

	if(lpImageDll == NULL || curp_lpImageDll == NULL ) 
		return 6; // �����ڴ�ʧ��

	//�µ�dosHeader ��Ŀ������еĵ�ַ
	new_dosHeader  = (PIMAGE_DOS_HEADER) lpImageDll;		
	//�ڱ��ص�dosHeader
	PIMAGE_DOS_HEADER curp_dosHeader = (PIMAGE_DOS_HEADER)curp_lpImageDll;

	//////////////////////////////////////////////////////////////////////////
	//д��ͷ������
	DWORD retsize = 0;
	WriteProcessMemory(ProcessHandle,lpImageDll,lpRawDll,HdrLen,&retsize);
	//д��ͷ������
	MoveMemory( curp_lpImageDll, lpRawDll, HdrLen ); 

	DWORD OrgAddr = 0;
	DWORD NewAddr = 0;
	DWORD Size = 0;

	//////////////////////////////////////////////////////////////////////////
	//д�����н�����
	for(i = 0;i<pNTHeader->FileHeader.NumberOfSections;i++)
	{
		OrgAddr = (DWORD)lpImageDll + (DWORD)section[i].VirtualAddress;		//�ڴ���Ӧ�ü��ص�λ�� +VA
		NewAddr = (DWORD)lpRawDll   + (DWORD)section[i].PointerToRawData;	
		Size    = (DWORD)section[i].SizeOfRawData;

		MoveMemory((void *)((DWORD)curp_lpImageDll + (DWORD)section[i].VirtualAddress), (void *)NewAddr, Size );//copy to cur-process
		WriteProcessMemory(ProcessHandle,(PVOID)OrgAddr,(PVOID)NewAddr,Size,&retsize);//copy to process
	}

	//pNTHeader ����Ϊ��ǰ�����ڵĸ���
	pNTHeader      = (PIMAGE_NT_HEADERS) ((DWORD)curp_dosHeader + curp_dosHeader->e_lfanew);
	//section      = (PIMAGE_SECTION_HEADER) ((DWORD)pNTHeader + sizeof(IMAGE_NT_HEADERS));
	pImageBase     = (PBYTE)curp_dosHeader;

	fsdebug("PNTHeader : %X  ImageBase : %X",pNTHeader,curp_dosHeader);

	/*
	//�����ڸ�ɶ�� ��Դ����ɶ��ͬ�� ���˽���
	if((ii!=0) && (IsNT()==TRUE))
	{
		section[ii].VirtualAddress   = section[ii].VirtualAddress   + (DWORD)lpRawDll;
		section[ii].PointerToRawData = section[ii].PointerToRawData + (DWORD)lpRawDll;
	}
	*/
	//////////////////////////////////////////////////////////////////////////
	////�ض�λ�����
	////������Ϊ��Ҫ�������� ����ֱ��ʹ�ñ��ص�dll���м��� ֻд���������ĵĵ�ַ

	//��鵼����Ƿ����
	DWORD importsStartRVA;
	// Look up where the imports section is (normally in the .idata section)
	// but not necessarily so. Therefore, grab the RVA from the data dir.
	importsStartRVA = GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_IMPORT);
	if ( !importsStartRVA ) 
	{
		VirtualFree(curp_lpImageDll,0,MEM_RELEASE);
		return 7;
	}

	//��ȡ������VA  ... �����importsStartRVA�ظ��ˣ�
	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	if(pImportDesc== 0)
	{
		VirtualFree(curp_lpImageDll,0,MEM_RELEASE);
		return 8;
	}

	//��ȡ��ǰ�����
	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) ((DWORD)pImportDesc + (DWORD)curp_dosHeader);

	//�������ڱ��еĵ�ַ
	while(1) 
	{
		// ����Ƿ������˿յ� IMAGE_IMPORT_DESCRIPTOR
		if ((pImportDesc->TimeDateStamp==0 ) && (pImportDesc->Name==0)) 
			break;

		//��ȡ�����dll
		sDllName = (char *) (pImportDesc->Name + (DWORD)pImageBase);
		hDll	 = GetModuleHandleA(sDllName);

		fsdebug("sDllName : %s",sDllName);

		//������
		//if( hDll == 0 ) 
		//hDll = LoadLibrary(sDllName);
		if( hDll == 0 )
		{
			//�����������ԭ����δ���ص�dll����ʾһ��
			MessageBoxA(0,sDllName,"Can't find required Dll",0);
			VirtualFree(curp_lpImageDll,0,MEM_RELEASE);
			return 9;
		}

		DWORD *lpFuncNameRef = (DWORD *) (pImportDesc->OriginalFirstThunk + (DWORD)curp_dosHeader);
		//DWORD *lpFuncAddr	 = (DWORD *) (pImportDesc->FirstThunk + (DWORD)new_dosHeader);				//��������Ҫ�����ĺ���ָ��  new_dosHeader Ŀ������еĵ�ַ
		DWORD lpFuncAddr	 = pImportDesc->FirstThunk + (DWORD)new_dosHeader;		//ֱ�ӵó���Ҫ�޸ĵĵ�ַ �Ժ�+4 �ƶ��Ϳ�����
		DWORD tmpFuncaddr;

		fsdebug("%s ImportDesc::FirstThunk : %X",sDllName,lpFuncAddr);

		//���ﴦ�����ĵ�ַʱ����Ҫ��ȡĿ����̺�����ַ(Ŀ�����ϵͳdll���ص�ַ+����VA)����
		//�������͵�ǰ������ͬ  ��Ϊֻ����kernel32 �� ntdll
		while( *lpFuncNameRef != 0)
		{
			pOrdinalName = (PIMAGE_IMPORT_BY_NAME) (*lpFuncNameRef + (DWORD)curp_dosHeader);
			DWORD pIMAGE_ORDINAL_FLAG = 0x80000000;

			//д�뵽Ŀ����̻�ȡ���ĵ�����еĺ�����ַ  [��д�븱��]
			if(*lpFuncNameRef & pIMAGE_ORDINAL_FLAG)
			{
				tmpFuncaddr = (DWORD) GetProcAddress(hDll, (const char *)(*lpFuncNameRef & 0xFFFF));
				WriteProcessMemory(ProcessHandle,(PVOID)lpFuncAddr,(PVOID)&tmpFuncaddr,4,&retsize);
			}
			else
			{
				tmpFuncaddr = (DWORD) GetProcAddress(hDll, (const char *)pOrdinalName->Name);
				//fsdebug("%s : %X  д���ַ-> %X",(const char *)pOrdinalName->Name,tmpFuncaddr,lpFuncAddr);
				WriteProcessMemory(ProcessHandle,(PVOID)lpFuncAddr,(PVOID)&tmpFuncaddr,4,&retsize);
			}

			lpFuncAddr+=4;			//�ڱ���ÿ�ε���4
			lpFuncNameRef++;
		}

		pImportDesc++;
	}


	//////////////////////////////////////////////////////////////////////////
	//�����ض�λ
	//hook_log("***** ��ʼ�ض�λ *****");
	DWORD TpOffset;
	//��ȡreloc��VA
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
					//��������ÿ����Ҫ�ض�λ�ĵ�ַ  - ���� call [xxxx]  ������������ַ���� [xxxxx] ����
					lpLink  = (PDWORD) ((DWORD)new_dosHeader + baseReloc->VirtualAddress + (*lpTypeOffset & 0xFFF));		
					
					//�ȶ�ȡ���������ַ�ϵ�DWORD
					DWORD get_lpLink = 0;
					ReadProcessMemory(ProcessHandle,(PVOID)lpLink,&get_lpLink,4,&retsize);
					
					//������µĵ�ַ
					//pNTHeader->OptionalHeader.ImageBase PE����ʱ�̶���װ�ص�ַ��VAʲô�Ķ��Ǹ����������
					//dosHeader+��ǰ���ض�λ��ַ�ϵ��ֽ���(DWORD)-�̶�װ�ص�ַ
					//DWORD curRelocDWORD = (DWORD)new_dosHeader + (*lpLink) - pNTHeader->OptionalHeader.ImageBase;
					DWORD curRelocDWORD = (DWORD)new_dosHeader + get_lpLink - pNTHeader->OptionalHeader.ImageBase;

					//hook_log("address : %X   д��  %08X",lpLink,curRelocDWORD);
					//д���µĵ�ַ�����1�δ����ض�λ  [��д�븱��]
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

	//������� �����޸�Ŀ�����OEP/������������DLL-EntryPoint�����ؾͿ�����
	EntryPoint = (LPENTRYPOINT) ((DWORD)pNTHeader->OptionalHeader.AddressOfEntryPoint + (DWORD)new_dosHeader);
	InjetCode(ProcessHandle,new_dosHeader,(DWORD)EntryPoint);

	VirtualFree(curp_lpImageDll,0,MEM_RELEASE);
	return 0;
}



/************************************************************************/
//ֱ��ע��dll��Ŀ���ڴ沢���޸�OEP������  
//injectdllPath�����Ǽ��ܵ� �����ҵķ�ʽ
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
		//hook_log("�Ҳ��� %s",injectdllPath);
		return FALSE;
	}
	//�����ڴ�ӳ���ļ�
	if (!(hMapping=CreateFileMapping(hFile,0,PAGE_READONLY|SEC_COMMIT,0,0,0)))
	{
		CloseHandle(hFile);
		return FALSE;
	}
	//���ļ�ӳ�����pBaseAddr
	if (!(pBaseAddr=MapViewOfFile(hMapping,FILE_MAP_READ,0,0,0)))
	{
		CloseHandle(hMapping);
		CloseHandle(hFile);
		return FALSE;
	}

	//�Ѿ����ص��ڴ��� ֱ��ע��
	int reta = FsMemoryInjectDll(ProcessHandle,pBaseAddr);
	if (0 != reta )
	{
		//hook_log("ERROR INJECT DLL [%d]",reta);
		MessageBoxA(0,"ERORR INJECT DLL!!!",0,MB_ICONSTOP);
	}

	//����ڴ�ӳ��͹ر��ļ�
	UnmapViewOfFile(pBaseAddr);
	CloseHandle(hMapping);
	CloseHandle(hFile);
	return reta;
}