#pragma once

PCH GetAddressModuleName( DWORD addr );
VOID ChangeModuleColor( int iline, int iSubItem, LPNMLVCUSTOMDRAW lplvcd );

DWORD GetModCodeBase( DWORD ModBase );
DWORD GetModCodeLen( DWORD ModBase );
DWORD GetOEP( DWORD ModBase );

DWORD FsGetProcaddressByName( PCH funcName );

VOID ListView_GetModule( NMLVDISPINFO* plvdi );
int _InitAllMod();
VOID FsInitModule();

PCH GetFunctionName( DWORD addr );