
// AACEncodeDlg.h : header file
//

#pragma once

#include "afxwin.h"
#include <string>
#include <vector>
#include <list>
#include <DShow.h>
#include "streams.h"
#include "DShowUtils.h"
#include "DShowMic.h"
#include "DShowGrabberCB.h"
#include "../heaac/HeAAC.h"

#define  AUDIO_LABA_SIZE 5


using namespace ds;

typedef struct {
	int	sampleRate;
	int	nChannels;
	long	nSamples;
	int	aFmt;
} WavInfo;

//struct SCaptureDevice2
//{
//	std::wstring FriendlyName;
//	std::wstring Description;
//	std::wstring DevicePath;
//	bool bVideo;
//	GUID guid;
//	CComPtr<IBaseFilter> pFilter;
//};

// CAACEncodeDlg dialog
class CAACEncodeDlg : public CDialog
{
// Construction
public:
	CAACEncodeDlg(CWnd* pParent = NULL);	// standard constructor
	~CAACEncodeDlg();
// Dialog Data
	enum { IDD = IDD_AACENCODE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedBtnDevlist();

	void EncodeFromFile();
	HRESULT ListAudioDevice();

	void Close();
	BOOL Init(const std::wstring& Device, LONG nSamplesPerSec, LONG nBitRate);
	void RunIt();

	static void Audio_BufferCB(SGrabberSample sample);
	void ComputeAudio(USHORT* data, int dataSize);
	
	void ResetVolumn();
	USHORT GetAverageValue();
	void SetADTSHead3(unsigned char* buf, unsigned int bufLength);

	//CEdit m_dir;
	CString m_filePath;
	CString m_curDirPath;
	CStatic m_prompt;

	std::vector<SCaptureDevice*> m_capDev;
	CComboBox m_devList;
	
	Mic_Graph m_micGraph;
	BOOL m_bCancel;
	HANDLE m_hThread;
	LONG m_nValueCycleMS;
	double m_nTimeStamp;

	std::wstring m_Device;
	int m_nSamplesPerSec;
	int m_nBitRate;

	HEAACCodec m_codecAAC;
	unsigned long m_inputLenAAC;

	CCritSec m_audioLock;

	list<SGrabberSample> m_audioSampleList;

	CGrabberCB m_audioGrabberCB;
	static CAACEncodeDlg *g_pThisAudio;

	FILE* m_hPCMFile;
	FILE* m_hADTSFile;
	afx_msg void OnBnClickedButton3();
	long m_waveLength;
	CDC* m_dc;
	USHORT m_nAverageValue[AUDIO_LABA_SIZE];
	int	   m_nValuePos;
	string m_dir;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
