#pragma once

//////////////////////////////////////////////////////////////////////////
//thread process struct

typedef LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

#define NT_SUCCESS(status)          ((NTSTATUS)(status)>=0)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#define STATUS_ACCESS_DENIED        ((NTSTATUS)0xC0000022L)

typedef enum _KWAIT_REASON
{
	Executive,
	FreePage,
	PageIn,
	PoolAllocation,
	DelayExecution,
	Suspended,
	UserRequest,
	WrExecutive,
	WrFreePage,
	WrPageIn,
	WrPoolAllocation,
	WrDelayExecution,
	WrSuspended,
	WrUserRequest,
	WrEventPair,
	WrQueue,
	WrLpcReceive,
	WrLpcReply,
	WrVertualMemory,
	WrPageOut,
	WrRendezvous,
	Spare2,
	Spare3,
	Spare4,
	Spare5,
	Spare6,
	WrKernel
}KWAIT_REASON;

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemBasicInformation, // 0 Y N 
	SystemProcessorInformation, // 1 Y N 
	SystemPerformanceInformation, // 2 Y N 
	SystemTimeOfDayInformation, // 3 Y N 
	SystemNotImplemented1, // 4 Y N 
	SystemProcessesAndThreadsInformation, // 5 Y N 
	SystemCallCounts, // 6 Y N 
	SystemConfigurationInformation, // 7 Y N 
	SystemProcessorTimes, // 8 Y N 
	SystemGlobalFlag, // 9 Y Y 
	SystemNotImplemented2, // 10 Y N 
	SystemModuleInformation, // 11 Y N 
	SystemLockInformation, // 12 Y N 
	SystemNotImplemented3, // 13 Y N 
	SystemNotImplemented4, // 14 Y N 
	SystemNotImplemented5, // 15 Y N 
	SystemHandleInformation, // 16 Y N 
	SystemObjectInformation, // 17 Y N 
	SystemPagefileInformation, // 18 Y N 
	SystemInstructionEmulationCounts, // 19 Y N 
	SystemInvalidInfoClass1, // 20 
	SystemCacheInformation, // 21 Y Y 
	SystemPoolTagInformation, // 22 Y N 
	SystemProcessorStatistics, // 23 Y N 
	SystemDpcInformation, // 24 Y Y 
	SystemNotImplemented6, // 25 Y N 
	SystemLoadImage, // 26 N Y 
	SystemUnloadImage, // 27 N Y 
	SystemTimeAdjustment, // 28 Y Y 
	SystemNotImplemented7, // 29 Y N 
	SystemNotImplemented8, // 30 Y N 
	SystemNotImplemented9, // 31 Y N 
	SystemCrashDumpInformation, // 32 Y N 
	SystemExceptionInformation, // 33 Y N 
	SystemCrashDumpStateInformation, // 34 Y Y/N 
	SystemKernelDebuggerInformation, // 35 Y N 
	SystemContextSwitchInformation, // 36 Y N 
	SystemRegistryQuotaInformation, // 37 Y Y 
	SystemLoadAndCallImage, // 38 N Y 
	SystemPrioritySeparation, // 39 N Y 
	SystemNotImplemented10, // 40 Y N 
	SystemNotImplemented11, // 41 Y N 
	SystemInvalidInfoClass2, // 42 
	SystemInvalidInfoClass3, // 43 
	SystemTimeZoneInformation, // 44 Y N 
	SystemLookasideInformation, // 45 Y N 
	SystemSetTimeSlipEvent, // 46 N Y 
	SystemCreateSession, // 47 N Y 
	SystemDeleteSession, // 48 N Y 
	SystemInvalidInfoClass4, // 49 
	SystemRangeStartInformation, // 50 Y N 
	SystemVerifierInformation, // 51 Y Y 
	SystemAddVerifier, // 52 N Y 
	SystemSessionProcessesInformation // 53 Y N 
}SYSTEM_INFORMATION_CLASS;

typedef enum _THREAD_STATE
{
	StateInitialized,
	StateReady,
	StateRunning,
	StateStandby,
	StateTerminated,
	StateWait,
	StateTransition,
	StateUnknown
}THREAD_STATE;

