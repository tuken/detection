#include "stdafx.h"
#include "detection.h"
#include "detectionDlg.h"
#include "afxdialogex.h"
#include <initguid.h>
#include <AyonixFaceID.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_IDEL (WM_USER + 1)

CDetectionDlg::CDetectionDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(CDetectionDlg::IDD, pParent)
, m_shot(0)
, m_once(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	hr = m_gb.CoCreateInstance(CLSID_FilterGraph);

	//int res = AFIDInitialize(TEXT("C:\\Program Files (x86)\\Ayonix\\FaceID\\data\\engine"), &m_engine);
	int res = AFIDInitialize(TEXT("data\\engine"), &m_engine);
	wchar_t msg[128] = { 0 };
	_snwprintf_s<128>(msg, _TRUNCATE, L"AFIDInitialize res[%d]\r\n", res);
	OutputDebugString(msg);

	memset(&m_mt, 0, sizeof(m_mt));

	m_dlg.Create(IDD_INFO_DIALOG, this);
}

void CDetectionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VIDEO, m_video);
}

BEGIN_MESSAGE_MAP(CDetectionDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

BOOL CDetectionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// SampleGrabber(Filter)�𐶐�
	CComPtr<ISampleGrabber> grabber;
	HRESULT hr = grabber.CoCreateInstance(CLSID_SampleGrabber);

	// Filter����ISampleGrabber�C���^�[�t�F�[�X���擾���܂�
	CComQIPtr<IBaseFilter> grbFilter = grabber;

	// SampleGrabber��ڑ�����t�H�[�}�b�g���w��B
	// �������|�C���g�ł��B
	// �����̎w��̎d���ɂ��SampleGrabber�̑}���ӏ�������ł��܂��B���̃T���v���̂悤�Ȏw��������
	// ��ʏo�͂̐��O�ŃT���v�����擾�ł��܂��B
	m_mt.majortype = MEDIATYPE_Video;
	m_mt.subtype = MEDIASUBTYPE_RGB24;
	m_mt.formattype = FORMAT_VideoInfo;
	hr = grabber->SetMediaType(&m_mt);

	// Graph��SampleGrabber Filter��ǉ�
	hr = m_gb->AddFilter(grbFilter, L"Sample Grabber");

	// CaptureGraphBuilder2�Ƃ����L���v�`���pGraphBuilder�𐶐�����
	CComPtr<ICaptureGraphBuilder2> capgb;
	hr = capgb.CoCreateInstance(CLSID_CaptureGraphBuilder2);

	// FilterGraph���Z�b�g����
	capgb->SetFiltergraph(m_gb);

	// MediaControl�C���^�[�t�F�[�X�擾
	CComQIPtr<IMediaControl> ctrl = m_gb;

	// �f�o�C�X��񋓂��邽�߂�CreateDevEnum�𐶐�
	CComPtr<ICreateDevEnum> devs;
	hr = devs.CoCreateInstance(CLSID_SystemDeviceEnum);

	// VideoInputDevice��񋓂��邽�߂�EnumMoniker�𐶐� 
	CComPtr<IEnumMoniker> mons;
	hr = devs->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &mons, 0);
	if (!mons) {
		// �ڑ����ꂽ�f�����̓f�o�C�X����������ꍇ�ɂ͂���if���ɓ���܂�
		MessageBox(L"�J������������܂���B");
		return TRUE;
	}

	// EnumMoniker��Reset���� Reset����ƁA�擪���琔���Ȃ����܂�
	hr = mons->Reset();

	// �ŏ���Moniker���擾
	CComPtr<IMoniker> moniker;
	ULONG nFetched = 0;
	hr = mons->Next(1, &moniker, &nFetched);

	// Monkier��Filter��Bind����
	CComPtr<IBaseFilter> devFilter;
	hr = moniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&devFilter);

	// FilterGraph�Ƀf�o�C�X�t�B���^��ǉ�����
	hr = m_gb->AddFilter(devFilter, L"Device Filter");

	CComPtr<IBaseFilter> rndFilter;
	hr = rndFilter.CoCreateInstance(CLSID_VideoRenderer);

	hr = m_gb->AddFilter(rndFilter, L"Video Renderer");

	// Graph�𐶐�����
	hr = capgb->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, devFilter, grbFilter, rndFilter);
	hr = grabber->GetConnectedMediaType(&m_mt);
	hr = grabber->SetBufferSamples(TRUE);
	hr = grabber->SetCallback(this, 1);

	CComQIPtr<IVideoWindow> win = rndFilter;
	hr = win->put_Owner((OAHWND)m_video.m_hWnd);
	hr = win->put_WindowStyle(WS_CHILD);

	CRect rc;
	m_video.GetClientRect(&rc);
	//cyBorder = GetSystemMetrics(SM_CYBORDER);
	//cy = 150 + cyBorder;
	//rc.bottom -= cy;

	hr = win->SetWindowPosition(0, 0, rc.Width(), rc.Height());
	hr = win->put_Visible(-1);

	// �Đ��J�n
	ctrl->Run();

	return TRUE;
}

