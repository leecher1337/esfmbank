#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>

// GiveIO
#define READ_PORT_UCHAR(Port)                  _inp ((USHORT)Port)
#define READ_PORT_USHORT(Port)                 _inpw ((USHORT)Port)
#define READ_PORT_ULONG(Port)                  _inpd ((USHORT)Port)
#define WRITE_PORT_UCHAR(Port, Value)          _outp (((USHORT)Port), (Value))
#define WRITE_PORT_USHORT(Port, Value)         _outpw (((USHORT)Port), (Value))
#define WRITE_PORT_ULONG(Port, Value)          _outpd (((USHORT)Port), (Value))

#define IODriver_Exit()

