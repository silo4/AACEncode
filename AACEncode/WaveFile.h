#pragma once  

#include <stdio.h>  
#include <Windows.h>  
#include <string>  

struct RIFF_HEADER  
{  
	char szRiffID[4];  // 'R','I','F','F'  
	DWORD dwRiffSize;  
	char szRiffFormat[4]; // 'W','A','V','E'  
};  

struct WAVE_FORMAT  
{  
	WORD wFormatTag;  
	WORD wChannels;  
	DWORD dwSamplesPerSec;  
	DWORD dwAvgBytesPerSec;  
	WORD wBlockAlign;  
	WORD wBitsPerSample;  
};  

struct FMT_BLOCK  
{  
	char  szFmtID[4]; // 'f','m','t',' '  
	DWORD  dwFmtSize;  
	WAVE_FORMAT wavFormat;  
};  

struct DATA_BLOCK  
{  
	char szDataID[4]; // 'd','a','t','a'  
	DWORD dwDataSize;  
};  

union DWORD_CHAR  
{  
	int  nValue;  
	char charBuf[4];  
};  

void writeWaveHead(FILE *fp);  
void writeWaveBody(FILE *fp,long filelength);  