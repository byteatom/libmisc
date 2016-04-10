#include "screen_shot.h"
#include <exception>
#include <string>
#include <tchar.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "registry.h"

typedef TCHAR tchar;
typedef std::basic_string<tchar> tstring;

/*EnumDisplayDevices:
Enumerate all of current session's display device.

cb member of lpDisplayDevice  must be initialized.

To get information on the display adapter, 
call EnumDisplayDevices with lpDevice set to NULL.

To obtain information on a display monitor, 
first call EnumDisplayDevices with lpDevice set to NULL. 
Then call EnumDisplayDevices with lpDevice set to DISPLAY_DEVICE.DeviceName 
from the first call to EnumDisplayDevices and with iDevNum set to zero. 
Then DISPLAY_DEVICE.DeviceString is the monitor name.

To query all monitor devices associated with an adapter, 
call EnumDisplayDevices in a loop with lpDevice set to the adapter name, 
iDevNum set to start at 0, 
and iDevNum set to increment until the function fails. 
Note that DISPLAY_DEVICE.DeviceName changes with each call for monitor information, 
so you must save the adapter name. 
--------------------------------------------------------------------------------

EnumDisplaySettings:
Enumerate all of a display device's graphics modes.

dmSize and dmDriverExtra member of lpDevMode must be initialized.

A NULL lpszDeviceName pecifies the current display device on the calling thread
--------------------------------------------------------------------------------

ChangeDisplaySettingsEx:

A NULL lpszDeviceName specifies the default display device which can
be determined by  EnumDisplayDevices checking DISPLAY_DEVICE_PRIMARY_DEVICE  flag

A NULL lpDevMode means registry will be used for the display setting, with 0 for 
the dwFlags is the easiest way to return to the default mode. 

ChangeDisplaySettingsEx (lpszDeviceName1, lpDevMode1, NULL, (CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET), NULL);
ChangeDisplaySettingsEx (lpszDeviceName2, lpDevMode2, NULL, (CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET), NULL);
ChangeDisplaySettingsEx (NULL, NULL, NULL, 0, NULL);
*/

screenshot_rdp::screenshot_rdp() {

}

int screenshot_rdp::set_rect(int x, int y, int w, int h) {
	//find the specified driver
	tstring dev_str(_T("RDP Encoder Mirror Driver"));	
	ZeroMemory(&dev_, sizeof(dev_));
	dev_.cb = sizeof(dev_);	
	bool found = false;
	for (int i = 0; EnumDisplayDevices(NULL/*display adapter*/, i, &dev_, 0); i++)
	{
		if (dev_str == dev_.DeviceString) {
			/*ZeroMemory(&mode_, sizeof(mode_));
			mode_.dmSize = sizeof(mode_);
			mode_.dmDriverExtra = 0;
			if (EnumDisplaySettingsEx(dev_.DeviceName, ENUM_CURRENT_SETTINGS, &mode_, 0)) {
				found = true;
			}*/
			break;
		}
	}
	//if (!found) return 0;	
	
	DEVMODE mode;
	ZeroMemory(&mode, sizeof(mode));
	mode.dmSize = sizeof(mode);
	mode.dmDeviceName[0] = '\0';	
	mode.dmPelsWidth = w;
	mode.dmPelsHeight = h;
	mode.dmBitsPerPel = 24;
	mode.dmPosition.x = x;
	mode.dmPosition.y = y;
	mode.dmDriverExtra = 0/*mode_.dmDriverExtra*/;
	mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION;
	if (DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettingsEx(dev_.DeviceName, &mode,
		NULL, CDS_UPDATEREGISTRY, NULL))
		return 0;
	/*if (DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettingsEx(dev_.DeviceName, &mode, NULL, 0, NULL))
		return 0;*/
	
	x_ = x;
	y_ = y;
	w_ = (w + 3) & 0xFFFC;
	h_ = h;
	
	ZeroMemory(&bmp_info_, sizeof(bmp_info_));
	PBITMAPINFOHEADER bmp_info_head = (PBITMAPINFOHEADER)&(bmp_info_.bmiHeader);
	bmp_info_head->biSize = sizeof(*bmp_info_head);
	bmp_info_head->biWidth = w_;
	bmp_info_head->biHeight = -h_;
	bmp_info_head->biPlanes = 1;
	bmp_info_head->biBitCount = 24;
	bmp_info_head->biCompression = BI_RGB;
	bmp_info_head->biSizeImage = w_ * h_ * 3;

	//HWND desk_wnd = GetDesktopWindow();
	HDC desk_dc= ::GetDC(NULL);
	compat_dc_ = CreateCompatibleDC(NULL);
	compat_bmp_ = CreateCompatibleBitmap(desk_dc, w_, h_);
	compat_bmp_saved_ = (HBITMAP)SelectObject(compat_dc_, compat_bmp_);
	ReleaseDC(NULL, desk_dc);
	dev_dc_ = CreateDC(dev_.DeviceName, 0, 0, 0);

	return bmp_info_head->biSizeImage;
}

