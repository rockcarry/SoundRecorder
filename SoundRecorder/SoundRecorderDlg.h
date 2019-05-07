// SoundRecorderDlg.h : header file
//

#pragma once

// CSoundRecorderDlg dialog
class CSoundRecorderDlg : public CDialog
{
// Construction
public:
    CSoundRecorderDlg(CWnd* pParent = NULL);    // standard constructor

// Dialog Data
    enum { IDD = IDD_SOUNDRECORDER_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL PreTranslateMessage(MSG* pMsg);

// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    virtual BOOL OnInitDialog();
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
    afx_msg void OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/);
#endif
    DECLARE_MESSAGE_MAP()

private:
    #define WAVE_BUF_SIZE (sizeof(INT16) * 48000 * 2 * 10)
    WAVEFORMATEX m_tWaveFmt;
    HWAVEIN      m_hWaveIn;
    HWAVEOUT     m_hWaveOut;
    INT16       *m_pWaveBuf;
    WAVEHDR      m_tWaveHdrIn;
    WAVEHDR      m_tWaveHdrOut;
    int          m_nSampRate;
    int          m_nChannels;
    int          m_nMicGain;

public:
    afx_msg void OnBnClickedBtnRecord();
    afx_msg void OnBnClickedBtnPlay();
    afx_msg void OnBnClickedBtnRec2file();
    afx_msg void OnBnClickedBtnStop();
    afx_msg void OnBnClickedBtnIncGain();
    afx_msg void OnBnClickedBtnDecGain();
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
};