void CDetectionDlg::OnPaint()
{
	if (IsIconic())	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
		CDialogEx::OnPaint();
}

HCURSOR CDetectionDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

STDMETHODIMP CDetectionDlg::SampleCB(double SampleTime, IMediaSample *pSample)
{
	wchar_t msg[128] = { 0 };
	_snwprintf_s<128>(msg, _TRUNCATE, L"SampleCB SampleTime[%.4f]\r\n", SampleTime);
	OutputDebugString(msg);
	return S_OK;
}

STDMETHODIMP CDetectionDlg::BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen)
{

	BITMAPFILEHEADER bfh = { 0 };
	VIDEOINFOHEADER *vif = reinterpret_cast<VIDEOINFOHEADER *>(m_mt.pbFormat);

	bfh.bfType = ('M' << 8) | 'B';
	bfh.bfSize = sizeof(bfh) + sizeof(BITMAPINFOHEADER) + BufferLen;
	bfh.bfOffBits = sizeof(bfh) + sizeof(BITMAPINFOHEADER);

	BYTE *buf = static_cast<BYTE *>(malloc(sizeof(bfh) + sizeof(BITMAPINFOHEADER) + BufferLen));
	memcpy(buf, &bfh, sizeof(bfh));
	memcpy(buf + sizeof(bfh), &vif->bmiHeader, sizeof(BITMAPINFOHEADER));
	memcpy(buf + sizeof(bfh) + sizeof(BITMAPINFOHEADER), pBuffer, BufferLen);

	wchar_t msg[128] = { 0 };
	AynxImage *img = NULL;
	int res = AFIDDecodeImage(buf, sizeof(bfh) + sizeof(BITMAPINFOHEADER) + BufferLen, &img);
	_snwprintf_s<128>(msg, _TRUNCATE, L"AFIDDecodeImage res[%d]\r\n", res);
	OutputDebugString(msg);
	if (res == AYNX_OK) {
		AynxFace *faces = NULL;
		size_t count = 0;
		res = AFIDDetectFaces(m_engine, img, &faces, &count, NULL);
		_snwprintf_s<128>(msg, _TRUNCATE, L"AFIDDetectFaces res[%d]\r\n", res);
		OutputDebugString(msg);
		if (res == AYNX_OK) {
			_snwprintf_s<128>(msg, _TRUNCATE, L"AFIDDetectFaces count[%d]\r\n", count);
			OutputDebugString(msg);
		}
	}

	free(buf);

	//DWORD nWritten;
	//HANDLE h = CreateFile(L"result.bmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	//WriteFile(h, &bfh, sizeof(bfh), &nWritten, NULL);
	//WriteFile(h, &vif->bmiHeader, sizeof(BITMAPINFOHEADER), &nWritten, NULL);
	//WriteFile(h, pBuffer, BufferLen, &nWritten, NULL);
	//CloseHandle(h);

	m_shot++;

	return S_OK;
}

void CDetectionDlg::OnClose()
{
	m_dlg.DestroyWindow();

	CComQIPtr<IMediaControl> ctrl = m_gb;
	ctrl->Stop();

	__super::OnClose();
}

void CDetectionDlg::OnIdle()
{
	CRect rect;
	GetWindowRect(&rect);
	//GetClientRect(&rect);
	//ClientToScreen(&rect);
	//ScreenToClient(&rect);
	m_dlg.ShowWindow(SW_SHOW);
	m_dlg.SetWindowPos(NULL, rect.right - 10, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

LRESULT CDetectionDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_IDEL)
		OnIdle();

	return __super::WindowProc(message, wParam, lParam);
}

void CDetectionDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	__super::OnShowWindow(bShow, nStatus);

	if (bShow && !m_once) {
		m_once = true;
		PostMessage(WM_IDEL, MAKEWPARAM(0, 0), NULL);
	}
}