screenshot_rdp::~screenshot_rdp() {
	SelectObject(compat_dc_, compat_bmp_saved_);	
	DeleteDC(compat_dc_);
	DeleteObject(compat_bmp_);
	if (dev_dc_ != NULL) DeleteDC(dev_dc_);
	mode_.dmDeviceName[0] = 0;
	ChangeDisplaySettingsEx(dev_.DeviceName, NULL, NULL, 0, NULL);
}

void screenshot_rdp:: set_cursor(bool show)
{
 	HDC gdc = GetDC(NULL);
	if (show) {
	 	ExtEscape(gdc, MAP1, 0, NULL, NULL, NULL);
	 	ExtEscape(gdc, CURSOREN, 0, NULL, NULL, NULL);
	} else {
		ExtEscape(gdc, CURSORDIS, 0, NULL, NULL, NULL);
	}
	ReleaseDC(NULL, gdc);
}

int screenshot_rdp::get_bmp_data(unsigned char *buf) {
	BitBlt(compat_dc_, 0, 0, w_, h_, dev_dc_, x_, y_, SRCCOPY | CAPTUREBLT);
	return GetDIBits(compat_dc_, compat_bmp_, 0, h_, buf, &bmp_info_, DIB_RGB_COLORS);
}

int screenshot_rdp::save_bmp_file(tchar *file_name, unsigned char *buf) {
	HANDLE file = CreateFile(file_name,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	
	BITMAPFILEHEADER bmp_file_head;
	PBITMAPINFOHEADER bmp_info_head = (PBITMAPINFOHEADER)&(bmp_info_.bmiHeader);

	bmp_file_head.bfType = 0x4d42;	
	bmp_file_head.bfOffBits = sizeof(bmp_file_head) + bmp_info_head->biSize 
		+ bmp_info_head->biClrUsed * sizeof (bmp_info_.bmiColors);
	bmp_file_head.bfSize = bmp_file_head.bfOffBits + bmp_info_head->biSizeImage;
	bmp_file_head.bfReserved1 = 0;
	bmp_file_head.bfReserved2 = 0;
	
	DWORD writed = 0;
	WriteFile(file, &bmp_file_head, sizeof(bmp_file_head), &writed, NULL);
	WriteFile(file, bmp_info_head, 
		sizeof(*bmp_info_head) + bmp_info_head->biClrUsed * sizeof(bmp_info_.bmiColors),
		&writed, NULL);
	WriteFile(file, buf, bmp_info_head->biSizeImage, &writed, NULL);
	CloseHandle(file);
	return 0;
}


screenshot_dfmirage::screenshot_dfmirage() {

}

int screenshot_dfmirage::set_rect(int x, int y, int w, int h) {
	
	//find the specified driver
	tstring dev_str(_T("Mirage Driver"));	
	ZeroMemory(&dev_, sizeof(dev_));
	dev_.cb = sizeof(dev_);	
	bool found = false;
	for (int i = 0; EnumDisplayDevices(NULL/*display adapter*/, i, &dev_, 0); i++)
	{
		if (dev_str == dev_.DeviceString) {
			found = true;
			break;
		}
	}	
	if (!found) return 0;/*goto error;*/

	
	//Registry reg(HKEY_LOCAL_MACHINE, dev_.DeviceKey);
	tchar devNum[] = _T("DEVICE0");
	_tcsupr(dev_.DeviceKey);
	tchar *devNumPos = _tcsstr(dev_.DeviceKey, _T("\\DEVICE"));
	if (devNumPos)
		_tcscpy_s(devNum, sizeof(devNum), devNumPos + 1);
	Registry regDriver(HKEY_LOCAL_MACHINE, 
		_T("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\dfmirage"), 
		true);
	Registry regDevice(regDriver.key, devNum, true);
	DWORD value = 1;
	regDevice.SetValue(_T("Attach.ToDesktop"), REG_DWORD, (const BYTE *)&value, sizeof(value));

	/*HDESK   hdeskInput;
    HDESK   hdeskCurrent;
	hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
	if (hdeskCurrent == NULL) goto error;
	hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
	if (hdeskInput == NULL) goto error;
	SetThreadDesktop(hdeskInput);*/

	
	ZeroMemory(&mode_, sizeof(mode_));
	mode_.dmSize = sizeof(mode_);
	mode_.dmDeviceName[0] = '\0';	
	mode_.dmPelsWidth = w;
	mode_.dmPelsHeight = h;
	mode_.dmBitsPerPel = 24;
	mode_.dmPosition.x = x;
	mode_.dmPosition.y = y;
	mode_.dmDriverExtra = 0;
	mode_.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION;
	if (DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettingsEx(dev_.DeviceName, &mode_,
		NULL, CDS_UPDATEREGISTRY /*| CDS_RESET | CDS_GLOBAL*/, NULL))
		return 0;/*goto error;*/
	if (DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettingsEx(dev_.DeviceName, &mode_,
		NULL, 0, NULL))
		return 0;/* goto error;*/

	dev_dc_ = CreateDC(dev_.DeviceName, 0, 0, 0);
	
    if (0 >= ExtEscape(dev_dc_, dmf_esc_usm_pipe_map, 0, 0, sizeof(buf_), (LPSTR)&buf_)) {
      return 0;
    }
	
	/*SetThreadDesktop(hdeskCurrent);
	CloseDesktop(hdeskInput);*/
	
	x_ = x;
	y_ = y;
	w_ = w;
	h_ = h;
	
	ZeroMemory(&bmp_info_, sizeof(bmp_info_));
	PBITMAPINFOHEADER bmp_info_head = (PBITMAPINFOHEADER)&(bmp_info_.bmiHeader);
	bmp_info_head->biSize = sizeof(*bmp_info_head);
	bmp_info_head->biWidth = w_;
	bmp_info_head->biHeight = -h_;
	bmp_info_head->biPlanes = 1;
	bmp_info_head->biBitCount = 24;
	bmp_info_head->biCompression = BI_RGB;
	/*bmp_info_head->biSizeImage = ((((w_ * 24) + 31) >> 5) << 2) * h_;
	bmp_info_head->biSizeImage = ((((w_ * 24) + 31) & ~31) / 8) * h_;
	bmp_info_head->biSizeImage = ((((w_ * 24) + 31) / 32 ) *4) * h_;*/
	bmp_info_head->biSizeImage = ((w_ * (24 >> 3) + 3) & -4) * h_;

	//HWND desk_wnd = GetDesktopWindow();
	/*HDC desk_dc= ::GetDC(NULL);
	compat_dc_ = CreateCompatibleDC(NULL);
	compat_bmp_ = CreateCompatibleBitmap(desk_dc, w_, h_);
	compat_bmp_saved_ = (HBITMAP)SelectObject(compat_dc_, compat_bmp_);
	ReleaseDC(NULL, desk_dc);*/
	

	return bmp_info_head->biSizeImage;
	
/*error:
	if (NULL != hdeskCurrent) SetThreadDesktop(hdeskCurrent);
	if (NULL != hdeskInput) CloseDesktop(hdeskInput);
	return 0;*/
}

screenshot_dfmirage::~screenshot_dfmirage() {
	SelectObject(compat_dc_, compat_bmp_saved_);	
	DeleteDC(compat_dc_);
	DeleteObject(compat_bmp_);

	ExtEscape(dev_dc_, dmf_esc_usm_pipe_unmap, sizeof(buf_), (LPSTR)&buf_, 0, 0);
	
	if (dev_dc_ != NULL) DeleteDC(dev_dc_);
	
	tchar devNum[] = _T("DEVICE0");
	_tcsupr(dev_.DeviceKey);
	tchar *devNumPos = _tcsstr(dev_.DeviceKey, _T("\\DEVICE"));
	if (devNumPos)
		_tcscpy_s(devNum, sizeof(devNum), devNumPos + 1);
	Registry regDriver(HKEY_LOCAL_MACHINE, 
		_T("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\dfmirage"), 
		true);
	Registry regDevice(regDriver.key, devNum, true);
	DWORD value = 0;
	regDevice.SetValue(_T("Attach.ToDesktop"), REG_DWORD, (const BYTE *)&value, sizeof(value));

	mode_.dmDeviceName[0] = 0;
	mode_.dmPelsWidth = 0;
    mode_.dmPelsHeight = 0;
	ChangeDisplaySettingsEx(dev_.DeviceName, &mode_,
		NULL, CDS_UPDATEREGISTRY /*| CDS_RESET | CDS_GLOBAL*/, NULL);
	ChangeDisplaySettingsEx(dev_.DeviceName, &mode_,
		NULL, 0, NULL);
}

void screenshot_dfmirage:: set_cursor(bool show)
{
 	HDC gdc = GetDC(NULL);
	if (show) {
	 	ExtEscape(gdc, MAP1, 0, NULL, NULL, NULL);
	 	ExtEscape(gdc, CURSOREN, 0, NULL, NULL, NULL);
	} else {
		ExtEscape(gdc, CURSORDIS, 0, NULL, NULL, NULL);
	}
	ReleaseDC(NULL, gdc);
}

int screenshot_dfmirage::get_bmp_data(unsigned char *buf) {
	PBITMAPINFOHEADER bmp_info_head = (PBITMAPINFOHEADER)&(bmp_info_.bmiHeader);
	unsigned char *temp = (unsigned char *)(buf_.Userbuffer) + bmp_info_head->biSizeImage;

	return 0;
}

int screenshot_dfmirage::save_bmp_file(tchar *file_name, unsigned char *buf) {
	HANDLE file = CreateFile(file_name,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	
	BITMAPFILEHEADER bmp_file_head;
	PBITMAPINFOHEADER bmp_info_head = (PBITMAPINFOHEADER)&(bmp_info_.bmiHeader);

	bmp_file_head.bfType = 0x4d42;	
	bmp_file_head.bfOffBits = sizeof(bmp_file_head) + bmp_info_head->biSize 
		+ bmp_info_head->biClrUsed * sizeof (bmp_info_.bmiColors);
	bmp_file_head.bfSize = bmp_file_head.bfOffBits + bmp_info_head->biSizeImage;
	bmp_file_head.bfReserved1 = 0;
	bmp_file_head.bfReserved2 = 0;
	
	DWORD writed = 0;
	WriteFile(file, &bmp_file_head, sizeof(bmp_file_head), &writed, NULL);
	WriteFile(file, bmp_info_head, 
		sizeof(*bmp_info_head) + bmp_info_head->biClrUsed * sizeof(bmp_info_.bmiColors),
		&writed, NULL);
	WriteFile(file, buf, bmp_info_head->biSizeImage, &writed, NULL);
	CloseHandle(file);
	return 0;
}
int gdi_shot()
{
	HDC hdcScreen;
	int width, height;

	HDC hdcMem = NULL;
	HBITMAP hbmpMem = NULL;
	BITMAP bmpMem;

	hdcScreen = GetDC(NULL);
	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	hdcMem = CreateCompatibleDC(hdcScreen);
	hbmpMem = CreateCompatibleBitmap(hdcScreen, width, height);
	SelectObject(hdcMem, hbmpMem);

	BitBlt(hdcMem,
		0, 0,
		width, height,
		hdcScreen,
		0, 0,
		SRCCOPY);

	GetObject(hbmpMem, sizeof(BITMAP), &bmpMem);

	BITMAPFILEHEADER   bmpFileHeader;
	BITMAPINFOHEADER   bmpInfoHeader;

	bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfoHeader.biWidth = bmpMem.bmWidth;
	bmpInfoHeader.biHeight = bmpMem.bmHeight;
	bmpInfoHeader.biPlanes = 1;
	bmpInfoHeader.biBitCount = 32;
	bmpInfoHeader.biCompression = BI_RGB;
	bmpInfoHeader.biSizeImage = 0;
	bmpInfoHeader.biXPelsPerMeter = 0;
	bmpInfoHeader.biYPelsPerMeter = 0;
	bmpInfoHeader.biClrUsed = 0;
	bmpInfoHeader.biClrImportant = 0;

	DWORD dwBmpSize = ((bmpMem.bmWidth * bmpInfoHeader.biBitCount + 31) / 32) * 4 * bmpMem.bmHeight;

	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	char *lpbitmap = (char *)GlobalLock(hDIB);

	GetDIBits(hdcMem, hbmpMem, 0, (UINT)bmpMem.bmHeight,
		lpbitmap, (BITMAPINFO *)&bmpInfoHeader, DIB_RGB_COLORS);

	HANDLE hFile = CreateFile(L"test_gdi.bmp",
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);

	bmpFileHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER)+(DWORD)sizeof(BITMAPINFOHEADER);

	bmpFileHeader.bfSize = dwSizeofDIB;

	bmpFileHeader.bfType = 0x4D42; //BM   

	DWORD dwBytesWritten = 0;
	WriteFile(hFile, (LPSTR)&bmpFileHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)&bmpInfoHeader, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

	GlobalUnlock(hDIB);
	GlobalFree(hDIB);

	CloseHandle(hFile);

done:
	DeleteObject(hbmpMem);
	DeleteObject(hdcMem);
	ReleaseDC(NULL, hdcScreen);

	return 0;
}


int d3dx_shot() {
	IDirect3D9*			g_pD3D = NULL;
	IDirect3DDevice9* g_pd3dDevice;
	IDirect3DSurface9* g_pSurface;
	void* pBits;
	
	g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS PresentParams;
	memset(&PresentParams, 0, sizeof(D3DPRESENT_PARAMETERS));
	PresentParams.Windowed = TRUE;
	PresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDesktopWindow(), 
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &PresentParams, &g_pd3dDevice);

    g_pd3dDevice->CreateOffscreenPlainSurface(1366, 768,
		D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &g_pSurface, NULL);
	g_pd3dDevice->GetFrontBufferData(0, g_pSurface);

	D3DLOCKED_RECT lockedRect;
	g_pSurface->LockRect(&lockedRect, NULL,
	                   D3DLOCK_NO_DIRTY_UPDATE|
	                   D3DLOCK_NOSYSLOCK|D3DLOCK_READONLY);
	
	BITMAPINFOHEADER bmp_info_head;
	ZeroMemory(&bmp_info_head, sizeof(bmp_info_head));
	bmp_info_head.biSize = sizeof(bmp_info_head);
	bmp_info_head.biWidth = 1366;
	bmp_info_head.biHeight = -768;
	bmp_info_head.biPlanes = 1;
	bmp_info_head.biBitCount = 32;
	bmp_info_head.biCompression = BI_RGB;
	/*bmp_info_head->biSizeImage = ((((w_ * 24) + 31) >> 5) << 2) * h_;
	bmp_info_head->biSizeImage = ((((w_ * 24) + 31) & ~31) / 8) * h_;
	bmp_info_head->biSizeImage = ((((w_ * 24) + 31) / 32 ) *4) * h_;*/
	bmp_info_head.biSizeImage = ((1366 * (32 >> 3) + 3) & -4) * 768;

	HANDLE file = CreateFile(_T("test_d3d.bmp"),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	
	BITMAPFILEHEADER bmp_file_head;	
	bmp_file_head.bfType = 0x4d42;	
	bmp_file_head.bfOffBits = sizeof(bmp_file_head) + bmp_info_head.biSize;
	bmp_file_head.bfSize = bmp_file_head.bfOffBits + bmp_info_head.biSizeImage;
	bmp_file_head.bfReserved1 = 0;
	bmp_file_head.bfReserved2 = 0;
	
	DWORD writed = 0;
	WriteFile(file, &bmp_file_head, sizeof(bmp_file_head), &writed, NULL);
	WriteFile(file, &bmp_info_head, sizeof(bmp_info_head), &writed, NULL);
	WriteFile(file, lockedRect.pBits, bmp_info_head.biSizeImage, &writed, NULL);
	CloseHandle(file);

	
	//D3DXSaveSurfaceToFile(_T("test_d3dx.bmp"), D3DXIFF_BMP, g_pSurface, NULL, NULL);
	
	g_pSurface->UnlockRect();
	
	g_pSurface->Release();
	g_pd3dDevice->Release();
	g_pD3D->Release();

	return 0;
}

/*ScreenShot::ScreenShot() {
	DISPLAY_DEVICE device;	
	ZeroMemory(&device, sizeof(device));
	device.cb = sizeof(device);
	int n = 0;
	tstring nameRdp(_T("RDP Encoder Mirror Driver"));
	
	for (int i = 0; EnumDisplayDevicesW(NULL, i, &device, EDD_GET_DEVICE_INTERFACE_NAME); i++)
	{
		if (nameRdp == device.DeviceString) {
			imp = new ScreenShotRdp(device);
			break;
		}
	}
	if (nullptr == imp) {
		//uvnc
	}
	if (nullptr == imp) throw system_error("no mirror driver");
	return;
}

ScreenShot::~ScreenShot() {
	if (nullptr != imp) delete imp;
}*/

