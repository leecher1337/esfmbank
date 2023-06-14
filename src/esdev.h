/******************************************************************

    esdev.h - Functions to find ES1969 sound card IO port address

    ESFM Bank editor

    Copyright (c) 2023, leecher@dose.0wnz.at  All Rights Reserved.

*******************************************************************/
enum
{
	ESSPortIO = 0,
	ESSPortSB = 1,
	ESSPortVc = 2,
	ESSPortMPU = 3,
	ESSPortGP = 4
};

typedef struct
{
	char szDevName[128];
	USHORT Ports[5];
} ESS_DEVCFG;

DWORD EnumESSDevices(HWND hWnd, void (*EnumCB)(ESS_DEVCFG *pCfg, void *pUser), void *pUser);
