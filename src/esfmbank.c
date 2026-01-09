/******************************************************************

    esfmbank.c - Main application logic and window handling

    ESFM Bank editor

    Copyright (c) 2023, leecher@dose.0wnz.at  All Rights Reserved.

*******************************************************************/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <shlwapi.h>
#include <stdio.h>
#include "resource.h"
#include "esfmregs.h"
#include "ins_names_data.h"
#include "essplaymid/esfm.h"
#include "essplaymid/natv.h"
#include "esdev.h"

#pragma comment (lib, "comctl32.lib")  // Common controls
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "shlwapi.lib")


#pragma pack(1)
typedef struct
{
	USHORT offs[256];
	PATCHSET patches[256];
} PATCHMEM;
#pragma pack()

HINSTANCE g_hInstance;
HWND g_hMainWnd;

PATCHMEM m_patches={0};
PBYTE gBankMem = (PBYTE)&m_patches;

void TellError (HWND hWnd, char *pszFormat, ...) 
{
	va_list ap;
	char szMsg[1024];
	int iLen;

	va_start(ap, pszFormat);
	iLen = wvsprintf(szMsg, pszFormat, ap); 
	va_end(ap);

	szMsg[iLen++]=':';
	FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
		MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)szMsg+iLen, sizeof(szMsg)-iLen, NULL);
	MessageBox(hWnd, szMsg, "Error", MB_OK | MB_ICONSTOP);
}

// Avoid sprintf, as linking it in bloats code by 30KB!!
static char * _float_to_char(double x, char *p)
{
    char *s = p + 8;
    USHORT decimals;
    int units;
    decimals = (int)(x * 100) % 100;
    units = (int)x;

	*s = 0;
    *--s = (decimals % 10) + '0';
    decimals /= 10;
    *--s = (decimals % 10) + '0';
    *--s = '.';
	if (units == 0) *--s='0';
	else
    while (units > 0) {
        *--s = (units % 10) + '0';
        units /= 10;
    }
	while (s>p) *--s=' ';
    return s;
}

void CalcFreq(HWND hWnd)
{
	BOOL fFP = IsDlgButtonChecked(hWnd, IDC_FP);
	int MULT = GetDlgItemInt(hWnd, IDC_MUL, NULL, FALSE);
	int FNUM = GetDlgItemInt(hWnd, IDC_FNUM, NULL, FALSE);
	int BLOCK = GetDlgItemInt(hWnd, IDC_BLOCK, NULL, FALSE);
	char szFreq[16];

	if (fFP)
	{
		double FREQ = MULT * FNUM * ((double)1.00 / ((int)1<<(20 - BLOCK))) * 49716;

		// BLOAT BLOAT BLOAT:
		//sprintf (szFreq, "%d kHz", FREQ);
		_float_to_char(FREQ, szFreq);
		lstrcat(szFreq, " kHz");
	} else {
		// FIXME: Not supported yet
		*szFreq = 0; 
	}
	SetDlgItemText(hWnd, IDC_KHZ, szFreq);
}

