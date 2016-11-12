#include "stdafx.h"
#include "thread.h"
#include "breakpoint.h"

namespace td
{
	//
	// ����Win32δ�������� ZwQuerySystemInformation
	PVOID ctGetSystemInformation( SYSTEM_INFORMATION_CLASS iSystemInformationClass )
	{
		ULONG ulSize = 0x8000;
		ULONG ulRequired;
		LPVOID pvBuffer;
		NTSTATUS Status;
		DWORD ZwQuerySystemInformation = (DWORD)GetProcAddress( GetModuleHandleA( "ntdll.dll" ), "ZwQuerySystemInformation" );
		do
		{
			pvBuffer = HeapAlloc( GetProcessHeap(), 0, ulSize );

			if(!pvBuffer)
				return NULL;

			//Status = ZwQuerySystemInformation(iSystemInformationClass,pvBuffer,ulSize,&ulRequired);
			_asm
			{
				lea eax, ulRequired
				push eax
				push ulSize
				push pvBuffer
				push iSystemInformationClass
				call ZwQuerySystemInformation
				mov  Status, eax
			}

			if(Status == STATUS_INFO_LENGTH_MISMATCH)
			{
				HeapFree( GetProcessHeap(), 0, pvBuffer );
				ulSize *= 2;
			}
		}
		while(Status == STATUS_INFO_LENGTH_MISMATCH);

		if(NT_SUCCESS( Status ))
			return pvBuffer;

		HeapFree( GetProcessHeap(), 0, pvBuffer );
		return NULL;
	}

	//
	// ʵ�� GetThreadContext  (Win7x64)
#if ANTI_OSAPI_DETECTION
	NAKED BOOL WINAPI _ctGetThreadContext( HANDLE hthread, const CONTEXT *lpContext )
	{
		_asm
		{
			push ebp
			mov  ebp, esp

			push lpContext
			push hthread
			call _GsTc
			test eax, eax
			jge  _ret1

			xor eax, eax
			jmp _ee

		_ret1:
			xor eax, eax
			inc eax
		_ee:
			pop  ebp
			retn 8

		_GsTc:
			mov eax, 0xCA
			xor ecx, ecx
			lea edx, dword ptr[esp + 4]
			call dword ptr fs : [0xC0]
			add  esp, 4
			retn 8
		}
	}
	BOOL ctGetThreadContext( HANDLE hthread, CONTEXT *lpContext )
	{
		return _ctGetThreadContext( hthread, lpContext );
	}
#else
	#define ctGetThreadContext GetThreadContext
#endif

	//
	// ʵ�� SetThreadContext  (Win7x64)
#if ANTI_OSAPI_DETECTION
	NAKED BOOL WINAPI _ctSetThreadContext( HANDLE hthread, const CONTEXT *lpContext )
	{
		_asm
		{
			push ebp
			mov  ebp, esp

			push lpContext
			push hthread
			call _ZsTc
			test eax, eax
			jge  _ret1

			xor eax, eax
			jmp _ee
			_ret1 :
			xor eax, eax
			inc eax
		_ee:
			pop  ebp
			retn 8

		_ZsTc:
			mov     eax, 0x150
			xor ecx, ecx
			lea     edx, dword ptr[esp + 4]
			call    dword ptr fs : [0xC0]
			add     esp, 4
			retn    8
		}
	}
	BOOL ctSetThreadContext( HANDLE hthread, CONTEXT *lpContext )
	{
		return _ctSetThreadContext( hthread, lpContext );
	}
#else
	#define ctSetThreadContext SetThreadContext
#endif

	//
	// ��ͣ�����߳� (ʹ��)
	//	@param1 : 1����ͣ�����߳� ��֮��ָ�
	//	@param2 : ����ͣ���߳�id
	VOID SuspendAllThreads( BOOL bSuspendThread, DWORD withoutthreadid )
	{
		PVOID                        buf = NULL;
		PSYSTEM_PROCESSES            h_info = NULL;
		PSYSTEM_THREADS              t_info = NULL;
		ULONG						 tlback = 1;

		buf = ctGetSystemInformation( SystemProcessesAndThreadsInformation );
		if(buf)
		{
			h_info = (PSYSTEM_PROCESSES)buf;

			while(tlback)
			{
				if(h_info->NextEntryDelta == 0)
					tlback = 0;

				if(h_info->ProcessId == GetCurrentProcessId())
				{
					int  countnow = 0;

					t_info = h_info->Threads;

					for(ULONG i = 0; i < h_info->ThreadCount; i++, countnow++, t_info++)
					{
						if(t_info->ClientId.UniqueThread != (HWND)withoutthreadid  && t_info->ClientId.UniqueThread != (HWND)GetCurrentThreadId())
						{
							HANDLE aid = OpenThread( THREAD_ALL_ACCESS, 0, (DWORD)t_info->ClientId.UniqueThread );
							if(aid)
							{
								if(bSuspendThread)
									SuspendThread( aid );
								else
									ResumeThread( aid );

								CloseHandle( aid );
							}
						}
					}
				}

				h_info = (PSYSTEM_PROCESSES)((DWORD)h_info + h_info->NextEntryDelta);
			}

			HeapFree( GetProcessHeap(), 0, buf );
		}
	}

