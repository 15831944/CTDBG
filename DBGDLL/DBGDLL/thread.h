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
	USHORT  Length;     //UNICODE占用的内存字节数，个数*2;
	USHORT  MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _VM_COUNTERS
{
	ULONG PeakVirtualSize;                  //虚拟存储峰值大小；
	ULONG VirtualSize;                      //虚拟存储大小；
	ULONG PageFaultCount;                   //页故障数目；
	ULONG PeakWorkingSetSize;               //工作集峰值大小；
	ULONG WorkingSetSize;                   //工作集大小；
	ULONG QuotaPeakPagedPoolUsage;          //分页池使用配额峰值；
	ULONG QuotaPagedPoolUsage;              //分页池使用配额；
	ULONG QuotaPeakNonPagedPoolUsage;       //非分页池使用配额峰值；
	ULONG QuotaNonPagedPoolUsage;           //非分页池使用配额；
	ULONG PagefileUsage;                    //页文件使用情况；
	ULONG PeakPagefileUsage;                //页文件使用峰值；
}VM_COUNTERS, *PVM_COUNTERS;

typedef LONG KPRIORITY;
typedef struct _CLIENT_ID
{
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
}CLIENT_ID;


typedef struct _SYSTEM_THREADS
{
	LARGE_INTEGER KernelTime;               //CPU内核模式使用时间；
	LARGE_INTEGER UserTime;                 //CPU用户模式使用时间；
	LARGE_INTEGER CreateTime;               //线程创建时间；
	ULONG         WaitTime;                 //等待时间；
	PVOID         StartAddress;             //线程开始的虚拟地址；
	CLIENT_ID     ClientId;                 //线程标识符；
	KPRIORITY     Priority;                 //线程优先级；
	KPRIORITY     BasePriority;             //基本优先级；
	ULONG         ContextSwitchCount;       //环境切换数目；
	THREAD_STATE  State;                    //当前状态；
	KWAIT_REASON  WaitReason;               //等待原因；
}SYSTEM_THREADS, *PSYSTEM_THREADS;

typedef struct _SYSTEM_PROCESSES
{
	ULONG          NextEntryDelta;          //构成结构序列的偏移量；
	ULONG          ThreadCount;             //线程数目；
	ULONG          Reserved1[6];
	LARGE_INTEGER  CreateTime;              //创建时间；
	LARGE_INTEGER  UserTime;                //用户模式(Ring 3)的CPU时间；
	LARGE_INTEGER  KernelTime;              //内核模式(Ring 0)的CPU时间；
	UNICODE_STRING ProcessName;             //进程名称；
	KPRIORITY      BasePriority;            //进程优先权；
	ULONG          ProcessId;               //进程标识符；
	ULONG          InheritedFromProcessId;  //父进程的标识符；
	ULONG          HandleCount;             //句柄数目；
	ULONG          Reserved2[2];
	VM_COUNTERS    VmCounters;              //虚拟存储器的结构，见下；
	IO_COUNTERS    IoCounters;              //IO计数结构，见下；
	SYSTEM_THREADS Threads[1];              //进程相关线程的结构数组，见下；
}SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;





namespace td
{
	//
	// 调用Win32未导出函数 ZwQuerySystemInformation
	PVOID ctGetSystemInformation( SYSTEM_INFORMATION_CLASS iSystemInformationClass );

	//
	// 实现 GetThreadContext  (Win7x64)
	BOOL ctGetThreadContext( HANDLE hthread, CONTEXT *lpContext );

	//
	// 实现 SetThreadContext  (Win7x64)
	BOOL ctSetThreadContext( HANDLE hthread, CONTEXT *lpContext );

	//
	// 设置硬件断点 参数 地址 属性 长度
	//	@param3-dwAttribute:  0表示执行断点 3表示访问断点 1 表示写入断点
	//  @param4-dwLength:     取值 1 2 4
	VOID SetAllThreadsDr( DWORD withoutthreadid, DWORD dwaddr, DWORD dwAttribute, DWORD dwLength, DWORD onlythreadid );

	//
	// 删除所有线程的Dr寄存器
	//	@param1: withoutthreadid==不被暂停的线程id
	//	@param3: notconrtolthread==TRUE的时候直接操作线程 不执行暂停->恢复的动作  (默认FALSE)
	VOID DelAllThreadsDrs( DWORD withoutthreadid, int del_dr, BOOL notconrtolthread );

	//
	// 暂停所有线程 (使用)
	//	@param1 : 1则暂停所有线程 反之则恢复
	//	@param2 : 不暂停的线程id
	VOID SuspendAllThreads( BOOL bSuspendThread, DWORD withoutthreadid );
}