static void ChangeFixedPitch(HWND hWnd, HWND hWndFP)
{
	if (SendMessage(hWndFP, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		SetDlgItemText(hWnd, IDC_FNUMTXT, "Frequency number:");
		SetDlgItemText(hWnd, IDC_BLOCKTXT, "Block:");
		SendDlgItemMessage (hWnd, IDC_FNUM, EM_LIMITTEXT, 4, 0);
		SendDlgItemMessage (hWnd, IDC_BLOCK, EM_LIMITTEXT, 1, 0);
		SendDlgItemMessage (hWnd, IDC_SPINFNUM, UDM_SETRANGE, 0, MAKELONG(1023,0));
		SendDlgItemMessage (hWnd, IDC_SPINBLOCK, UDM_SETRANGE, 0, MAKELONG(7,0));
	}
	else
	{
		SetDlgItemText(hWnd, IDC_FNUMTXT, "Coarse tune:");
		SetDlgItemText(hWnd, IDC_BLOCKTXT, "Fine tune:");
		SendDlgItemMessage (hWnd, IDC_FNUM, EM_LIMITTEXT, 3, 0);
		SendDlgItemMessage (hWnd, IDC_BLOCK, EM_LIMITTEXT, 2, 0);
		SendDlgItemMessage (hWnd, IDC_SPINFNUM, UDM_SETRANGE, 0, MAKELONG(127,0));
		SendDlgItemMessage (hWnd, IDC_SPINBLOCK, UDM_SETRANGE, 0, MAKELONG(63,0));
	}
}

void OpDlgToReg(HWND hWnd, PATCH *pat, DWORD opno)
{
	USHORT FNUM;
	OPREG *op = &pat->o[opno];
	BYTE mask;
	BOOL fFP = IsDlgButtonChecked(hWnd, IDC_FP);

	op->r2.ATTACK = GetDlgItemInt(hWnd, IDC_ATTACK, NULL, FALSE);
	op->r2.DECAY = GetDlgItemInt(hWnd, IDC_DECAY, NULL, FALSE);
	op->r3.SUSTAIN = GetDlgItemInt(hWnd, IDC_SUSTAIN, NULL, FALSE);
	op->r3.RELEASE = GetDlgItemInt(hWnd, IDC_RELEASE, NULL, FALSE);
	op->r1.ATTENUATION = GetDlgItemInt(hWnd, IDC_ATTENUATION, NULL, FALSE);
	op->r0.MULT = GetDlgItemInt(hWnd, IDC_MUL, NULL, FALSE);

	FNUM = GetDlgItemInt(hWnd, IDC_FNUM, NULL, FALSE);
	if (fFP)
	{
		op->r5.FP.BLOCK = GetDlgItemInt(hWnd, IDC_BLOCK, NULL, FALSE);
		op->r4.FP.FNUMlo = FNUM&0xFF;
		op->r5.FP.FNUMhi = FNUM>>8;
	}
	else
	{
		op->r4.NFP.FINETUNE = GetDlgItemInt(hWnd, IDC_BLOCK, NULL, FALSE);
		op->r4.NFP.CTlo = FNUM&3;
		op->r5.NFP.CThi = FNUM>>2;
	}

	op->r7.WAVE = (UCHAR)SendDlgItemMessage(hWnd, IDC_WAVEFORM, CB_GETCURSEL, 0, 0);
	op->r1.KSL = (UCHAR)SendDlgItemMessage(hWnd, IDC_KSL, CB_GETCURSEL, 0, 0);
	op->r7.OU = (UCHAR)SendDlgItemMessage(hWnd, IDC_OUT, CB_GETCURSEL, 0, 0);
	op->r7.NOISE = (UCHAR)SendDlgItemMessage(hWnd, IDC_NOISE, CB_GETCURSEL, 0, 0);
	op->r5.FP.DELAY = (UCHAR)SendDlgItemMessage(hWnd, IDC_DELAY, CB_GETCURSEL, 0, 0);
	op->r6.MOD = (UCHAR)SendDlgItemMessage(hWnd, IDC_MOD, CB_GETCURSEL, 0, 0);
	pat->h3 &= ~(3 << (opno<<1));
	pat->h3 |= SendDlgItemMessage(hWnd, IDC_RELVEL, CB_GETCURSEL, 0, 0) << (opno<<1);

	mask = fFP << (4+opno);
	pat->h0 &= ~mask;
	pat->h0 |= mask;
	op->r0.TRM = IsDlgButtonChecked(hWnd, IDC_TREMOLO);
	op->r0.VIB = IsDlgButtonChecked(hWnd, IDC_VIBRATO);
	op->r6.TRMD = IsDlgButtonChecked(hWnd, IDC_TREMOLOD);
	op->r6.VIBD = IsDlgButtonChecked(hWnd, IDC_VIBRATOD);
	op->r0.EGT = IsDlgButtonChecked(hWnd, IDC_EG);
	op->r0.KSR = IsDlgButtonChecked(hWnd, IDC_KSR);
	op->r6.L = IsDlgButtonChecked(hWnd, IDC_L);
	op->r6.R = IsDlgButtonChecked(hWnd, IDC_R);
}

void RegToFNUM(HWND hWnd, PATCH *pat, DWORD opno, BOOL fFP)
{
	OPREG *op = &pat->o[opno];
	USHORT FNUM;

	if (fFP)
	{
		SetDlgItemInt(hWnd, IDC_BLOCK, op->r5.FP.BLOCK, FALSE);
		FNUM = op->r4.FP.FNUMlo | op->r5.FP.FNUMhi<<8;
	}
	else
	{
		SetDlgItemInt(hWnd, IDC_BLOCK, op->r4.NFP.FINETUNE, FALSE);
		FNUM = op->r5.NFP.CThi<<2 | op->r4.NFP.CTlo;
	}
	SetDlgItemInt(hWnd, IDC_FNUM, FNUM, FALSE);
}

void RegToOpDlg(HWND hWnd, PATCH *pat, DWORD opno)
{
	OPREG *op = &pat->o[opno];
	BOOL fFP = (pat->h0 >> (4+opno))&1;

	SetDlgItemInt(hWnd, IDC_ATTACK, op->r2.ATTACK, FALSE);
	SetDlgItemInt(hWnd, IDC_DECAY, op->r2.DECAY, FALSE);
	SetDlgItemInt(hWnd, IDC_SUSTAIN, op->r3.SUSTAIN, FALSE);
	SetDlgItemInt(hWnd, IDC_RELEASE, op->r3.RELEASE, FALSE);
	SetDlgItemInt(hWnd, IDC_ATTENUATION, op->r1.ATTENUATION, FALSE);
	SetDlgItemInt(hWnd, IDC_MUL, op->r0.MULT, FALSE);

	SendDlgItemMessage(hWnd, IDC_WAVEFORM, CB_SETCURSEL, op->r7.WAVE, 0);
	SendDlgItemMessage(hWnd, IDC_KSL, CB_SETCURSEL, op->r1.KSL, 0);
	SendDlgItemMessage(hWnd, IDC_OUT, CB_SETCURSEL, op->r7.OU, 0);
	SendDlgItemMessage(hWnd, IDC_NOISE, CB_SETCURSEL, op->r7.NOISE, 0);
	SendDlgItemMessage(hWnd, IDC_DELAY, CB_SETCURSEL, op->r5.FP.DELAY, 0);
	SendDlgItemMessage(hWnd, IDC_MOD, CB_SETCURSEL, op->r6.MOD, 0);
	SendDlgItemMessage(hWnd, IDC_RELVEL, CB_SETCURSEL, pat->h3>>(opno<<1) & 3 , 0);
	RegToFNUM(hWnd, pat, opno, fFP);

	CheckDlgButton(hWnd, IDC_FP, fFP);
	ChangeFixedPitch(hWnd, GetDlgItem(hWnd, IDC_FP));
	CheckDlgButton(hWnd, IDC_TREMOLO, op->r0.TRM);
	CheckDlgButton(hWnd, IDC_VIBRATO, op->r0.VIB);
	CheckDlgButton(hWnd, IDC_TREMOLOD, op->r6.TRMD);
	CheckDlgButton(hWnd, IDC_VIBRATOD, op->r6.VIBD);
	CheckDlgButton(hWnd, IDC_EG, op->r0.EGT);
	CheckDlgButton(hWnd, IDC_KSR, op->r0.KSR);
	CheckDlgButton(hWnd, IDC_L, op->r6.L);
	CheckDlgButton(hWnd, IDC_R, op->r6.R);

	CalcFreq(hWnd);
}

BOOL LoadPatchSet(HWND hWnd, char *pszFile)
{
	HANDLE hFile, hMap;
	BYTE *lpMem;
	BOOL bRet = FALSE;
	PUSHORT pTbl;
	PATCHSET *ps;
	DWORD dwSize;
	int i;

    if ((hFile = CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
	{
		dwSize = GetFileSize(hFile, NULL);
		if (dwSize > 512 && (hMap = CreateFileMapping (hFile, NULL, PAGE_READONLY, 0, 0, NULL)))
		{
			if (lpMem = MapViewOfFile (hMap, FILE_MAP_READ, 0, 0, 0))
			{
				pTbl = (PUSHORT)lpMem;
				for (i=0; i<256; i++)
				{
					if (pTbl[i] && pTbl[i] + sizeof(PATCH)<= dwSize)
					{
						ps = (PATCHSET*)&lpMem[pTbl[i]];
						m_patches.offs[i] = 512 + (i*(USHORT)sizeof(m_patches.patches[0]));
						m_patches.patches[i].p[0] = ps->p[0];
						if (((HDR0*)&ps->p[0].h0)->OP > 0 && pTbl[i] + (sizeof(PATCH) * 2) <= dwSize)
							m_patches.patches[i].p[1] = ps->p[1];
					} else 
						m_patches.offs[i] = 0;
				}
				bRet = TRUE;
				UnmapViewOfFile (lpMem);
			}
			else
				TellError(hWnd, "Cannot map view of file %s: ", pszFile);
			CloseHandle(hMap);
		}
		else
			TellError(hWnd, "Cannot map file %s: ", pszFile);
		CloseHandle(hFile);
	}
	else
		TellError(hWnd, "Cannot open file %s: ", pszFile);

	return bRet;
}

BOOL SavePatchSet(HWND hWnd, char *pszFile)
{
	HANDLE hFile;
	int i;
	DWORD dwWritten;
	USHORT offs[256]={0};
	BOOL bRet = FALSE;

    if ((hFile = CreateFile(pszFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL)) != INVALID_HANDLE_VALUE)
	{
		WriteFile(hFile, offs, sizeof(offs), &dwWritten, NULL);
		for (i=0; i<256; i++)
		{
			if (m_patches.offs[i])
			{
				offs[i] = (USHORT)SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
				if (!WriteFile(hFile, &m_patches.patches[i].p[0], sizeof(m_patches.patches[i].p[0]), &dwWritten, NULL))
					break;
				if (((HDR0*)&m_patches.patches[i].p[0].h0)->OP > 0 &&
					!WriteFile(hFile, &m_patches.patches[i].p[1], sizeof(m_patches.patches[i].p[1]), &dwWritten, NULL))
					break;
			}
		}
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		if (!WriteFile(hFile, &offs, sizeof(offs), &dwWritten, NULL)) i=0;
		if (i == 256) bRet = TRUE;
		else TellError(hWnd, "Error writing file %s:");

		CloseHandle(hFile);
	}
	else
		TellError(hWnd, "Cannot open file %s: ", pszFile);

	return bRet;
}

//
// The MIDI listener
//
void CALLBACK midiCB(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if(wMsg == MIM_DATA) MidiMessage((DWORD)dwParam1);
}

HMIDIIN MIDIlstnStart(HWND hWnd, UINT devIndex)
{
	HMIDIIN hmi;
	MIDIINCAPSA caps;
	MMRESULT  res;

    if ((res = midiInGetDevCapsA(devIndex, &caps, sizeof(MIDIINCAPSA))) != MMSYSERR_NOERROR ||
        (res = midiInOpen(&hmi, devIndex, (DWORD_PTR)&midiCB, 0, CALLBACK_FUNCTION)) != MMSYSERR_NOERROR ||
        (res = midiInStart(hmi)) != MMSYSERR_NOERROR)
	{
		char szMsg[128];

		wsprintf (szMsg, "Cannot open MIDI device, failed with error: %d", res);
		MessageBox(hWnd, szMsg, "Error", MB_OK | MB_ICONSTOP);
		return NULL;
	}
	return hmi;
}

void MIDIlstnStop(HMIDIIN hmi)
{
    midiInStop(hmi);
    midiInClose(hmi);
}

void RegToVoiceDlg(HWND hWnd, PATCH *pat)
{
	TCITEM tci={0};
	int i, nItems;
	HWND hWndTab = GetDlgItem(hWnd, IDC_TABCHANNELS);

	CheckDlgButton(hWnd, IDC_PAT16, ((HDR0*)&pat->h0)->PAT16);
	nItems = TabCtrl_GetItemCount(hWndTab);
	tci.mask = TCIF_PARAM;
	for (i=0; i<nItems; i++)
	{
		TabCtrl_GetItem (hWndTab, i, &tci);
		RegToOpDlg((HWND)tci.lParam, pat, i);
	}
}

void Change2ndVoice(HWND hWnd, DWORD OP, BOOL fCheck)
{
	TCITEM tci={0};
	HWND hWndTab = GetDlgItem(hWnd, IDC_TABVOICE);

	if (fCheck)
	{
		CheckDlgButton(hWnd, IDC_2NDVOICE, OP > 0);
		CheckDlgButton(hWnd, IDC_STEAL, OP > 1);
	}
	EnableWindow(GetDlgItem(hWnd, IDC_STEAL), OP > 0);
	tci.mask = TCIF_PARAM;
	TabCtrl_GetItem (hWndTab, 1, &tci);
	EnableWindow((HWND)tci.lParam, OP > 0);
}

void PatchSetToMask(HWND hWnd, PATCHSET *ps)
{
	TCITEM tci={0};
	int i, nItems;
	HWND hWndTab = GetDlgItem(hWnd, IDC_TABVOICE);

	nItems = TabCtrl_GetItemCount(hWndTab);
	tci.mask = TCIF_PARAM;
	for (i=0; i<nItems; i++)
	{
		TabCtrl_GetItem (hWndTab, i, &tci);
		RegToVoiceDlg((HWND)tci.lParam, &ps->p[i]);
	}
	Change2ndVoice(hWnd, ((HDR0*)&ps->p[0].h0)->OP, TRUE);
}

void ApplyMainDlg(HWND hWndMain, PATCHSET *ps, DWORD dwVoice)
{
	HDR0 *h0;

	h0 = (HDR0*)&ps->p[dwVoice].h0;
	if (h0->OP = IsDlgButtonChecked(hWndMain, IDC_2NDVOICE))
		h0->OP += IsDlgButtonChecked(hWndMain, IDC_STEAL);
}

void ApplyVoiceDlg(HWND hWndVoice, PATCHSET *ps, DWORD dwVoice)
{
	((HDR0*)&ps->p[dwVoice].h0)->PAT16 = IsDlgButtonChecked(hWndVoice, IDC_PAT16);
}

void ApplyOp(HWND hWndOp, PATCHSET *ps)
{
	DWORD dwOp, dwVoice;
	HWND hWndVoice = GetParent(GetParent(hWndOp)), hWndMain;
	HWND hWndTab = GetDlgItem(hWndVoice, IDC_TABCHANNELS);

	dwOp = TabCtrl_GetCurSel(hWndTab);

	hWndMain = GetParent(GetParent(hWndVoice));
	hWndTab = GetDlgItem(hWndMain, IDC_TABVOICE);
	dwVoice = TabCtrl_GetCurSel(hWndTab);
	OpDlgToReg(hWndOp, &ps->p[dwVoice], dwOp);
	ApplyVoiceDlg(hWndVoice, ps, dwVoice);
	ApplyMainDlg(hWndMain, ps, dwVoice);
}

void ResetOp(HWND hWndOp, PATCHSET *ps)
{
	DWORD dwOp, dwVoice;
	HWND hWndVoice = GetParent(GetParent(hWndOp));
	HWND hWndTab = GetDlgItem(hWndVoice, IDC_TABCHANNELS), hWndMain;

	dwOp = TabCtrl_GetCurSel(hWndTab);

	hWndMain = GetParent(GetParent(hWndVoice));
	hWndTab = GetDlgItem(hWndMain, IDC_TABVOICE);
	dwVoice = TabCtrl_GetCurSel(hWndTab);
	RegToOpDlg(hWndOp, &ps->p[dwVoice], dwOp);
	Change2ndVoice(hWndOp, ((HDR0*)&ps->p[0].h0)->OP, TRUE);
}

void ResetFNUM(HWND hWndOp, PATCHSET *ps)
{
	DWORD dwOp, dwVoice;
	HWND hWndVoice = GetParent(GetParent(hWndOp));
	HWND hWndTab = GetDlgItem(hWndVoice, IDC_TABCHANNELS), hWndMain;

	dwOp = TabCtrl_GetCurSel(hWndTab);

	hWndMain = GetParent(GetParent(hWndVoice));
	hWndTab = GetDlgItem(hWndMain, IDC_TABVOICE);
	dwVoice = TabCtrl_GetCurSel(hWndTab);
	RegToFNUM(hWndOp, &ps->p[dwVoice], dwOp, IsDlgButtonChecked(hWndOp, IDC_FP));
}


// Load from ntdll if you want to avoid the bloated CRT
#define RtlCompareMemory memcmp

BOOL VoiceChanged(HWND hWndMain, HWND hWndVoice, DWORD dwVoice, PATCHSET *ps, BOOL fApply)
{
	TCITEM tci={0};
	int i, nItems;
	HWND hWndTab = GetDlgItem(hWndVoice, IDC_TABCHANNELS);
	PATCHSET curPs;

	curPs = *ps;
	nItems = TabCtrl_GetItemCount(hWndTab);
	tci.mask = TCIF_PARAM;
	for (i=0; i<nItems; i++)
	{
		TabCtrl_GetItem (hWndTab, i, &tci);
		OpDlgToReg((HWND)tci.lParam, &curPs.p[dwVoice], i);
	}
	ApplyVoiceDlg(hWndVoice, &curPs, dwVoice);
	ApplyMainDlg(hWndMain, &curPs, dwVoice);
	if (RtlCompareMemory(&curPs.p[dwVoice], &ps->p[dwVoice], sizeof(curPs.p[dwVoice])))
	{
		if (fApply) RtlCopyMemory(&ps->p[dwVoice], &curPs.p[dwVoice], sizeof(curPs.p[dwVoice]));
		else return TRUE;
	}

	return FALSE;
}

BOOL PatchsetChanged(HWND hWnd, PATCHSET *ps, BOOL fApply)
{
	TCITEM tci={0};
	int i, nItems;
	HWND hWndTab = GetDlgItem(hWnd, IDC_TABVOICE);
	BOOL bRet = FALSE;

	nItems = TabCtrl_GetItemCount(hWndTab);
	tci.mask = TCIF_PARAM;
	for (i=0; i<nItems; i++)
	{
		TabCtrl_GetItem (hWndTab, i, &tci);
		if (VoiceChanged(hWnd, (HWND)tci.lParam, i, ps, fApply))
			return TRUE;
	}
	return FALSE;
}

void MakeDBScale(HWND hWndCtl)
{
	SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"0  -inf dB");
	SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"1  -36 dB");
	SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"2  -30 dB");
	SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"3  -24 dB");
	SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"4  -18 dB");
	SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"5  -12 dB");
	SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"6  -6 dB");
	SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"7  0 dB");
	SendMessage (hWndCtl, CB_SETCURSEL, 0, 0);
}

