// SoundRecorderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SoundRecorder.h"
#include "SoundRecorderDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TIMER_WAVE_RECORD  1
#define TIMER_WAVE_PLAY    2

//++ for wave file
typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef signed   short  int16_t;

#pragma pack(1)
typedef struct {
    char     chunk_id[4];
    uint32_t chunk_size;
    char     riff_fmt[4];
} RIFF_HEADER;

typedef struct {
    uint16_t format_tag;
    uint16_t channels;
    uint32_t sample_per_sec;
    uint32_t avgbyte_per_sec;
    uint16_t block_align;
    uint16_t bits_per_sample;
} WAVE_FORMAT;

typedef struct {
    char     fmt_id[4];
    uint32_t fmt_size;
    WAVE_FORMAT wav_format;
} FMT_BLOCK;

typedef struct {
    char     data_id[4];
    uint32_t data_size;
} DATA_BLOCK;
#pragma pack()

void save_wave_file(char *file, int samprate, int channels, INT16 *buf, int len)
{
    FILE *fp;
    RIFF_HEADER header = {0};
    FMT_BLOCK   format = {0};
    DATA_BLOCK  datblk = {0};
    memcpy(header.chunk_id, "RIFF", 4);
    memcpy(header.riff_fmt, "WAVE", 4);
    memcpy(format.fmt_id  , "fmt ", 4);
    memcpy(datblk.data_id , "data", 4);
    format.fmt_size = sizeof(WAVE_FORMAT);
    format.wav_format.format_tag      = 1;
    format.wav_format.channels        = channels;
    format.wav_format.sample_per_sec  = samprate;
    format.wav_format.avgbyte_per_sec = samprate * channels * sizeof(INT16);
    format.wav_format.block_align     = channels * sizeof(INT16);
    format.wav_format.bits_per_sample = 16;
    datblk.data_size = len;
    header.chunk_size = sizeof(header) - 8 + sizeof(format) + sizeof(datblk) + datblk.data_size;

    fopen_s(&fp, file, "wb");
    if (fp) {
        fwrite(&header, sizeof(header), 1, fp);
        fwrite(&format, sizeof(format), 1, fp);
        fwrite(&datblk, sizeof(datblk), 1, fp);
        fwrite(buf, len, 1, fp);
        fclose(fp);
    }
}
//-- for wave file

#define IOCTL_AUD_MICGAIN_GET  CTL_CODE(FILE_DEVICE_SOUND, 2058, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AUD_MICGAIN_SET  CTL_CODE(FILE_DEVICE_SOUND, 2054, METHOD_BUFFERED, FILE_ANY_ACCESS)
int get_mic_gain(int *gain)
{
    HANDLE h = CreateFile(L"WAV1:", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        MessageBox(NULL, L"failed to open wavdev !", L"error", MB_OK);
        return -1;
    }
    DWORD ret = DeviceIoControl(h, IOCTL_AUD_MICGAIN_GET, NULL, 0, gain, sizeof(DWORD), NULL, NULL);
    if (!ret) {
//      MessageBox(NULL, L"failed to get mic gain !", L"error", MB_OK);
    }
    CloseHandle(h);
    return ret ? 0 : -1;
}

int set_mic_gain(int gain)
{
    HANDLE h = CreateFile(L"WAV1:", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        MessageBox(NULL, L"failed to open wavdev !", L"error", MB_OK);
        return -1;
    }
    DWORD ret = DeviceIoControl(h, IOCTL_AUD_MICGAIN_SET, &gain, sizeof(DWORD), NULL, 0, NULL, NULL);
    if (!ret) {
//      MessageBox(NULL, L"failed to set mic gain !", L"error", MB_OK);
    }
    CloseHandle(h);
    return ret ? 0 : -1;
}

// CSoundRecorderDlg dialog

