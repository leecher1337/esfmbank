/******************************************************************

    esfmregs.h - Structures for ESFM registers and patches file

    ESFM Bank editor

    Copyright (c) 2023, leecher@dose.0wnz.at  All Rights Reserved.

*******************************************************************/
#pragma pack(1)

typedef struct
{
	unsigned char MULT:4;
	unsigned char KSR:1;
	unsigned char EGT:1;
	unsigned char VIB:1;
	unsigned char TRM:1;
} OPREG0;

typedef struct
{
	unsigned char ATTENUATION:6;
	unsigned char KSL:2;
} OPREG1;

typedef struct
{
	unsigned char DECAY:4;
	unsigned char ATTACK:4;
} OPREG2;

typedef struct
{
	unsigned char RELEASE:4;
	unsigned char SUSTAIN:4;
} OPREG3;

typedef struct
{
	unsigned char FNUMlo;
} OPREG4;

typedef struct
{
	unsigned char FNUMhi:2;
	unsigned char BLOCK:3;
	unsigned char DELAY:3;
} OPREG5;

typedef struct
{
	unsigned char unk:1;
	unsigned char MOD:3;
	unsigned char L:1;
	unsigned char R:1;
	unsigned char VIBD:1;
	unsigned char TRMD:1;
} OPREG6;

typedef struct
{
	unsigned char WAVE:3;
	unsigned char NOISE:2;
	unsigned char OU:3;
} OPREG7;

typedef struct
{
	unsigned char PAT16:1;
	unsigned char OP:2;
	unsigned char unk1:1;
	unsigned char FP1:1;
	unsigned char FP2:1;
	unsigned char FP3:1;
	unsigned char FP4:1;
} HDR0;

typedef struct {
	unsigned char RV1:2;
	unsigned char RV2:2;
	unsigned char RV3:2;
	unsigned char RV4:2;
} HDR3;

typedef struct
{
	OPREG0 r0;
	OPREG1 r1;
	OPREG2 r2;
	OPREG3 r3;
	OPREG4 r4;
	OPREG5 r5;
	OPREG6 r6;
	OPREG7 r7;
} OPREG; 

typedef struct
{
	unsigned char h0;
	unsigned char h1;
	unsigned char h2;
	unsigned char h3;

	OPREG o[4];
} PATCH;

typedef struct
{
	PATCH p[2];
} PATCHSET;

#pragma pack()
