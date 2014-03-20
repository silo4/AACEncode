
// AACEncodeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AACEncode.h"
#include "AACEncodeDlg.h"
#include "WaveFile.h"
#include <string>
using namespace std;

#define  WAV_HEADER_SIZE  64
#define  ADTS_HEADER_SIZE 7
#define  MAX_CHANNELS     2
#define  DEFAULT_AUDIO_RATE 26000

#define  DRAW_VOLUMN_TIMER   0x100
#define  WAV_PATH  "D:\\kxwav\\"


CAACEncodeDlg *CAACEncodeDlg::g_pThisAudio = NULL;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CAACEncodeDlg dialog




CAACEncodeDlg::CAACEncodeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAACEncodeDlg::IDD, pParent),
	m_audioGrabberCB(Audio_BufferCB),
	m_hPCMFile(NULL),
	m_hADTSFile(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_curDirPath = L"d:\\wav\\";
	m_filePath = L"";

	m_Device = _T("");
	m_nSamplesPerSec = 44100;
	m_nBitRate = DEFAULT_AUDIO_RATE;

	m_inputLenAAC = 0;
	m_nTimeStamp = 0;

	m_bCancel = FALSE;
	m_hThread = NULL;

	m_nValueCycleMS = 0;

	g_pThisAudio = this;

	m_waveLength = 0;
	m_dc = NULL;

	m_dir = WAV_PATH;
}

CAACEncodeDlg::~CAACEncodeDlg()
{
	if (m_dc != NULL)
	{
		ReleaseDC(m_dc);
		m_dc = NULL;
	}
}

void CAACEncodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_EDIT1, m_dir);
	DDX_Control(pDX, IDC_STATICPROMPT, m_prompt);
	DDX_Control(pDX, IDC_COMBO1, m_devList);
}

BEGIN_MESSAGE_MAP(CAACEncodeDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CAACEncodeDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CAACEncodeDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON1, &CAACEncodeDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CAACEncodeDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BTN_DEVLIST, &CAACEncodeDlg::OnBnClickedBtnDevlist)
	ON_BN_CLICKED(IDC_BUTTON3, &CAACEncodeDlg::OnBnClickedButton3)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CAACEncodeDlg message handlers

BOOL CAACEncodeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	ListAudioDevice();
	ResetVolumn();
	BOOL bRet = ::CreateDirectory(_T(WAV_PATH), NULL);
	m_prompt.SetWindowText(L"请选择麦克风进行录音!");

	m_dc = GetDC();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAACEncodeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAACEncodeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAACEncodeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



inline FILE* AuChannelOpen (const char* filename, WavInfo* info)
{
	unsigned char header[12];
	unsigned char data[WAV_HEADER_SIZE];
	FILE *handle;
	unsigned int   chunksize;


	if (!strcmp(filename,"-"))
		handle = stdin;
	else
		handle = fopen(filename, "rb");

	if(!handle) return NULL;

	if(fread(header, 1, 12, handle) != 12) return NULL;
	info->nSamples		= (header[4] | (header[5] << 8) | (header[6] << 16) | (header[7] << 24)) + 8;

	while (memcmp(header, "data", 4) != 0){
		if(fread(header, 1, 8, handle) != 8) return NULL;
		chunksize             = (header[4] | (header[5] << 8) | (header[6] << 16) | (header[7] << 24));
		//fprintf(stderr, "%c%c%c%c %d", header[0],  header[1], header[2], header[3], chunksize);
		if(!memcmp(header, "fmt ", 4)) {
			if(chunksize > WAV_HEADER_SIZE) return NULL;
			if(fread(data, 1, chunksize, handle) != chunksize) return NULL;
			info->aFmt		= data[0] | data[1] << 8;
			info->nChannels		= data[2] | data[3] << 8;
			info->sampleRate	= data[4] | data[5] << 8 | data[6] << 12 | data[7] << 16;
		} else if(memcmp(header, "data", 4) != 0) {
			if(fseek(handle, chunksize, SEEK_CUR) != 0) return NULL;
		}
	}

	return handle;
}

