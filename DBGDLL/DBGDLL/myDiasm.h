#pragma once



BOOL FsAsmWriteAddresss( PCH cmd, OUT PCH errtext );

VOID ListView_GetDisasm( NMLVDISPINFO* plvdi );

VOID ListJmpToChoiceAsmCode();

VOID ChangeDisasmColor( int iline, int iSubItem, LPNMLVCUSTOMDRAW lplvcd );
BOOL FsDisasm( PCH bufaddr );
BOOL FsDisasm( DWORD addr );
BOOL FsDisasmLastAddr();

VOID GotoHexEdit( DWORD addr );