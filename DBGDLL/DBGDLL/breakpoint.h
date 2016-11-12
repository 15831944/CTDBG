#pragma once

#define	TF	0x100
#define STACK_SIZE	100		//(x4) bytes


//dr7���Կ��ƼĴ���
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


//DR0-DR3��ʹ�����
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
	//���ص�ǰ���õĵ��ԼĴ���
	int FindFreeDebugRegister( DWORD setbpaddr );

	//����Ӳ���ϵ� ���� ��ַ ���� ����
	//dwAttribute 0��ʾִ�жϵ� 3��ʾ���ʶϵ� 1 ��ʾд��ϵ�
	//dwLength ȡֵ 1 2 4
	//nIndex   �ڼ���dr�Ĵ���
	VOID SetDr7( PCONTEXT pCt, DWORD dwBpAddress, DWORD dwAttribute, DWORD dwLength, int nIndex );

	//����
	VOID SetUserRun();
	//��������
	VOID SetStepUserRun();

	//���Ӳ��
	//dwAttribute 0��ʾִ�жϵ� 3��ʾ���ʶϵ� 1 ��ʾд��ϵ�
	//dwLength ȡֵ 1 2 4
	void SetHardBP( DWORD dwBpAddress, DWORD dwAttribute, DWORD dwLength, DWORD onlytid );

	//ɾ��Ӳ���ϵ�  s1=�ڼ��� 0-3
	VOID DelHardPoint( int idr );

	// ��ָ��������ListView�л�ȡ�ϵ�
	VOID ListView_GetBp( NMLVDISPINFO* plvdi );

	//�ϵ��Ƿ������ݿ���
	BOOL IsInCCRecord( DWORD addr );

	//��Ӷϵ�������ݿ�
	BOOL AddCCToRecord( DWORD addr, DWORD nextaddr );

	//ɾ���ϵ�
	BOOL DelCC( DWORD addr );

	//���/ɾ���ϵ�
	BOOL SoftBreakPoint();

	// �ȴ�����F9
	VOID WaitUserRun( PCONTEXT pCt );

	//ɾ������
	BOOL DelBreakTj();
	//��������
	BOOL SetBreakTj( HWND TjWnd );

	//
	// ��ʾ��ǰ�ĵ��ö�ջ
	//	@param1 == ��ջ��ʾ��һ����ַ�ĵ�ַ
	VOID ShowStack( DWORD StartAddr );

	// ��ջ����ʾ���list��ѡ�еĵ�ַ
	// ע�⣺ ���list�����item=1��Ϊ��ַ
	VOID ShowStackForListWnd( HWND ListWnd );

	//
	// ��ʾ��ǰ�ļĴ���
	VOID ShowRegister( PCONTEXT pContext );

	//�ȴ��û�������ʾ��ǰ��ջ�Ĵ�����
	VOID WaitUserPressRun( PCONTEXT pCt );

	//�ж��Ƿ��ҵ�Ӳ�� ����80000004�е�dr������ʱ
	BOOL IsMyHardPoint( int nIndex, PCONTEXT pCt );

	BOOL HookKiUserExceptionDispatcher();
}