inline size_t AuChannelReadShort(FILE *audioChannel, short *samples, int nSamples, int *readed)
{
	*readed = fread(samples, 2, nSamples, audioChannel);
	return *readed <= 0;
}

inline size_t AuChannelReadFloat(FILE *audioChannel, float *samples, int nSamples, int *readed)
{
	*readed = fread(samples, 4, nSamples, audioChannel);
	return *readed <= 0;
}

void CAACEncodeDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	//OnOK();

	int nIndex = m_devList.GetCurSel();
	CString tempSelDev;
	m_devList.GetLBText(nIndex, tempSelDev);
	std::wstring selDev(tempSelDev.GetBuffer(0));

	Close();
	Init(selDev, 44100, DEFAULT_AUDIO_RATE);
	RunIt();
}

void CAACEncodeDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	for (int i = 0; i < m_capDev.size(); ++i)
	{
		//m_capDev[i]->pFilter.Release();
		delete m_capDev[i];
		m_capDev[i] = NULL;
	}
	m_capDev.clear();

	OnCancel();
}

void CAACEncodeDlg::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);
	dlg.m_ofn.lpstrInitialDir = m_curDirPath.GetBuffer(0);
	CString strFilePath;
	if(dlg.DoModal() == IDOK)
	{
		CArray<CString, CString> aryFilename;
		POSITION posFile=dlg.GetStartPosition();
		while(posFile!=NULL)
		{
			aryFilename.Add(dlg.GetNextPathName(posFile));
		}
		for(int i=0;i<aryFilename.GetSize();i++)
		{
			if(strFilePath.GetLength()>0)
			{
				strFilePath.AppendChar(';');//多个文件‘;’分开
			}
			strFilePath+= aryFilename.GetAt(i);
		}
	}
	//m_dir.SetWindowText(strFilePath);
	int pos = strFilePath.ReverseFind('\\'); 
	m_curDirPath = strFilePath.Left(pos); 
	m_filePath = strFilePath;
	
}

void CAACEncodeDlg::OnBnClickedButton2()
{
	// TODO: Add your control notification handler code here
	m_curDirPath = _T(WAV_PATH);
	ShellExecute(NULL, NULL, _T("explorer"), m_curDirPath, NULL, SW_SHOW);
}

void CAACEncodeDlg::EncodeFromFile()
{
	m_prompt.SetWindowText(L"编码中,请耐心等待..........");

	WavInfo inputInfo;
	FILE *inputFile = NULL;
	FILE *hADTSFile;

	int  error;
	int  bEncodeMono = 0;//1表示单声道，0表示双声道
	int  frmCnt = 0;

	string inputPath = m_dir + "bywdsg.wav";
	inputFile = AuChannelOpen (inputPath.c_str(), &inputInfo);

	if(inputFile == NULL)
	{
		return;
	}

	if (inputInfo.nChannels==1 && !bEncodeMono) {
		return;
	}

	string outputPath = m_dir + "output.aac";
	hADTSFile = fopen(outputPath.c_str(), "wb");

	if(hADTSFile == NULL) 
	{
		return;
	}

	unsigned long inputSamples=0;
	unsigned long maxOutputBytes=0;

	HEAACCodec codecAAC;
	bool bRet = codecAAC.Init(inputInfo.sampleRate, inputInfo.nChannels, inputSamples, 26000, 1);
	if (!bRet)
	{
		m_prompt.SetWindowText(L"编码器初始化失败!");
		return;
	}

	int ret = 0;
	maxOutputBytes = (6144/8)*MAX_CHANNELS+ADTS_HEADER_SIZE;
	unsigned char *outputBuffer = (unsigned char*)malloc(maxOutputBytes);
	int  *TimeDataPcm;
	if(inputInfo.aFmt == 0xFFFE) {
		TimeDataPcm = (int*)calloc(inputSamples, sizeof(float));
	} else {
		TimeDataPcm = (int*)calloc(inputSamples, sizeof(short));
	}

	int stopLoop = 0;
	int bytes = 0;
	do {
		int numSamplesRead = 0;
		if(inputInfo.aFmt == 0xFFFE) {
			if ( AuChannelReadFloat(inputFile, (float *) TimeDataPcm, inputSamples, &numSamplesRead) > 0) {
				stopLoop = 1;
				break;
			}
		} else {
			if ( AuChannelReadShort(inputFile, (short *) TimeDataPcm, inputSamples, &numSamplesRead) > 0) {
				stopLoop = 1;
				break;
			}
		}

		if(numSamplesRead < inputSamples) {
			stopLoop = 1;
			break;
		}

		bytes = codecAAC.Encode((unsigned char *) TimeDataPcm, numSamplesRead);

		if(bytes > 0)
		{
			fwrite(codecAAC.GetBuffer(), bytes, 1, hADTSFile);
		}

	} while (!stopLoop && bytes >= 0);

	codecAAC.Close();
	fclose(hADTSFile);
	free(outputBuffer);
	free(TimeDataPcm);

	m_prompt.SetWindowText(L"编码结束，点击查看按钮");
}

