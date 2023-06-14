#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "drv_giveio.h"

// GiveIO
BOOL IODriver_Init(USHORT first, USHORT last)
{
	HANDLE h;

	h = CreateFile("\\\\.\\giveio", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CloseHandle(h);

	return h != INVALID_HANDLE_VALUE;
}
