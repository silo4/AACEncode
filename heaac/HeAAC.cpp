#include <Windows.h>
extern "C"{
#include "sbr_main.h"
#include "aacenc.h"
#include "aac_ram.h"
#include "aac_rom.h"
}; 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "HeAAC.h"

#define CORE_DELAY   (1600)
#define INPUT_DELAY  ((CORE_DELAY)*2 +6*64-2048+1)
#define MAX_DS_FILTER_DELAY 16                         
#define CORE_INPUT_OFFSET_PS (0)

#define MAX_CHANNELS 2

#define DEFAULT_BITRATE 26000


HEAACCodec::HEAACCodec()
{
	sbr_handler_ = NULL;
	aac_handler_ = NULL;

	//Ĭ�ϲ���
	sample_rate_ = 44100;
	bitrate_ = DEFAULT_BITRATE;
	channels_ = 2;

	writeOffset = INPUT_DELAY*MAX_CHANNELS;
	envReadOffset = 0;
	coreWriteOffset = 0;
	inputBuffer = NULL;
	outputBuffer = NULL;
	m_maxOutputBytes = 1024;

}

HEAACCodec::~HEAACCodec()
{
	Close();
}

bool HEAACCodec::Init(unsigned long sampleRate, unsigned int numChannels, unsigned long &inputSamples,
					  unsigned long bitRate, unsigned int inputFormat)
{
	Close();

	//��֧�ֵ����ʵĲ���
	if(sampleRate < 44100 || bitRate < 24000 || numChannels < 2)
	{
		return false;
	}

	if(bitRate > DEFAULT_BITRATE)
		bitRate = DEFAULT_BITRATE;

	//Ĭ�ϲ���
	sample_rate_ = sampleRate;
	bitrate_ = bitRate;
	channels_ = numChannels;

	useParametricStereo = 0;

	if ((numChannels == 2) && (bitRate > 16000) && (bitRate < 36000))
	{
		useParametricStereo = 1;
	}

	unsigned int sample_rate_AAC = sample_rate_;
	unsigned int channels_AAC = numChannels;
	unsigned int channels_SBR = numChannels;

	if (useParametricStereo)
	{
		channels_AAC = 1;
		channels_SBR = 2;
	}

	AACENC_CONFIG   config;
	config.sampleRate = sampleRate;
	config.bitRate = bitRate;
	config.nChannelsIn = numChannels;
	config.nChannelsOut = channels_AAC;
	config.bandWidth = 0;

	//��ʼ��SBR
	if(!IsSbrSettingAvail(bitRate, channels_AAC, sample_rate_AAC, &sample_rate_AAC)) 
	{
		return false;
	}

	HANDLE_SBR_ENCODER hEnvEnc = NULL;
	sbrConfiguration sbrConfig;

	//ParametricStereo��λ��
	envReadOffset   = 0;
	coreWriteOffset = 0;
	if(useParametricStereo)
	{
		envReadOffset = (MAX_DS_FILTER_DELAY + INPUT_DELAY)*MAX_CHANNELS;
		coreWriteOffset = CORE_INPUT_OFFSET_PS;
		writeOffset = envReadOffset;
	}

	//���SBR��ʼ������
	InitializeSbrDefaults(&sbrConfig);

	AdjustSbrSettings(&sbrConfig,
		bitRate,
		channels_AAC,
		sample_rate_AAC,
		AACENC_TRANS_FAC,
		24000);

	//ʹ��ParametricStereo
	sbrConfig.usePs = useParametricStereo;
	EnvOpen(&hEnvEnc,
		inputBuffer + coreWriteOffset,
		&sbrConfig,
		&config.bandWidth);

	sbr_handler_ = (void *)hEnvEnc;

	//��ʼ��AAC
	config.sampleRate = sample_rate_AAC;

	struct AAC_ENCODER *aacEnc = 0;
	if(AacEncOpen(&aacEnc, config) != 0)
	{
		AacEncClose(aacEnc);
		EnvClose(hEnvEnc);
		return false;
	}

	aac_handler_ = (void *)aacEnc;

	//���ٱ������ڴ�
	inputBuffer = new float[(AACENC_BLOCKSIZE*2 + MAX_DS_FILTER_DELAY + INPUT_DELAY)*MAX_CHANNELS];
	outputBuffer = new unsigned int[(6144/8)*MAX_CHANNELS/(sizeof(int))];

	inputSamples = AACENC_BLOCKSIZE * 4;

	m_outputBuffer = auto_ptr<unsigned char>(new unsigned char [m_maxOutputBytes]);

	encode_frame_ = false;

	return true;
}

void HEAACCodec::Close()
{
	if(sbr_handler_ != NULL)
	{
		EnvClose((HANDLE_SBR_ENCODER)sbr_handler_);
		sbr_handler_ = NULL;
	}

	if(aac_handler_ != NULL && encode_frame_)
	{
		AacEncClose((struct AAC_ENCODER *)aac_handler_);
		aac_handler_ = NULL;
	}

	if(inputBuffer != NULL)
	{
		delete []inputBuffer;
		inputBuffer = NULL;
	}

	if(outputBuffer != NULL)
	{
		delete []outputBuffer;
		outputBuffer = NULL;
	}

	return;
}

int HEAACCodec::Encode(unsigned char *pBufIn, unsigned int nLenIn)
{
	int code_size = 0;

	if(sbr_handler_ == NULL || aac_handler_ == NULL || nLenIn <= 0)
	{
		return -1;
	}
	
	short* raw = (short *)pBufIn;

	unsigned int numAncDataBytes=0;
	unsigned char ancDataBytes[MAX_PAYLOAD_SIZE]; 
	unsigned int ancDataLength = 0;

	//ת��FLOAT���б���
	for(register int i=0; i<nLenIn / 2; ++ i) 
	{
		inputBuffer[i+writeOffset] = (float) raw[i];
	}

	//sbr
	EnvEncodeFrame((HANDLE_SBR_ENCODER)sbr_handler_,
		inputBuffer + envReadOffset,
		inputBuffer + coreWriteOffset,
		MAX_CHANNELS,
		&numAncDataBytes,
		ancDataBytes); 

	int out_size = 0; 
	if(useParametricStereo)
	{
		AacEncEncode((struct AAC_ENCODER *)aac_handler_, 
			inputBuffer,
			1, /* stride */
			ancDataBytes,
			&numAncDataBytes,
			outputBuffer,
			&out_size);
	} 
	else
	{
		AacEncEncode((struct AAC_ENCODER *)aac_handler_,
			inputBuffer+0,
			MAX_CHANNELS,
			ancDataBytes,
			&numAncDataBytes,
			outputBuffer,
			&out_size);
	}


	code_size = out_size;

	memcpy(inputBuffer,inputBuffer+AACENC_BLOCKSIZE,CORE_INPUT_OFFSET_PS*sizeof(float));
	
	unsigned char *code = GetBuffer();
	memcpy(code, outputBuffer, code_size);

	encode_frame_ = true;

	return out_size;
}

