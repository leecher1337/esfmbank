// Buttio
#include "buttio.h"

extern IOHandler buttioHand;
__forceinline
VOID
WRITE_PORT_UCHAR (
    USHORT Port,
    UCHAR Value
    )

{
	buttio_wu8(&buttioHand, (Port), (Value));
    return;
}

__forceinline
UCHAR
READ_PORT_UCHAR (
    USHORT Port
    )

{
	BYTE b;
	buttio_ru8(&buttioHand, (Port), &b);
	return b;
}
void IODriver_Exit(void);

#pragma comment(lib, "buttio.lib")
