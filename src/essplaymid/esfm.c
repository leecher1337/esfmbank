/******************************************************************

    esfm.c - Functions for communicating with the ES1969 soundcard

    ESFM Bank editor

    Copyright (c) 2023, leecher@dose.0wnz.at  All Rights Reserved.

*******************************************************************/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "iodriver.h"
#include "ess.h"
#include "natv.h"

static PUCHAR m_pSBBase = NULL;

void KeStallExecutionProcessor(DWORD uSecTime) {
    static LONGLONG freq=0;
    LONGLONG start=0, cur=0, wait=0;
    
    if (freq == 0) QueryPerformanceFrequency((PLARGE_INTEGER)&freq);
    if (freq != 0) {
        wait = ((LONGLONG)uSecTime * freq)/(LONGLONG)1000000;
        QueryPerformanceCounter((PLARGE_INTEGER)&start);
        while (cur < (start + wait)) {
            QueryPerformanceCounter((PLARGE_INTEGER)&cur);
        }
    } else {
        //TODO: alternate timing mechanism
    }
}

VOID FAR PASCAL fmwrite (WORD wAddress, BYTE bValue)
{
  WRITE_PORT_UCHAR(m_pSBBase + 2, LOBYTE(wAddress));
  KeStallExecutionProcessor(10);
  WRITE_PORT_UCHAR(m_pSBBase + 3, HIBYTE(wAddress));
  KeStallExecutionProcessor(10);
  WRITE_PORT_UCHAR(m_pSBBase + 1, bValue);
  KeStallExecutionProcessor(10);
}


UCHAR dspReadMixer(UCHAR Address)
{
    WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERADDR, Address);
    return READ_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERDATA);
}

void dspWriteMixer(UCHAR Address, UCHAR Value)
{
    WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERADDR, Address);
    WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_MIXERDATA, Value);
}

void SetDacToMidi(BOOL MidiActive)
{
    if ( MidiActive )
    {
        dspWriteMixer(ESM_MIXER_MDR, dspReadMixer(ESM_MIXER_MDR) & (~0x01));
		dspWriteMixer(ESM_MIXER_FM_VOL, 119);
		dspWriteMixer(ESM_MIXER_DAC_RECVOL, 0);
    }
}

void StartESFM (BOOL MidiActive)
{
    BYTE SerialMode;

    SerialMode = dspReadMixer(ESM_MIXER_SERIALMODE_CTL);
    if ( MidiActive )
        SerialMode &= ~0x10;
    else
        SerialMode |= 0x10;
    dspWriteMixer(ESM_MIXER_SERIALMODE_CTL, SerialMode);
    
    SetDacToMidi(MidiActive);
    
    if ( MidiActive )
    {
        WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_POWER, READ_PORT_UCHAR((PUCHAR)ESSSB_REG_POWER) & (~0x20));
        WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_FMHIGHADDR, 5);
        KeStallExecutionProcessor(25);
        // We want ESFM mode!
        WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_FMLOWADDR + 1, 0x80);
        KeStallExecutionProcessor(25);
    }
    else
    {
        WRITE_PORT_UCHAR(m_pSBBase + ESSSB_REG_POWER, READ_PORT_UCHAR((PUCHAR)ESSSB_REG_POWER) | 0x20);
    }
}

BOOL esfm_init(USHORT SBBase)
{
	m_pSBBase = (PUCHAR)SBBase;
	if (!IODriver_Init(SBBase, SBBase+0xF))
		return FALSE;
    StartESFM(TRUE);
    fmreset();
	return TRUE;
}

void esfm_exit()
{
	StartESFM(FALSE);
	IODriver_Exit();
}