int GetCurrentInstrument(HWND hWndRoot)
{
	return (int)SendDlgItemMessage (hWndRoot, IDC_INSTRUMENTS, LB_GETCURSEL, 0, 0) + 
		128 * IsDlgButtonChecked(hWndRoot, IDC_PERCUSSION);
}


LRESULT CALLBACK SplashDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG:
			return TRUE;
	}
	return FALSE;
}


LRESULT CALLBACK OperatorDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG:
		{
			HWND hWndCtl;

			SendDlgItemMessage (hWnd, IDC_SPINATTACK, UDM_SETRANGE, 0, MAKELONG(15,0));
			SendDlgItemMessage (hWnd, IDC_SPINDECAY, UDM_SETRANGE, 0, MAKELONG(15,0));
			SendDlgItemMessage (hWnd, IDC_SPINSUSTAIN, UDM_SETRANGE, 0, MAKELONG(15,0));
			SendDlgItemMessage (hWnd, IDC_SPINRELEASE, UDM_SETRANGE, 0, MAKELONG(15,0));
			SendDlgItemMessage (hWnd, IDC_SPINMUL, UDM_SETRANGE, 0, MAKELONG(15,0));
			SendDlgItemMessage (hWnd, IDC_SPINATTENUATION, UDM_SETRANGE, 0, MAKELONG(63,0));
			SendDlgItemMessage (hWnd, IDC_SPINFNUM, UDM_SETRANGE, 0, MAKELONG(1023,0));
			SendDlgItemMessage (hWnd, IDC_SPINBLOCK, UDM_SETRANGE, 0, MAKELONG(7,0));

			SendDlgItemMessage (hWnd, IDC_ATTACK, EM_LIMITTEXT, 2, 0);
			SendDlgItemMessage (hWnd, IDC_DECAY, EM_LIMITTEXT, 2, 0);
			SendDlgItemMessage (hWnd, IDC_SUSTAIN, EM_LIMITTEXT, 2, 0);
			SendDlgItemMessage (hWnd, IDC_RELEASE, EM_LIMITTEXT, 2, 0);
			SendDlgItemMessage (hWnd, IDC_MUL, EM_LIMITTEXT, 2, 0);
			SendDlgItemMessage (hWnd, IDC_ATTENUATION, EM_LIMITTEXT, 2, 0);
			SendDlgItemMessage (hWnd, IDC_FNUM, EM_LIMITTEXT, 4, 0);
			SendDlgItemMessage (hWnd, IDC_BLOCK, EM_LIMITTEXT, 1, 0);

			hWndCtl = GetDlgItem(hWnd, IDC_WAVEFORM);
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"0 - Sine");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"1 - Half-Sine");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"2 - Absolute Sine");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"3 - Pulse-Sine");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"4 - Sine - even periods only");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"5 - Abs-Sine - even periods only");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"6 - Square");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"7 - Derived Square");
			SendMessage (hWndCtl, CB_SETCURSEL, 0, 0);

			hWndCtl = GetDlgItem(hWnd, IDC_KSL);
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"0  None");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"1  1.5 dB/oct");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"2  3.0 dB/oct");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"3  6.0 dB/oct");
			SendMessage (hWndCtl, CB_SETCURSEL, 0, 0);

			MakeDBScale(GetDlgItem(hWnd, IDC_OUT));
			MakeDBScale(GetDlgItem(hWnd, IDC_MOD));

			hWndCtl = GetDlgItem(hWnd, IDC_RELVEL);
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"0  None");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"1  1/16");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"2  1/8");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"3  1/4");
			SendMessage (hWndCtl, CB_SETCURSEL, 0, 0);

			hWndCtl = GetDlgItem(hWnd, IDC_NOISE);
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"0  Melodic");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"1  Snare drum");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"2  Hi-hat");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"3  Top cymbal");
			SendMessage (hWndCtl, CB_SETCURSEL, 0, 0);

			hWndCtl = GetDlgItem(hWnd, IDC_DELAY);
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"0  0 Samples (0 ms)");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"1  512 Samples (10.3 ms)");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"2  1024 Samples (20.6 ms)");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"3  2048 Samples (41.2 ms)");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"4  4096 Samples (82.4 ms)");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"5  8192 Samples (164.8 ms)");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"6  16384 Samples (329.6 ms)");
			SendMessage (hWndCtl, CB_ADDSTRING, 0, (LPARAM)"7  32768 Samples (659.1 ms)");
			SendMessage (hWndCtl, CB_SETCURSEL, 0, 0);

			return TRUE;
		}
		case WM_COMMAND:
		{
			int iCurSel;

			switch (LOWORD(wParam))
			{
			case IDOK:
			case IDCANCEL:
				if (HIWORD(wParam) == BN_CLICKED)
				{
					HWND hWndRoot = GetAncestor(hWnd, GA_ROOT);
					iCurSel = GetCurrentInstrument(hWndRoot);
					if (iCurSel != LB_ERR)
					{
						if (LOWORD(wParam) == IDOK)
							ApplyOp(hWnd, &m_patches.patches[iCurSel]);
						else
							ResetOp(hWnd, &m_patches.patches[iCurSel]);
					}
				}
				break;
			case IDC_FP:
				if (HIWORD(wParam) == BN_CLICKED)
				{
						HWND hWndRoot = GetAncestor(hWnd, GA_ROOT);
						ChangeFixedPitch(hWnd, (HWND) lParam);
						iCurSel = GetCurrentInstrument(hWndRoot);
						ResetFNUM(hWnd, &m_patches.patches[iCurSel]);
				}
				break;
			case IDC_MUL:
			case IDC_FNUM:
			case IDC_BLOCK:
				if (HIWORD(wParam) == EN_CHANGE)
					CalcFreq(hWnd);
				break;
			}
			break;
		}

	}
	return FALSE;

}

