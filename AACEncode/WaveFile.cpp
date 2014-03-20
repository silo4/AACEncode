
#include "stdafx.h"
#include "WaveFile.h"  
#include <memory.h>  
#include <string.h>  
/* 
Name: 
     writeFile2Int 
Description: 
     ����֪�ļ���ָ��λ�ã�дһ������ 
     ������裬int����ռ�ĸ��ֽ� 
Parameter:sFN    �ļ��� 
     nWhere ׼��д��λ�ã����㿪ʼ������ 
     nValue ����ֵ��      
Remark: 
     ���ļ�����С��nWhere������д���ļ�λ��0��ʼ�ĵط� 
TestEnv: 
     VS2008+SP1  32λ���� 
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
        //дWAV�ļ�ͷ  
        RIFF_HEADER rh;  
        memset(&rh,0,sizeof(rh));  
        strncpy(rh.szRiffFormat,"WAVE",4);  
        strncpy(rh.szRiffID,"RIFF",4);  
        //rh.dwRiffSize = dataSize + 4 + sizeof(FMT_BLOCK) + sizeof(rh) - 8;//�ļ���С��ȥ8  
  
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
    //����WAV�ļ�dwRiffSize�ֶ��е�ֵ  
    int nWhere = 4;  
    writeFile2Int(fp,nWhere, filelength - 8);  
  
    //����WAV�ļ�DataChunk��Size�ֶε�ֵ  
    nWhere=sizeof(RIFF_HEADER)+sizeof(FMT_BLOCK)+4;  
    writeFile2Int(fp,nWhere,filelength - (sizeof(RIFF_HEADER)+sizeof(FMT_BLOCK)+8) );  
}  