HRESULT CAACEncodeDlg::ListAudioDevice()
{
	HRESULT hr = E_FAIL;

	m_capDev.clear();
	m_devList.ResetContent();

	CComPtr<ICreateDevEnum> pDevEnum;
	CComPtr<IEnumMoniker> pEnum;

	pDevEnum.CoCreateInstance( CLSID_SystemDeviceEnum );
	if ( !pDevEnum )
		return hr;

	hr = pDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEnum, 0);

	if (FAILED(hr))
		return hr;

	if(pEnum == NULL)
		return S_FALSE;

	CComPtr<IMoniker> pMoniker;
	while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
	{
		CComPtr<IPropertyBag> pPropBag;
		hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
			(void**)(&pPropBag));
		if (FAILED(hr))
		{
			pMoniker.Release();
			continue;  // Skip this one, maybe the next one will work.
		} 

		VARIANT varNameDescription;
		VariantInit(&varNameDescription);

		VARIANT varNameFriendly;
		VariantInit(&varNameFriendly);

		VARIANT varNamePath;
		VariantInit(&varNamePath);

		SCaptureDevice* cd = new SCaptureDevice;

		hr = pPropBag->Read(L"Description", &varNameDescription, 0);
		if (SUCCEEDED(hr))
			cd->Description = varNameDescription.bstrVal;
		hr = pPropBag->Read(L"FriendlyName", &varNameFriendly, 0);
		if (SUCCEEDED(hr))
			cd->FriendlyName = varNameFriendly.bstrVal;
		hr = pPropBag->Read(L"DevicePath", &varNamePath, 0);
		if (SUCCEEDED(hr))
			cd->DevicePath = varNamePath.bstrVal;

		CComPtr<IBaseFilter> pCap;
		hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pCap);
		if (SUCCEEDED(hr))
		{
			GUID guid = GUID_NULL;
			pCap->GetClassID(&guid);
 
			cd->bVideo = false;
			cd->guid = guid;
			cd->pFilter = pCap;
			m_capDev.push_back(cd);
		}

		pPropBag.Release();
		pMoniker.Release();
	}

	if (m_capDev.size() > 0)
	{
		for (int i = 0; i < m_capDev.size(); ++i)
		{
			m_devList.AddString(m_capDev[i]->FriendlyName.c_str());
		}
		m_devList.SetCurSel(0);
	}

	return S_OK;
}


void CAACEncodeDlg::OnBnClickedBtnDevlist()
{
	// TODO: Add your control notification handler code here
	//ListAudioDevice();
}