void CreateTabs(HWND hWnd, UINT uDlgItemTabs, UINT uDlgItemPage, DLGPROC lpDialogFunc, char **pszTexts, UINT nTexts)
{
	TCITEM tci={0};
	HWND hWndTabs = GetDlgItem (hWnd, uDlgItemTabs);
	RECT rc;
	UINT i;

	// https://web.archive.org/web/20140325152119/http://support.microsoft.com/kb/149501
	SetWindowLong(hWndTabs, GWL_EXSTYLE, GetWindowLong(hWndTabs, GWL_EXSTYLE) | WS_EX_CONTROLPARENT);
	GetClientRect (hWndTabs, &rc);
	tci.mask=TCIF_TEXT | TCIF_PARAM;
	for (i=0; i<nTexts; i++)
	{
		tci.lParam=(LPARAM)CreateDialog(g_hInstance, MAKEINTRESOURCE(uDlgItemPage), hWndTabs, lpDialogFunc);
		tci.pszText=pszTexts[i];
		TabCtrl_InsertItem(hWndTabs, i, &tci);
		if (i == 0)
		{
			ShowWindow ((HWND)tci.lParam, SW_SHOW);
			TabCtrl_AdjustRect(hWndTabs, FALSE, &rc);
		}
		MoveWindow((HWND)tci.lParam, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top , FALSE);
	}
	TabCtrl_SetCurSel (hWndTabs, 0);
}