	//
	// ɾ�������̵߳�Dr�Ĵ���
	//	@param1: withoutthreadid==������ͣ���߳�id
	//	@param3: notconrtolthread==TRUE��ʱ��ֱ�Ӳ����߳� ��ִ����ͣ->�ָ��Ķ���  (Ĭ��FALSE)
	VOID DelAllThreadsDrs( DWORD withoutthreadid, int del_dr, BOOL notconrtolthread )
	{
		PVOID                        buf = NULL;
		PSYSTEM_PROCESSES            h_info = NULL;
		PSYSTEM_THREADS              t_info = NULL;
		ULONG						 tlback = 1;

		buf = ctGetSystemInformation( SystemProcessesAndThreadsInformation );
		if(buf)
		{
			h_info = (PSYSTEM_PROCESSES)buf;

			while(tlback)
			{
				if(h_info->NextEntryDelta == 0)
					tlback = 0;

				if(h_info->ProcessId == GetCurrentProcessId())
				{
					int  countnow = 0;

					t_info = h_info->Threads;

					for(ULONG i = 0; i < h_info->ThreadCount; i++, countnow++, t_info++)
					{
						if(t_info->ClientId.UniqueThread != (HWND)withoutthreadid  &&
							t_info->ClientId.UniqueThread != (HWND)GetCurrentThreadId())
						{
							HANDLE aid = OpenThread( THREAD_ALL_ACCESS, 0, (DWORD)t_info->ClientId.UniqueThread );
							if(aid)
							{
								if(notconrtolthread == FALSE)
									SuspendThread( aid );

								//��ȡԭʼ
								CONTEXT ct = {0};
								ct.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
								ctGetThreadContext( aid, &ct );

								//����Dr7
								//��ֵ���Ƕ����DR7�ṹ��,ʡȥλ�Ʋ����ķ���
								PCONTEXT pCt = &ct;
								DR7 tagDr7 = {0};
								tagDr7.dwDr7 = pCt->Dr7;

								switch(del_dr)
								{
									case 0:
										//�жϵ�ַ
										pCt->Dr0 = 0;
										//�ϵ㳤��
										tagDr7.DRFlag.len0 = 0;
										//����
										tagDr7.DRFlag.rw0 = 0;
										//�ֲ��ϵ�
										tagDr7.DRFlag.L0 = 0;
										break;
									case 1:
										pCt->Dr1 = 0;
										tagDr7.DRFlag.len1 = 0;
										tagDr7.DRFlag.rw1 = 0;
										tagDr7.DRFlag.L1 = 0;
										break;
									case 2:
										pCt->Dr2 = 0;
										tagDr7.DRFlag.len2 = 0;
										tagDr7.DRFlag.rw2 = 0;
										tagDr7.DRFlag.L2 = 0;
										break;
									case 3:
										pCt->Dr3 = 0;
										tagDr7.DRFlag.len3 = 0;
										tagDr7.DRFlag.rw3 = 0;
										tagDr7.DRFlag.L3 = 0;
										break;
								}

								//д��ȥ
								ctSetThreadContext( aid, &ct );

								if(notconrtolthread == FALSE)
									ResumeThread( aid );

								CloseHandle( aid );
							}
						}
					}
				}

				h_info = (PSYSTEM_PROCESSES)((DWORD)h_info + h_info->NextEntryDelta);
			}

			HeapFree( GetProcessHeap(), 0, buf );
		}
	}

	//
	// �����̵߳�Dr7 
	// Ӳ���ϵ�
	VOID SetThread( DWORD tid, DWORD dwaddr, DWORD dwAttribute, DWORD dwLength, int iDr )
	{
		HANDLE aid = OpenThread( THREAD_ALL_ACCESS, 0, tid );
		if(aid)
		{
			SuspendThread( aid );

			//��ȡԭʼ
			CONTEXT ct = {0};
			ct.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
			ctGetThreadContext( aid, &ct );
			//����Dr7
			bp::SetDr7( &ct, dwaddr, dwAttribute, dwLength, iDr );
			ctSetThreadContext( aid, &ct );

			ResumeThread( aid );
			CloseHandle( aid );
		}
	}

	//
	// ����Ӳ���ϵ� ���� ��ַ ���� ����
	//	@param3-dwAttribute:  0��ʾִ�жϵ� 3��ʾ���ʶϵ� 1 ��ʾд��ϵ�
	//  @param4-dwLength:     ȡֵ 1 2 4
	VOID SetAllThreadsDr( DWORD withoutthreadid, DWORD dwaddr, DWORD dwAttribute, DWORD dwLength, DWORD onlythreadid )
	{
		PVOID                        buf = NULL;
		PSYSTEM_PROCESSES            h_info = NULL;
		PSYSTEM_THREADS              t_info = NULL;
		ULONG						 tlback = 1;

		int iDr = bp::FindFreeDebugRegister( dwaddr );
		if(iDr == -1)
			return;

		buf = ctGetSystemInformation( SystemProcessesAndThreadsInformation );
		if(buf)
		{
			h_info = (PSYSTEM_PROCESSES)buf;

			while(tlback)
			{
				if(h_info->NextEntryDelta == 0)
					tlback = 0;

				if(h_info->ProcessId == GetCurrentProcessId())
				{
					int  countnow = 0;

					t_info = h_info->Threads;

					for(ULONG i = 0; i < h_info->ThreadCount; i++, countnow++, t_info++)
					{
						if(onlythreadid)
						{
							if(onlythreadid == (DWORD)t_info->ClientId.UniqueThread)
							{
								SetThread( (DWORD)t_info->ClientId.UniqueThread, dwaddr, dwAttribute, dwLength, iDr );
								HeapFree( GetProcessHeap(), 0, buf );
								return;
							}
						}
						else
						{
							if(t_info->ClientId.UniqueThread != (HWND)withoutthreadid  && t_info->ClientId.UniqueThread != (HWND)GetCurrentThreadId())
							{
								SetThread( (DWORD)t_info->ClientId.UniqueThread, dwaddr, dwAttribute, dwLength, iDr );
							}
						}
					}
				}

				h_info = (PSYSTEM_PROCESSES)((DWORD)h_info + h_info->NextEntryDelta);
			}

			HeapFree( GetProcessHeap(), 0, buf );
		}
	}

}