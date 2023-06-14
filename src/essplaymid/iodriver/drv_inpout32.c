#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "drv_inpout32.h"

BOOL __stdcall IsInpOutDriverOpen();

BOOL IODriver_Init(USHORT first, USHORT last)
{
	return IsInpOutDriverOpen();
}


