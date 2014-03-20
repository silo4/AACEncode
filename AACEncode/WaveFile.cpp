
#include "stdafx.h"
#include "WaveFile.h"  
#include <memory.h>  
#include <string.h>  
/* 
Name: 
     writeFile2Int 
Description: 
     在已知文件的指定位置，写一个整数 
     这里假设，int类型占四个字节 
Parameter:sFN    文件名 
     nWhere 准备写的位置（以零开始计数） 
     nValue 具体值。      
Remark: 
     若文件长度小于nWhere，数据写到文件位置0开始的地方 
TestEnv: 
     VS2008+SP1  32位程序 
*/  
bool writeFile2Int(FILE *fp,int nWhere,int nValue)  
{  
    if(fp==NULL)  
    {  
        return false;  
    }  
  
    fseek(fp,nWhere,SEEK_SET);  
    DWORD_CHAR dc;  
    dc.nValue=nValue;  
    fwrite(dc.charBuf,1,4,fp);        
    return true;  
}  
  
void writeWaveHead(FILE *fp)  
{  
    if (fp)  
    {  
        //写WAV文件头  
        RIFF_HEADER rh;  
        memset(&rh,0,sizeof(rh));  
        strncpy(rh.szRiffFormat,"WAVE",4);  
        strncpy(rh.szRiffID,"RIFF",4);  
        //rh.dwRiffSize = dataSize + 4 + sizeof(FMT_BLOCK) + sizeof(rh) - 8;//文件大小减去8  
  
        fwrite(&rh,1,sizeof(rh),fp);  
  
        FMT_BLOCK fb;  
        strncpy(fb.szFmtID,"fmt ",4);  
        fb.dwFmtSize = 16;  
        fb.wavFormat.wFormatTag = 0x0001;  
        fb.wavFormat.wChannels = 2;  
        fb.wavFormat.wBitsPerSample = 16;         
        fb.wavFormat.dwSamplesPerSec = 44100;         
        fb.wavFormat.wBlockAlign = fb.wavFormat.wChannels*fb.wavFormat.wBitsPerSample/8;   //4;  
        fb.wavFormat.dwAvgBytesPerSec = fb.wavFormat.dwSamplesPerSec * fb.wavFormat.wBlockAlign;  
          
  
        fwrite(&fb,1,sizeof(fb),fp);  
  
        char buf[]={"data0000"};  
        fwrite(buf,1,sizeof(buf),fp);  
    }  
}  
  
void writeWaveBody(FILE *fp,long filelength)  
{  
    //更新WAV文件dwRiffSize字段中的值  
    int nWhere = 4;  
    writeFile2Int(fp,nWhere, filelength - 8);  
  
    //更新WAV文件DataChunk中Size字段的值  
    nWhere=sizeof(RIFF_HEADER)+sizeof(FMT_BLOCK)+4;  
    writeFile2Int(fp,nWhere,filelength - (sizeof(RIFF_HEADER)+sizeof(FMT_BLOCK)+8) );  
}  