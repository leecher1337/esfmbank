#ifdef IODRIVER_BUTTIO
#include "iodriver/drv_buttio.h"
#elif defined(IODRIVER_GIVEIO)
#include "iodriver/drv_giveio.h"
#else
#include "iodriver/drv_inpout32.h"
#endif

BOOL IODriver_Init(USHORT first, USHORT last);
