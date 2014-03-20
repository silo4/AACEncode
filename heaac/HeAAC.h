#ifndef _HE_AAC_CODEC_H_
#define _HE_AAC_CODEC_H_


#include <memory>
using namespace std;

class HEAACCodec
{
public:
	HEAACCodec();
	virtual ~HEAACCodec();

	bool Init(unsigned long sampleRate, unsigned int numChannels, unsigned long &inputSamples, 		
		unsigned long bitRate, unsigned int inputFormat);
	void Close();
	unsigned char * GetBuffer() { return m_outputBuffer.get(); }
	int Encode(unsigned char *pBufIn, unsigned int nLenIn);

protected:
	void*	sbr_handler_;
	void*	aac_handler_;

	unsigned int	sample_rate_;
	unsigned int	bitrate_;
	unsigned int	channels_;

	unsigned long m_maxOutputBytes;
	unsigned long m_bitRate;
	unsigned int m_inputFormat;

	float*	inputBuffer;
	unsigned int* outputBuffer;

	//∂¡–¥Œª÷√
	int		writeOffset;
	int		envReadOffset;
	int		coreWriteOffset;
	int		useParametricStereo;
	bool	encode_frame_;

	auto_ptr<unsigned char> m_outputBuffer;
};


#endif