LRESULT CALLBACK VoiceDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG:
		{
			char *pszTabs[] = {TEXT("Operator 1"), TEXT("Operator 2"), TEXT("Operator 3"), TEXT("Operator 4")};

			CreateTabs(hWnd, IDC_TABCHANNELS, IDD_PROPPAGE_OPERATOR, OperatorDlgProc, pszTabs, sizeof(pszTabs)/sizeof(pszTabs[0]));
			return TRUE;
		}
		case WM_NOTIFY:
		{		
			switch(((LPNMHDR)lParam)->code)
			{
				case TCN_SELCHANGING:
				case TCN_SELCHANGE:
				{
					TCITEM tci={0};
					int nCurSel;

					nCurSel = TabCtrl_GetCurSel(((LPNMHDR)lParam)->hwndFrom);
					tci.mask = TCIF_PARAM;

					TabCtrl_GetItem (((LPNMHDR)lParam)->hwndFrom, nCurSel, &tci);
					ShowWindow ((HWND)tci.lParam, ((LPNMHDR)lParam)->code==TCN_SELCHANGING?SW_HIDE:SW_SHOW);
					SetFocus ((HWND)tci.lParam);
					break;
				}
			}
			break;
		}
	}
	return FALSE;

}

//=============================== MAIN DIALOG ====================================//

