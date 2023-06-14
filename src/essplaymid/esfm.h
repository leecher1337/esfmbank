/******************************************************************

    esfm.h - Functions for communicating with the ES1969 soundcard

    ESFM Bank editor

    Copyright (c) 2023, leecher@dose.0wnz.at  All Rights Reserved.

*******************************************************************/

BOOL esfm_init(USHORT SBBase);
void esfm_exit();
VOID NEAR PASCAL MidiMessage (DWORD dwData);