CSoundRecorderDlg::CSoundRecorderDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CSoundRecorderDlg::IDD, pParent)
    , m_hWaveIn (NULL)
    , m_hWaveOut(NULL)
    , m_nSampRate(0)
    , m_nChannels(0)
    , m_nMicGain(0)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSoundRecorderDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Radio(pDX, IDC_RADIO_SAMPRATE, m_nSampRate);
    DDX_Radio(pDX, IDC_RADIO_CHANNELS, m_nChannels);
    DDX_Text(pDX, IDC_EDT_MIC_GAIN, m_nMicGain);
}

BEGIN_MESSAGE_MAP(CSoundRecorderDlg, CDialog)
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
    ON_WM_SIZE()
#endif
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_BTN_RECORD, &CSoundRecorderDlg::OnBnClickedBtnRecord)
    ON_BN_CLICKED(IDC_BTN_PLAY, &CSoundRecorderDlg::OnBnClickedBtnPlay)
    ON_BN_CLICKED(IDC_BTN_REC2FILE, &CSoundRecorderDlg::OnBnClickedBtnRec2file)
    ON_BN_CLICKED(IDC_BTN_STOP, &CSoundRecorderDlg::OnBnClickedBtnStop)
    ON_BN_CLICKED(IDC_BTN_INC_GAIN, &CSoundRecorderDlg::OnBnClickedBtnIncGain)
    ON_BN_CLICKED(IDC_BTN_DEC_GAIN, &CSoundRecorderDlg::OnBnClickedBtnDecGain)
    ON_WM_DESTROY()
    ON_WM_TIMER()
END_MESSAGE_MAP()


// CSoundRecorderDlg message handlers

BOOL CSoundRecorderDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    // TODO: Add extra initialization here
    m_pWaveBuf = (INT16*)calloc(1, WAVE_BUF_SIZE);
    memset(&m_tWaveHdrIn , 0, sizeof(m_tWaveHdrIn ));
    memset(&m_tWaveHdrOut, 0, sizeof(m_tWaveHdrOut));
    get_mic_gain(&m_nMicGain);
    UpdateData(FALSE);
    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSoundRecorderDlg::OnDestroy()
{
    CDialog::OnDestroy();
    if (m_pWaveBuf) {
        free(m_pWaveBuf);
        m_pWaveBuf = NULL;
    }
}

#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
void CSoundRecorderDlg::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
    if (AfxIsDRAEnabled()) {
        DRA::RelayoutDialog(
            AfxGetResourceHandle(),
            this->m_hWnd,
            DRA::GetDisplayMode() != DRA::Portrait ?
            MAKEINTRESOURCE(IDD_SOUNDRECORDER_DIALOG_WIDE) :
            MAKEINTRESOURCE(IDD_SOUNDRECORDER_DIALOG));
    }
}
#endif

void CSoundRecorderDlg::OnBnClickedBtnRecord()
{
    OnBnClickedBtnStop();
    SetTimer(TIMER_WAVE_RECORD, 100, NULL);
}

void CSoundRecorderDlg::OnBnClickedBtnPlay()
{
    OnBnClickedBtnStop();
    SetTimer(TIMER_WAVE_PLAY, 100, NULL);
}

void CSoundRecorderDlg::OnBnClickedBtnRec2file()
{
    char file[MAX_PATH];
    OnBnClickedBtnStop();
    sprintf_s(file, sizeof(file), "\\SDMMC\\record_%dhz_%s.wav", m_tWaveFmt.nSamplesPerSec, m_tWaveFmt.nChannels == 1 ? "mono" : "stereo");
    save_wave_file(file, m_tWaveFmt.nSamplesPerSec, m_tWaveFmt.nChannels, m_pWaveBuf, WAVE_BUF_SIZE);
}