void EnablePlayButtons(HWND hWnd, BOOL fEnable)
{
	UINT auMidiButt[] = {IDC_PLAY, IDC_PLAYMJCHORD, IDC_PLAYMNCHORD, IDC_PLAYAUGCHORD, 
		IDC_PLAYDIMCHORD, IDC_PLAYMJ7CHORD, IDC_PLAYMN7CHORD, IDC_SHUT}, i;

	for (i=0; i<sizeof(auMidiButt)/sizeof(auMidiButt[0]); i++)
		EnableWindow(GetDlgItem(hWnd, auMidiButt[i]), fEnable);
}

void EnumMIDIDevices(HWND hWnd)
{
	HWND hWndCB = GetDlgItem(hWnd, IDC_MIDIDEV);
    UINT i, numMidiInDevs = midiInGetNumDevs();
    
    for (i=0; i<numMidiInDevs; i++) 
	{
        MIDIINCAPSA midiCaps = {0};
        
        if (midiInGetDevCapsA(i, &midiCaps, sizeof(MIDIINCAPSA)) == MMSYSERR_NOERROR) 
			SendMessage(hWndCB, CB_SETITEMDATA, SendMessage(hWndCB, CB_ADDSTRING, 0, (LPARAM)midiCaps.szPname), i);
    }
}

void EnumDevCB(ESS_DEVCFG *pCfg, void *pUser)
{
	HWND hWnd = (HWND)pUser, hWndCB = GetDlgItem(hWnd, IDC_DEVICE);

	SendMessage(hWndCB, CB_SETITEMDATA, SendMessage(hWndCB, CB_ADDSTRING, 0, (LPARAM)pCfg->szDevName), 
		MAKELPARAM(pCfg->Ports[ESSPortIO], pCfg->Ports[ESSPortSB]));
}


