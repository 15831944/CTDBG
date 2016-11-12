#pragma once

#define	TF	0x100
#define STACK_SIZE	100		//(x4) bytes


//dr7调试控制寄存器
typedef union _Tag_DR7
{
	struct __DRFlag
	{
		unsigned int L0 : 1;
		unsigned int G0 : 1;
		unsigned int L1 : 1;
		unsigned int G1 : 1;
		unsigned int L2 : 1;
		unsigned int G2 : 1;
		unsigned int L3 : 1;
		unsigned int G3 : 1;
		unsigned int Le : 1;
		unsigned int Ge : 1;
		unsigned int b : 3;
		unsigned int gd : 1;
		unsigned int a : 2;
		unsigned int rw0 : 2;
		unsigned int len0 : 2;
		unsigned int rw1 : 2;
		unsigned int len1 : 2;
		unsigned int rw2 : 2;
		unsigned int len2 : 2;
		unsigned int rw3 : 2;
		unsigned int len3 : 2;
	} DRFlag;

	DWORD dwDr7;
}DR7;


//DR0-DR3的使用情况
typedef struct _DR_USE
{
	BOOL Dr0;
	DWORD Dr0_addr;
	DWORD Dr0_Attribute;
	DWORD Dr0_Len;

	BOOL Dr1;
	DWORD Dr1_addr;
	DWORD Dr1_Attribute;
	DWORD Dr1_Len;

	BOOL Dr2;
	DWORD Dr2_addr;
	DWORD Dr2_Attribute;
	DWORD Dr2_Len;

	BOOL Dr3;
	DWORD Dr3_addr;
	DWORD Dr3_Attribute;
	DWORD Dr3_Len;

} DR_USE;

extern DWORD last_StackStartAddr;
extern BOOL WaitRun ;

namespace bp
{
	//返回当前可用的调试寄存器
	int FindFreeDebugRegister( DWORD setbpaddr );

	//设置硬件断点 参数 地址 属性 长度
	//dwAttribute 0表示执行断点 3表示访问断点 1 表示写入断点
	//dwLength 取值 1 2 4
	//nIndex   第几个dr寄存器
	VOID SetDr7( PCONTEXT pCt, DWORD dwBpAddress, DWORD dwAttribute, DWORD dwLength, int nIndex );

	//运行
	VOID SetUserRun();
	//单步继续
	VOID SetStepUserRun();

	//添加硬断
	//dwAttribute 0表示执行断点 3表示访问断点 1 表示写入断点
	//dwLength 取值 1 2 4
	void SetHardBP( DWORD dwBpAddress, DWORD dwAttribute, DWORD dwLength, DWORD onlytid );

	//删除硬件断点  s1=第几个 0-3
	VOID DelHardPoint( int idr );

	// 在指定的虚拟ListView中获取断点
	VOID ListView_GetBp( NMLVDISPINFO* plvdi );

	//断点是否在数据库中
	BOOL IsInCCRecord( DWORD addr );

	//添加断点进入数据库
	BOOL AddCCToRecord( DWORD addr, DWORD nextaddr );

	//删除断点
	BOOL DelCC( DWORD addr );

	//添加/删除断点
	BOOL SoftBreakPoint();

	// 等待操作F9
	VOID WaitUserRun( PCONTEXT pCt );

	//删除条件
	BOOL DelBreakTj();
	//设置条件
	BOOL SetBreakTj( HWND TjWnd );

	//
	// 显示当前的调用堆栈
	//	@param1 == 堆栈显示第一个地址的地址
	VOID ShowStack( DWORD StartAddr );

	// 堆栈里显示这个list里选中的地址
	// 注意： 这个list里必须item=1的为地址
	VOID ShowStackForListWnd( HWND ListWnd );

	//
	// 显示当前的寄存器
	VOID ShowRegister( PCONTEXT pContext );

	//等待用户处理并显示当前堆栈寄存器等
	VOID WaitUserPressRun( PCONTEXT pCt );

	//判断是否我的硬断 传入80000004中的dr几断下时
	BOOL IsMyHardPoint( int nIndex, PCONTEXT pCt );

	BOOL HookKiUserExceptionDispatcher();
}
