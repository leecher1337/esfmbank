/******************************************************************

    esdev.c - Functions to find ES1969 sound card IO port address

    ESFM Bank editor

    Copyright (c) 2023, leecher@dose.0wnz.at  All Rights Reserved.

*******************************************************************/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include "esdev.h"

#ifdef _MSC_VER
#pragma comment(lib, "Setupapi.lib")
#endif

// ks.h
#define KSCATEGORY_RENDER {0x65E8773EL, 0x8F56, 0x11D0, 0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}


DWORD EnumESSDevices(HWND hWnd, void (*EnumCB)(ESS_DEVCFG *pCfg, void *pUser), void *pUser)
{
#ifdef CM_Free_Log_Conf_Handle 
	GUID render = KSCATEGORY_RENDER;
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA infoData={0};
	LOG_CONF  hFirstLogConf;
	CONFIGRET rcCm;
	ESS_DEVCFG devCfg;
	DWORD i, n, ret = 0;
	
	hDevInfo = SetupDiGetClassDevsW(&render, NULL, hWnd, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE) return 0;

	infoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (n = 0; SetupDiEnumDeviceInfo(hDevInfo, n, &infoData); ++n)
	{
		if (SetupDiGetDeviceRegistryProperty(hDevInfo, &infoData, SPDRP_DEVICEDESC, NULL, devCfg.szDevName, sizeof(devCfg.szDevName), NULL))
		{
			rcCm = CM_Get_First_Log_Conf(&hFirstLogConf, infoData.DevInst, ALLOC_LOG_CONF);
			if (rcCm != CR_SUCCESS)
				rcCm = CM_Get_First_Log_Conf(&hFirstLogConf, infoData.DevInst, BOOT_LOG_CONF);
			if (rcCm == CR_SUCCESS)
			{
				/* Get the first resource descriptor handle. */
				LOG_CONF hCurLogConf = 0;
				rcCm = CM_Get_Next_Res_Des(&hCurLogConf, hFirstLogConf, ResType_IO, 0, 0);
				if (rcCm == CR_SUCCESS)
				{
					for (i=0;i<sizeof(devCfg.Ports)/sizeof(devCfg.Ports[0]);i++)
					{
						ULONG cbData;
						IO_DES *pIoDesc;
						RES_DES hFreeResDesc;

						rcCm = CM_Get_Res_Des_Data_Size(&cbData, hCurLogConf, 0);
						if (rcCm != CR_SUCCESS)
							cbData = 0;
						cbData = max(cbData, sizeof(IO_DES));
						pIoDesc = (IO_DES *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbData);
						if (pIoDesc)
						{
							rcCm = CM_Get_Res_Des_Data(hCurLogConf, pIoDesc, cbData, 0L);
							if (rcCm == CR_SUCCESS)
							{
								/*
								printf("drvHostParallelGetWinHostIoPortsSub: Count=%d Type=%x Base=%I64x End=%I64x Flags=%x\n",
										pIoDesc->IOD_Count, pIoDesc->IOD_Type, pIoDesc->IOD_Alloc_Base,
										pIoDesc->IOD_Alloc_End,  pIoDesc->IOD_DesFlags);*/
								devCfg.Ports[i] = (USHORT)pIoDesc->IOD_Alloc_Base;
							}
							HeapFree(GetProcessHeap(), 0, pIoDesc);
						}

						/* Next */
						hFreeResDesc = hCurLogConf;
						rcCm = CM_Get_Next_Res_Des(&hCurLogConf, hCurLogConf, ResType_IO, 0, 0);
						CM_Free_Res_Des_Handle(hFreeResDesc);
						if (rcCm != CR_SUCCESS)
							break;
					}
					if (i == sizeof(devCfg.Ports)/sizeof(devCfg.Ports[0])-1)
					{
						EnumCB(&devCfg, pUser);
						ret++;
					}
				}
				CM_Free_Log_Conf_Handle(hFirstLogConf);
			}
		}
	}
	return ret;
#else
	return 0;
#endif
}