LRESULT CALLBACK MainDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int iLastSel = 0;
	static char szCurrentFileName[MAX_PATH]={0};
	static HMIDIIN hmi = NULL;

	switch (message) 
	{
		case WM_INITDIALOG:
		{
			char *pszTabs[] = {TEXT("Voice 1"), TEXT("Voice 2")};

			SendDlgItemMessage (hWnd, IDC_SPINNOTE, UDM_SETRANGE, 0, MAKELONG(127,0));
			SetDlgItemInt(hWnd, IDC_NOTE, 60, FALSE);
			CreateTabs(hWnd, IDC_TABVOICE, IDD_PROPPAGE_VOICES, VoiceDlgProc, pszTabs, sizeof(pszTabs)/sizeof(pszTabs[0]));
			SendDlgItemMessage (hWnd, IDC_INSTRUMENTS, LB_INITSTORAGE, 128, 128*20);
			CheckDlgButton(hWnd, IDC_MELODIC, BST_CHECKED);
			SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_MELODIC, BN_CLICKED), 0);
			SendMessage((HWND)lParam, LB_SETCURSEL, 0, 0);
			if (EnumESSDevices(hWnd, EnumDevCB, hWnd))
			{
				HWND hWndCB = GetDlgItem(hWnd, IDC_DEVICE);
				SendMessage(hWndCB, CB_SETCURSEL, 0, 0);
				SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_DEVICE, CBN_SELCHANGE), (LPARAM)hWndCB);
			}
			EnumMIDIDevices(hWnd);
			if (GetFileAttributes("bnk_common.bin") != 0xFFFFFFFF)
			{
				lstrcpy(szCurrentFileName, "bnk_common.bin");
				LoadPatchSet(hWnd, szCurrentFileName);
				EnableMenuItem(GetMenu(hWnd), IDM_SAVE, MF_ENABLED);
				PatchSetToMask(hWnd, &m_patches.patches[0]);
			}
			return TRUE;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDC_PERCUSSION:
			case IDC_MELODIC:
				if (HIWORD(wParam) == BN_CLICKED)
				{
					int i, j;
					char kind = LOWORD(wParam) == IDC_MELODIC?'M':'P', szInstrument[32];
					HWND hWndInstr = GetDlgItem(hWnd, IDC_INSTRUMENTS);

					SendMessage(hWndInstr, LB_RESETCONTENT, 0, 0);
					for (i=0, j=0; i<sizeof(Gm1Set)/sizeof(Gm1Set[0]) && j<128; i++)
					{
						if (Gm1Set[i].kind == kind)
						{
							if (Gm1Set[i].program == j)
							{
								SendMessage(hWndInstr , LB_SETITEMDATA, 
									SendMessage(hWndInstr, LB_ADDSTRING, 0, (LPARAM)Gm1Set[i].patchName), (LPARAM)&Gm1Set[i]);
							}
							else
							{
								wsprintf (szInstrument, "<Reserved %d>", j);
								SendMessage(hWndInstr, LB_ADDSTRING, 0, (LPARAM)szInstrument);
								i--;
							}
							j++;
						}
					}
					UpdateWindow (hWndInstr);

				}
				break;
			case IDC_INSTRUMENTS:
				if (HIWORD(wParam) == LBN_SELCHANGE)
				{
					int iCurSel = GetCurrentInstrument(hWnd);
					int iMsgReturn = IDNO;
					if (iCurSel != LB_ERR && iCurSel != iLastSel)
					{
						if (PatchsetChanged(hWnd, &m_patches.patches[iLastSel], FALSE))
						{
							switch (iMsgReturn = MessageBox(hWnd, 
								"You have not applies changes made to the instrument yet. Do you want to save the changes now?", "Save changes?", 
								MB_YESNOCANCEL | MB_ICONQUESTION))
							{
							case IDYES:
								PatchsetChanged(hWnd, &m_patches.patches[iLastSel], TRUE);
								break;
							case IDCANCEL:
								iCurSel = iLastSel;
								SendMessage((HWND)lParam, LB_SETCURSEL, iCurSel, 0);
								break;
							}
						}
						if (iMsgReturn != IDCANCEL)
						{
							PatchSetToMask(hWnd, &m_patches.patches[iCurSel]);
							iLastSel = iCurSel;
						}
					}
				}
				break;
			case IDC_DEVICE:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					char szPort[8];
					LPARAM lPorts = SendMessage((HWND)lParam, CB_GETITEMDATA, SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0), 0);
					wsprintf(szPort, "%x", HIWORD(lPorts));
					SetDlgItemText(hWnd, IDC_SBBASE, szPort);
				}
				break;
			case IDC_2NDVOICE:
				Change2ndVoice(hWnd, SendMessage((HWND)lParam, BM_GETCHECK, 0, 0)==BST_CHECKED, FALSE);
				break;
			case IDC_CONNECTDEV:
				if (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0)==BST_CHECKED)
				{
					char szPort[16] = {'0','x',0};
					int iPort;
					
					// Connect
					if (!GetDlgItemText(hWnd, IDC_SBBASE, szPort+2, sizeof(szPort)-2) ||
						!StrToIntEx(szPort, STIF_SUPPORT_HEX, &iPort))
					{
						MessageBox(hWnd, "Invalid IO Address given, has to be hexadecimal address, please correct.", "Error",
							MB_OK | MB_ICONSTOP);
						SendMessage((HWND)lParam, BM_SETCHECK, BST_UNCHECKED, 0);
						break;
					}
					if (!esfm_init((USHORT)iPort))
					{
						MessageBox(hWnd, "Cannot initialize Port IO driver, are you sure that it is installed and running and you started this application as administrator?", "Error",
							MB_OK | MB_ICONSTOP);
						SendMessage((HWND)lParam, BM_SETCHECK, BST_UNCHECKED, 0);
						break;
					}
					else
					{
						EnablePlayButtons(hWnd, TRUE);
					}
					break;
				}
				else
				{
					esfm_exit();
					EnablePlayButtons(hWnd, FALSE);
				}
				break;
			case IDC_CONNECTMIDI:
				if (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0)==BST_CHECKED)
				{
					// Connect
					HWND hWndCB = GetDlgItem(hWnd, IDC_MIDIDEV);
					int iSel = (int)SendMessage(hWndCB, CB_GETCURSEL, 0, 0);

					if (iSel != CB_ERR)
					{
						hmi = MIDIlstnStart(hWnd, (UINT)SendMessage(hWndCB, CB_GETITEMDATA, iSel, 0));
						if (hmi) break;
					}
					else
					{
						MessageBox(hWnd, "No MIDI input device selected", "Error", MB_OK | MB_ICONSTOP);
					}
					SendMessage((HWND)lParam, BM_SETCHECK, BST_UNCHECKED, 0);
					break;
				}
				else
				{
					if (hmi)
					{
						MIDIlstnStop(hmi);
						hmi = NULL;
					}
				}
				break;
			
			case IDC_PLAY:
			case IDC_PLAYMJCHORD:
			case IDC_PLAYMNCHORD:
			case IDC_PLAYAUGCHORD:
			case IDC_PLAYDIMCHORD:
			case IDC_PLAYMJ7CHORD:
			case IDC_PLAYMN7CHORD:
			{
				int n = GetDlgItemInt(hWnd, IDC_NOTE, NULL, FALSE);
				int iInstrument = GetCurrentInstrument(hWnd);
				PATCHSET psBak = m_patches.patches[iInstrument];

				// Temporarily apply current settings for preview 
				PatchsetChanged(hWnd, &m_patches.patches[iInstrument], TRUE);
				if (iInstrument > 127)
				{
					MidiMessage(0x7f0099 | ((iInstrument - 128) << 8));
				} 
				else 
				{
					MidiMessage(0xc0 | (iInstrument << 8));
					MidiMessage(0x7f0090 | (n << 8));
					switch (LOWORD(wParam))
					{
					case IDC_PLAYMJCHORD:
						MidiMessage(0x7f0090 | ((n-12) << 8));
						MidiMessage(0x7f0090 | ((n+4) << 8));
						MidiMessage(0x7f0090 | ((n-5) << 8));
						break;
					case IDC_PLAYMNCHORD:
						MidiMessage(0x7f0090 | ((n-12) << 8));
						MidiMessage(0x7f0090 | ((n+3) << 8));
						MidiMessage(0x7f0090 | ((n-5) << 8));
						break;
					case IDC_PLAYAUGCHORD:
						MidiMessage(0x7f0090 | ((n-12) << 8));
						MidiMessage(0x7f0090 | ((n+3) << 8));
						MidiMessage(0x7f0090 | ((n-4) << 8));
						break;
					case IDC_PLAYDIMCHORD:
						MidiMessage(0x7f0090 | ((n-12) << 8));
						MidiMessage(0x7f0090 | ((n+3) << 8));
						MidiMessage(0x7f0090 | ((n-6) << 8));
						break;
					case IDC_PLAYMJ7CHORD:
						MidiMessage(0x7f0090 | ((n-12) << 8));
						MidiMessage(0x7f0090 | ((n-2) << 8));
						MidiMessage(0x7f0090 | ((n+4) << 8));
						MidiMessage(0x7f0090 | ((n-5) << 8));
						break;
					case IDC_PLAYMN7CHORD:
						MidiMessage(0x7f0090 | ((n-12) << 8));
						MidiMessage(0x7f0090 | ((n-2) << 8));
						MidiMessage(0x7f0090 | ((n+3) << 8));
						MidiMessage(0x7f0090 | ((n-5) << 8));
						break;
					}
				}
				m_patches.patches[iInstrument] = psBak;
				break;
			}
			case IDC_SHUT:
				MidiAllNotesOff();
				break;
			case IDM_SAVEAS:
			case IDM_OPEN:
				{
					OPENFILENAME ofn={0};
					char szFileName[MAX_PATH]={0};

					ofn.lStructSize=sizeof(ofn);
					ofn.hwndOwner=hWnd;
					ofn.nMaxFile=MAX_PATH;
					ofn.lpstrFile=szFileName;
					ofn.Flags=OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST;
					ofn.lpstrDefExt="bin";
					ofn.lpstrFilter=".bin files\0*.bin\0All files (*.*)\0*.*\0\0";
					ofn.lpstrTitle="Load patches";
					if (LOWORD(wParam) == IDM_OPEN)
					{
						lstrcpy(szFileName, szCurrentFileName);
						if (GetOpenFileName(&ofn) && *szFileName)
						{
							if (LoadPatchSet(hWnd, szFileName))
							{
								int iCurSel = GetCurrentInstrument(hWnd);

								lstrcpy(szCurrentFileName, szFileName);
								EnableMenuItem(GetMenu(hWnd), IDM_SAVE, MF_ENABLED);
								if (iCurSel == LB_ERR)
								{
									iCurSel = 0;
								}
								PatchSetToMask(hWnd, &m_patches.patches[iCurSel]);
							}
						}
					}
					else
					{
						ofn.Flags=OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
						if (GetSaveFileName(&ofn) && *szFileName)
						{
							if (SavePatchSet(hWnd, szFileName))
							{
								lstrcpy(szCurrentFileName, szFileName);
								EnableMenuItem(GetMenu(hWnd), IDM_SAVE, MF_ENABLED);
							}
						}
					}
					break;
				}
			case IDM_SAVE:
				SavePatchSet(hWnd, szCurrentFileName);
				break;			
			case IDM_EXIT:
				SendMessage(hWnd, WM_CLOSE, 0, 0);
				break;
			}
			break;
		}
		case WM_NOTIFY:
		{		
			switch(((LPNMHDR)lParam)->code)
			{
				case TCN_SELCHANGING:
				case TCN_SELCHANGE:
				{
					TCITEM tci={0};
					int nCurSel;

					nCurSel = TabCtrl_GetCurSel(((LPNMHDR)lParam)->hwndFrom);
					tci.mask = TCIF_PARAM;

					TabCtrl_GetItem (((LPNMHDR)lParam)->hwndFrom, nCurSel, &tci);
					ShowWindow ((HWND)tci.lParam, ((LPNMHDR)lParam)->code==TCN_SELCHANGING?SW_HIDE:SW_SHOW);
					SetFocus ((HWND)tci.lParam);
					break;
				}
			}
			break;
		}
		case WM_CLOSE:
		{
			DestroyWindow (hWnd);
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
	}
	return FALSE;
}

