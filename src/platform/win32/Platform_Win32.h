/*********************************************************
* Copyright (C) VERTVER, 2019. All rights reserved.
* OMCIDE - Open Micro-controller IDE
* License: GPLv3
**********************************************************
* Module Name: Kernel Win32 
*********************************************************/
#pragma once

#ifdef WINDOWS_PLATFORM
#include <windows.h>
#include <d3d11.h>
#endif

#define SLEEP(x) Sleep(x)
#define MAX_RENDERS 3