void CAACEncodeDlg::Close()
{
	m_micGraph.StopGraph();

	m_bCancel = TRUE;
	if (m_hThread)
	{
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	m_nTimeStamp = 0;
	m_nValueCycleMS = 0;
}

BOOL CAACEncodeDlg::Init(const wstring& Device, LONG nSamplesPerSec, LONG nBitRate)
{
	nBitRate = DEFAULT_AUDIO_RATE;

	m_Device = Device;
	m_nSamplesPerSec = nSamplesPerSec;
	m_nBitRate = nBitRate;

	//音频文件初始化
	if (m_hADTSFile == NULL)
	{
		string inPath = m_dir + "output.aac";
		m_hADTSFile = fopen(inPath.c_str(), "wb");
	}
	if (m_hPCMFile == NULL)
	{
		string outPath = m_dir + "capture.wav";
		m_hPCMFile = fopen(outPath.c_str(), "wb");
	}
	
	if (m_hPCMFile == NULL || m_hADTSFile == NULL)
	{
		m_prompt.SetWindowText(L"音频文件初始化出错，请在( D:/ )盘下创建名为 kxwav 的目录！");
		return FALSE;
	}
	
	//writeWaveHead(m_hPCMFile);

	if (!m_codecAAC.Init(nSamplesPerSec, 2, m_inputLenAAC, nBitRate, 1))
	{
		m_prompt.SetWindowText(L"编解码器初始化出错！");
		return FALSE;
	}
	
	m_inputLenAAC *= 2;

	if (m_capDev.size() <= 0)
	{
		return FALSE;
	}

	CComPtr<IBaseFilter> pFilterMic = m_capDev[0]->pFilter;
	for(int i = 0; i < m_capDev.size(); i ++)
	{
		if(m_capDev[i]->FriendlyName == Device)
		{
			pFilterMic = m_capDev[i]->pFilter;
			if (pFilterMic == NULL)
			{
				return FALSE;
			}

			break;
		}
	}

	HRESULT hr;
	hr = m_micGraph.InitInstance(pFilterMic, nSamplesPerSec, 2);
	if (FAILED(hr))
	{
		return FALSE;
	}

	CComPtr<ISampleGrabber> pGrabber = m_micGraph.GetAudioGrabber();
	if (pGrabber == NULL)
	{
		return FALSE;
	}

	hr = pGrabber->SetCallback(&m_audioGrabberCB, 1);
	if (FAILED(hr))
	{
		return FALSE;
	}

	hr = m_micGraph.RunGraph();
	if (FAILED(hr))
	{
		return FALSE;
	}

	m_bCancel = FALSE;

	m_prompt.SetWindowText(L"正在录制...");
	//SetTimer(DRAW_VOLUMN_TIMER, 100, NULL);

	return TRUE;
}

void CAACEncodeDlg::RunIt()
{
	
}

void CAACEncodeDlg::Audio_BufferCB( SGrabberSample sample )
{
	if (!g_pThisAudio)
		return;

	if (g_pThisAudio->m_bCancel)
	{
		g_pThisAudio->Close();
		return;
	}

	//计算音量
	g_pThisAudio->ComputeAudio((USHORT *)sample.pBuffer.get(), sample.BufferLen);

	//if (!CACameraCtrl::g_pThisCtrl->IsConnect())
		//return;

	//CACameraCtrl::g_pThisCtrl->SetStartAudio(TRUE);
	//DWORD n;
	//if (!CACameraCtrl::g_pThisCtrl->GetStarted(n))
		//return;

	//if (g_pThisAudio->m_nTimeStamp == 0)
	//{
	//	g_pThisAudio->m_nTimeStamp = timeGetTime() - n;
	//}

	CAutoLock lock(&(g_pThisAudio->m_audioLock));

	if (g_pThisAudio->m_inputLenAAC == 0)
		return;

	// Ignore TimeStamp
	long nAlignNeed = 0;
	if (g_pThisAudio->m_audioSampleList.size() > 0)
	{
		SGrabberSample &sampleLast = g_pThisAudio->m_audioSampleList.back();
		nAlignNeed = g_pThisAudio->m_inputLenAAC - sampleLast.BufferLen;
	}

	if (nAlignNeed > 0)
	{
		SGrabberSample &sampleLast = g_pThisAudio->m_audioSampleList.back();

		if (nAlignNeed < sample.BufferLen)
		{
			shared_ptr<BYTE> pBuffer(new BYTE [sampleLast.BufferLen + nAlignNeed]);
			memcpy(pBuffer.get(), sampleLast.pBuffer.get(), sampleLast.BufferLen);
			memcpy(pBuffer.get()+sampleLast.BufferLen, sample.pBuffer.get(), nAlignNeed);
			sampleLast.pBuffer = pBuffer;
			sampleLast.BufferLen += nAlignNeed;

			shared_ptr<BYTE> pBuffer1(new BYTE [sample.BufferLen - nAlignNeed]);
			memcpy(pBuffer1.get(), sample.pBuffer.get()+nAlignNeed, sample.BufferLen - nAlignNeed);
			sample.pBuffer = pBuffer1;
			sample.BufferLen -= nAlignNeed;
		}
		else
		{
			shared_ptr<BYTE> pBuffer(new BYTE [sampleLast.BufferLen + sample.BufferLen]);
			memcpy(pBuffer.get(), sampleLast.pBuffer.get(), sampleLast.BufferLen);
			memcpy(pBuffer.get()+sampleLast.BufferLen, sample.pBuffer.get(), sample.BufferLen);
			sampleLast.pBuffer = pBuffer;
			sampleLast.BufferLen += sample.BufferLen;
			return;
		}
	}

	DWORD nItems = sample.BufferLen / g_pThisAudio->m_inputLenAAC;
	for (int i = 0; i < nItems; i++)
	{
		DWORD BufferLen = g_pThisAudio->m_inputLenAAC;
		SGrabberSample sampleItem;
		shared_ptr<BYTE> pBuffer(new BYTE [BufferLen]);
		memcpy(pBuffer.get(), sample.pBuffer.get()+i*BufferLen, BufferLen);
		sampleItem.SampleTime = sample.SampleTime;
		sampleItem.pBuffer = pBuffer;
		sampleItem.BufferLen = BufferLen;
		sample.BufferLen -= BufferLen;

		g_pThisAudio->m_audioSampleList.push_back(sampleItem);
	}

	if ((sample.BufferLen % g_pThisAudio->m_inputLenAAC) != 0)
	{
		DWORD BufferLen = sample.BufferLen % g_pThisAudio->m_inputLenAAC;
		SGrabberSample sampleItem;
		shared_ptr<BYTE> pBuffer(new BYTE [BufferLen]);
		memcpy(pBuffer.get(), sample.pBuffer.get()+nItems*g_pThisAudio->m_inputLenAAC, BufferLen);
		sampleItem.SampleTime = sample.SampleTime;
		sampleItem.pBuffer = pBuffer;
		sampleItem.BufferLen = BufferLen;

		g_pThisAudio->m_audioSampleList.push_back(sampleItem);
	}

	// Process Audio
	CAACEncodeDlg *pThis = g_pThisAudio;
	DWORD nProcessItems = 0;
	list<SGrabberSample>::iterator i;
	for (i = pThis->m_audioSampleList.begin(); i != pThis->m_audioSampleList.end(); i++)
	{
		SGrabberSample &sample = *i;

		if (sample.BufferLen == pThis->m_inputLenAAC)
		{
			//这里写刚采集完的数据文件
			if (g_pThisAudio->m_hPCMFile != NULL)
			{
				//writeWaveBody(g_pThisAudio->m_hPCMFile, sample.BufferLen);	
				fwrite(sample.pBuffer.get(), sample.BufferLen, 1, g_pThisAudio->m_hPCMFile);
				g_pThisAudio->m_waveLength  += sample.BufferLen;
			}
			
			int nEncodeBytes = pThis->m_codecAAC.Encode(sample.pBuffer.get(), sample.BufferLen);
			if (nEncodeBytes > 0)
			{
				//这里写压缩后的文件	
				if (g_pThisAudio->m_hADTSFile != NULL)
				{
					g_pThisAudio->SetADTSHead3(g_pThisAudio->m_codecAAC.GetBuffer(), nEncodeBytes);
				}
			}
	
			nProcessItems++;
		}
		else
			break;
	}

	for (int j = 0; j < nProcessItems; j++)
		pThis->m_audioSampleList.pop_front();
}

//音量

void CAACEncodeDlg::ResetVolumn()
{
	m_nValuePos = 0;
	for(int i = 0; i < AUDIO_LABA_SIZE; i ++)
	{
		m_nAverageValue[i] = 0;
	}
}

USHORT CAACEncodeDlg::GetAverageValue()
{
	CAutoLock lock(&m_audioLock);

	USHORT ret = m_nAverageValue[m_nValuePos % AUDIO_LABA_SIZE];
	m_nValuePos ++;
	return ret;
}

void CAACEncodeDlg::ComputeAudio( USHORT* data, int dataSize )
{
	//清空音量
	ResetVolumn();

	CAutoLock lock(&(m_audioLock));
	m_nValueCycleMS = dataSize * 50 / (m_nSamplesPerSec); //每100MS画一次

	//最多获取前面40个字节的样本;
	int nMinFixedBytes = 40;
	ULONGLONG nSumValue = 0;
	USHORT* pos = data;
	int nPerNum =  dataSize / (2 * AUDIO_LABA_SIZE);

	if (nMinFixedBytes > nPerNum / 10)
		nMinFixedBytes = nPerNum / 10;

	for(int n = 0; n < AUDIO_LABA_SIZE; n ++) //分成5段分别存储在数组中
	{
		nSumValue = 0;
		pos = data + n * nPerNum;
		for (int i = 0; i < nMinFixedBytes; i++)
		{
			nSumValue += *pos;
			pos ++;
		}

		LONGLONG nTmp = 0;
		if(nMinFixedBytes > 0)
		{
			nTmp = nSumValue / nMinFixedBytes;
		}

		m_nAverageValue[n] = (USHORT)(nTmp);
	}
	
}

void CAACEncodeDlg::OnBnClickedButton3()
{
	// TODO: Add your control notification handler code here
	
	m_codecAAC.Close();

	if (m_hPCMFile != NULL)
	{
		//写文件头
		fseek(m_hPCMFile, 0, SEEK_SET);
		writeWaveHead(m_hPCMFile);
		writeWaveBody(m_hPCMFile, m_waveLength);
		m_waveLength = 0;

		fclose(m_hPCMFile);
		m_hPCMFile = NULL;
	}

	if (m_hADTSFile != NULL)
	{
		fclose(m_hADTSFile);
		m_hADTSFile = NULL;
	}

	ResetVolumn();

	//KillTimer(DRAW_VOLUMN_TIMER);

	CString strTemp;
	CString path = _T(WAV_PATH);
	strTemp.Format(L"已停止录制, 录音文件保存在 %s 下", path);
	m_prompt.SetWindowText(strTemp.GetBuffer());
	strTemp.ReleaseBuffer();
}

void CAACEncodeDlg::SetADTSHead3( unsigned char* buf, unsigned int bufLength )
{
	static char adtsHeader[7] = {0};  

	if (m_hADTSFile == NULL)  
	{  
		return;  
	}  

	adtsHeader[0] = 0xFF;  
	adtsHeader[1] = 0xF1;  
	adtsHeader[2] = 0x5C;  
	adtsHeader[3] = 0x80;  
	adtsHeader[4] = (bufLength+7) >> 3;  
	adtsHeader[5] = ((bufLength+7) & 0x7) << 5;  
	adtsHeader[6] = 0xFC;  

	if (7 != fwrite(adtsHeader, 1, 7, m_hADTSFile))  
	{  
		return;  
	}  

	if (bufLength != fwrite(buf, 1, bufLength, m_hADTSFile))  
	{  
		return;  
	}  
}

void CAACEncodeDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == DRAW_VOLUMN_TIMER)
	{
		if (m_dc == NULL)
		{
			return;
		}
		CRect rcVolumn, rcBk;
		GetClientRect(&rcBk);
		rcBk.bottom = 25;
		rcVolumn = rcBk;
		//
		int vol = (int)GetAverageValue();
		CString strVol;
		strVol.Format(L"%d", vol);
		//m_dc->SetBkMode(TRANSPARENT);
		//m_dc->DrawText(strVol, rcVolumn, DT_CENTER);
		m_dc->FillSolidRect(rcBk, RGB(0, 255, 0));
		float rat = vol/65535.0;
		rat = (rat > 1 ? 1 : rat);
		rcVolumn.right = rcVolumn.left + (int) (rcBk.Width() * rat);
		m_dc->FillSolidRect(rcVolumn, RGB(0, 0, 255));	
	}

	CDialog::OnTimer(nIDEvent);
}
