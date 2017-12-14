/* Copyright 2017 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

// StyphoonTestControlDlg.h : header file
//

#pragma once

#include "DeviceController.h"
#include "afxwin.h"

#define AJA_STATUS_UPDATE (WM_APP + 1)

// CStyphoonTestControlDlg dialog
class CStyphoonTestControlDlg : public CDialogEx
{
// Construction
public:
	CStyphoonTestControlDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_STYPHOONTESTCONTROL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

    LRESULT OnStatusUpdate(WPARAM w, LPARAM l);

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
    afx_msg void OnStnClickedCaptureFrames();
    afx_msg void OnBnClickedButtonStartCapture();
    afx_msg void OnBnClickedButtonStartPlayback();

private:

    static void UpdateCallback(void* context);
    void UpdateCallback();

    DeviceController control;
public:
    UINT captureDevice;
    afx_msg void OnEnChangeEditCapChannel();
    UINT captureChannel;
    UINT playbackDevice;
    UINT playbackChannel;
    UINT capFramesReceived;
    UINT capBytesPerFrame;
    UINT capFramesAvailable;
    UINT pbFramesSent;
    UINT pbBytesPerFrame;
    UINT pcFramesAvailable;
    CString capStatus;
    CString pbStatus;
    afx_msg void OnBnClickedCheck1();
    BOOL routeFrames;
    afx_msg void OnCbnSelchangeCombo1();
    afx_msg void OnBnClickedButtonSelectAudioFile();
    afx_msg void OnBnClickedButtonSelectVideoFile();
private:
    CComboBox ctrlVideoFormat;
public:
    CButton btnCapture;
    CButton btnPlayback;
private:
    BOOL compressedVideo;
};