void CSoundRecorderDlg::OnBnClickedBtnStop()
{
    if (m_hWaveIn) {
        waveInStop(m_hWaveIn);
        waveInUnprepareHeader(m_hWaveIn, &m_tWaveHdrIn, sizeof(WAVEHDR));
        waveInClose(m_hWaveIn);
        m_hWaveIn = NULL;
    }
    if (m_hWaveOut) {
        waveOutReset(m_hWaveOut);
        waveOutUnprepareHeader(m_hWaveOut, &m_tWaveHdrOut, sizeof(WAVEHDR));
        waveOutClose(m_hWaveOut);
        m_hWaveOut = NULL;
    }
}

BOOL CSoundRecorderDlg::PreTranslateMessage(MSG *pMsg)
{
    switch (pMsg->message) {
    case MM_WIM_CLOSE:
    case MM_WOM_DONE:
    case MM_WOM_CLOSE:
        OnBnClickedBtnStop();
        break;
    }
    return CDialog::PreTranslateMessage(pMsg);
}

void CSoundRecorderDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_WAVE_RECORD || nIDEvent == TIMER_WAVE_PLAY) {
        static int samprate_tab[] = { 48000, 44100, 32000, 16000, 11050, 8000 };
        UpdateData(TRUE);
        m_tWaveFmt.wFormatTag     = WAVE_FORMAT_PCM;
        m_tWaveFmt.nChannels      = m_nChannels == 0 ? 1 : 2;
        m_tWaveFmt.nSamplesPerSec = samprate_tab[m_nSampRate];
        m_tWaveFmt.nAvgBytesPerSec= m_tWaveFmt.nSamplesPerSec * sizeof(INT16) * m_tWaveFmt.nChannels;
        m_tWaveFmt.nBlockAlign    = sizeof(INT16) * m_tWaveFmt.nChannels;
        m_tWaveFmt.wBitsPerSample = 16;
    }

    switch (nIDEvent) {
    case TIMER_WAVE_RECORD:
        KillTimer(TIMER_WAVE_RECORD);
        waveInOpen(&m_hWaveIn, WAVE_MAPPER, &m_tWaveFmt, (DWORD)GetSafeHwnd(), NULL, CALLBACK_WINDOW);
        memset(m_pWaveBuf, 0, WAVE_BUF_SIZE);
        m_tWaveHdrIn.lpData         = (LPSTR)m_pWaveBuf;
        m_tWaveHdrIn.dwBufferLength = WAVE_BUF_SIZE;
        waveInPrepareHeader(m_hWaveIn, &m_tWaveHdrIn, sizeof(WAVEHDR));
        waveInAddBuffer(m_hWaveIn, &m_tWaveHdrIn, sizeof(WAVEHDR));
        waveInStart(m_hWaveIn);
        break;

    case TIMER_WAVE_PLAY:
        KillTimer(TIMER_WAVE_PLAY);
        waveOutOpen(&m_hWaveOut, WAVE_MAPPER, &m_tWaveFmt, (DWORD)GetSafeHwnd(), NULL, CALLBACK_WINDOW);
        m_tWaveHdrOut.lpData         = (LPSTR)m_pWaveBuf;
        m_tWaveHdrOut.dwBufferLength = WAVE_BUF_SIZE;
        waveOutPrepareHeader(m_hWaveOut, &m_tWaveHdrOut, sizeof(WAVEHDR));
        waveOutRestart(m_hWaveOut);
        waveOutWrite(m_hWaveOut, &m_tWaveHdrOut, sizeof(WAVEHDR));
        break;
    }

    CDialog::OnTimer(nIDEvent);
}

void CSoundRecorderDlg::OnBnClickedBtnIncGain()
{
    UpdateData(TRUE);
    m_nMicGain++;
    set_mic_gain( m_nMicGain);
    get_mic_gain(&m_nMicGain);
    UpdateData(FALSE);
}

void CSoundRecorderDlg::OnBnClickedBtnDecGain()
{
    UpdateData(TRUE);
    m_nMicGain--;
    set_mic_gain( m_nMicGain);
    get_mic_gain(&m_nMicGain);
    UpdateData(FALSE);
}
