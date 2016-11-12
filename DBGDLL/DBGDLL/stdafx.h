// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <time.h>
#include <math.h>
#include <Tlhelp32.h>
#include <Psapi.h>
#pragma  comment(lib,"Psapi.lib")
#include <commctrl.h> 
#pragma  comment(lib,   "comctl32.lib ")					//ImageList_Create必须

// TODO:  在此处引用程序需要的其他头文件
#include "config.h"
#include "os.h"
#include "system.h"
#include "resource.h"

#include "Disasm.h"
#include "dialogs.h"
#include "module.h"