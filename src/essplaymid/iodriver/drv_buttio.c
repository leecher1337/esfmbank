#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "drv_buttio.h"

IOHandler buttioHand = {0};


BOOL IODriver_Init(USHORT first, USHORT last)
{
	if (!buttio_init(&buttioHand, NULL, BUTTIO_MET_IOPM))
		return FALSE;
	iopm_fillRange(buttioHand.iopm, first, last, TRUE);
	buttio_flushIOPMChanges(&buttioHand);
	return TRUE;
}

void IODriver_Exit(void)
{
	buttio_shutdown(&buttioHand);
}