WPARAM MessagePump (HWND hWnd)
{
	MSG msg;

	while (GetMessage (&msg, NULL, 0, 0)>0)
	{
		if(!IsWindow(hWnd) || !IsDialogMessage(hWnd, &msg))
		{
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
	}
	return msg.wParam;
}

WPARAM ProcessPendingMessages(HWND hWnd)
{
	MSG msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)>0)
	{
		if(!IsWindow(hWnd) || !IsDialogMessage(hWnd, &msg))
		{
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
	}
	return msg.wParam;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow )
{
	int iRetVal, i;
	HWND hWndSplash;

	g_hInstance = hInstance;
	InitCommonControls();

	// Init our bloated virtual patches-table
	for (i=0; i<sizeof(m_patches.offs)/sizeof(m_patches.offs[0]); i++)
		if (i<128 || (i>127+35 && i<127+81))
			m_patches.offs[i] = 512 + (i*(USHORT)sizeof(m_patches.patches[0]));

	hWndSplash = CreateDialog (hInstance, MAKEINTRESOURCE(IDD_SPLASH), NULL, (DLGPROC)SplashDlgProc);
	ProcessPendingMessages(hWndSplash);
	g_hMainWnd = CreateDialog (hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)MainDlgProc);
	ProcessPendingMessages(g_hMainWnd);
	DestroyWindow(hWndSplash);
	if (g_hMainWnd)
	{
		iRetVal = (int)MessagePump (g_hMainWnd);
	}
	else
	{
		TellError (NULL, "Cannot create main window");
	}

	return iRetVal;
}