typedef struct _UNICODE_STRING
{
	USHORT  Length;     //UNICODEռ�õ��ڴ��ֽ���������*2;
	USHORT  MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _VM_COUNTERS
{
	ULONG PeakVirtualSize;                  //����洢��ֵ��С��
	ULONG VirtualSize;                      //����洢��С��
	ULONG PageFaultCount;                   //ҳ������Ŀ��
	ULONG PeakWorkingSetSize;               //��������ֵ��С��
	ULONG WorkingSetSize;                   //��������С��
	ULONG QuotaPeakPagedPoolUsage;          //��ҳ��ʹ������ֵ��
	ULONG QuotaPagedPoolUsage;              //��ҳ��ʹ����
	ULONG QuotaPeakNonPagedPoolUsage;       //�Ƿ�ҳ��ʹ������ֵ��
	ULONG QuotaNonPagedPoolUsage;           //�Ƿ�ҳ��ʹ����
	ULONG PagefileUsage;                    //ҳ�ļ�ʹ�������
	ULONG PeakPagefileUsage;                //ҳ�ļ�ʹ�÷�ֵ��
}VM_COUNTERS, *PVM_COUNTERS;

typedef LONG KPRIORITY;
typedef struct _CLIENT_ID
{
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
}CLIENT_ID;


typedef struct _SYSTEM_THREADS
{
	LARGE_INTEGER KernelTime;               //CPU�ں�ģʽʹ��ʱ�䣻
	LARGE_INTEGER UserTime;                 //CPU�û�ģʽʹ��ʱ�䣻
	LARGE_INTEGER CreateTime;               //�̴߳���ʱ�䣻
	ULONG         WaitTime;                 //�ȴ�ʱ�䣻
	PVOID         StartAddress;             //�߳̿�ʼ�������ַ��
	CLIENT_ID     ClientId;                 //�̱߳�ʶ����
	KPRIORITY     Priority;                 //�߳����ȼ���
	KPRIORITY     BasePriority;             //�������ȼ���
	ULONG         ContextSwitchCount;       //�����л���Ŀ��
	THREAD_STATE  State;                    //��ǰ״̬��
	KWAIT_REASON  WaitReason;               //�ȴ�ԭ��
}SYSTEM_THREADS, *PSYSTEM_THREADS;

typedef struct _SYSTEM_PROCESSES
{
	ULONG          NextEntryDelta;          //���ɽṹ���е�ƫ������
	ULONG          ThreadCount;             //�߳���Ŀ��
	ULONG          Reserved1[6];
	LARGE_INTEGER  CreateTime;              //����ʱ�䣻
	LARGE_INTEGER  UserTime;                //�û�ģʽ(Ring 3)��CPUʱ�䣻
	LARGE_INTEGER  KernelTime;              //�ں�ģʽ(Ring 0)��CPUʱ�䣻
	UNICODE_STRING ProcessName;             //�������ƣ�
	KPRIORITY      BasePriority;            //��������Ȩ��
	ULONG          ProcessId;               //���̱�ʶ����
	ULONG          InheritedFromProcessId;  //�����̵ı�ʶ����
	ULONG          HandleCount;             //�����Ŀ��
	ULONG          Reserved2[2];
	VM_COUNTERS    VmCounters;              //����洢���Ľṹ�����£�
	IO_COUNTERS    IoCounters;              //IO�����ṹ�����£�
	SYSTEM_THREADS Threads[1];              //��������̵߳Ľṹ���飬���£�
}SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;





namespace td
{
	//
	// ����Win32δ�������� ZwQuerySystemInformation
	PVOID ctGetSystemInformation( SYSTEM_INFORMATION_CLASS iSystemInformationClass );

	//
	// ʵ�� GetThreadContext  (Win7x64)
	BOOL ctGetThreadContext( HANDLE hthread, CONTEXT *lpContext );

	//
	// ʵ�� SetThreadContext  (Win7x64)
	BOOL ctSetThreadContext( HANDLE hthread, CONTEXT *lpContext );

	//
	// ����Ӳ���ϵ� ���� ��ַ ���� ����
	//	@param3-dwAttribute:  0��ʾִ�жϵ� 3��ʾ���ʶϵ� 1 ��ʾд��ϵ�
	//  @param4-dwLength:     ȡֵ 1 2 4
	VOID SetAllThreadsDr( DWORD withoutthreadid, DWORD dwaddr, DWORD dwAttribute, DWORD dwLength, DWORD onlythreadid );

	//
	// ɾ�������̵߳�Dr�Ĵ���
	//	@param1: withoutthreadid==������ͣ���߳�id
	//	@param3: notconrtolthread==TRUE��ʱ��ֱ�Ӳ����߳� ��ִ����ͣ->�ָ��Ķ���  (Ĭ��FALSE)
	VOID DelAllThreadsDrs( DWORD withoutthreadid, int del_dr, BOOL notconrtolthread );

	//
	// ��ͣ�����߳� (ʹ��)
	//	@param1 : 1����ͣ�����߳� ��֮��ָ�
	//	@param2 : ����ͣ���߳�id
	VOID SuspendAllThreads( BOOL bSuspendThread, DWORD withoutthreadid );
}