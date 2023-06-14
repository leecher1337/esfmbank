// inpout 32

VOID	__stdcall Out32(USHORT, UCHAR);
UCHAR	__stdcall Inp32(USHORT);

#define WRITE_PORT_UCHAR(Port, Value) Out32((USHORT)Port, Value)
#define READ_PORT_UCHAR(Port) Inp32((USHORT)Port)

#define IODriver_Exit()

#ifdef _WIN64
#pragma comment(lib, "inpoutx64.lib")
#else
#pragma comment(lib, "inpout32.lib")
#endif

