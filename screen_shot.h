#ifndef __SCREEN_SHOT__
#define __SCREEN_SHOT__

#include "config.h"

#include <Windows.h>

#include "DisplayEsc.h"

#define MAP1 1030
#define UNMAP1 1031
#define CURSOREN 1060
#define CURSORDIS 1061

/*class ScreenShotIntf {
	virtual ~ScreenShot();
	virtual getData
};*/

int gdi_shot();
int d3dx_shot();
class screenshot_rdp/* : public ScreenShotIntf */{
	DISPLAY_DEVICE dev_;
	DEVMODE mode_;
	int x_,y_,w_,h_;
	BITMAPINFO	bmp_info_;
	HDC compat_dc_;
	HBITMAP	compat_bmp_;
	HBITMAP	compat_bmp_saved_;
	HDC dev_dc_;
public:
	screenshot_rdp();
	virtual ~screenshot_rdp();
	int set_rect(int x, int y, int w, int h);
	void set_cursor(bool show);
	int get_bmp_data(unsigned char *buf);
	int save_bmp_file(wchar_t *file_name, unsigned char *buf);
};

class screenshot_dfmirage {
public:

	DISPLAY_DEVICE dev_;
	DEVMODE mode_;
	int x_,y_,w_,h_;
	BITMAPINFO	bmp_info_;
	HDC compat_dc_;
	HBITMAP	compat_bmp_;
	HBITMAP	compat_bmp_saved_;
	HDC dev_dc_;
	GETCHANGESBUF buf_;

	screenshot_dfmirage();
	virtual ~screenshot_dfmirage();
	int set_rect(int x, int y, int w, int h);
	void set_cursor(bool show);
	int get_bmp_data(unsigned char *buf);
	int save_bmp_file(wchar_t *file_name, unsigned char *buf);
};



/*class ScreenShot : public ScreenShotIntf {
	ScreenShotIntf *imp = nullptr;
public:
	ScreenShot();
	virtual ~ScreenShot();
};*/

#endif
