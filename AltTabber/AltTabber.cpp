// AltTabber.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "AltTabber.h"
#include <Dwmapi.h>
#include <cstdio>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <Windowsx.h>
#include <sstream>
#include <Psapi.h>

#define MAX_LOADSTRING 100

typedef struct {
    HTHUMBNAIL thumb;
    HWND hwnd;
} AppThumb_t;

typedef struct {
    HWND hwnd;
    RECT r;
    int moveUpDownAmount;
} SlotThing_t;

struct {
    BOOL showing;
    HWND prevActiveWindow;
    signed long activeSlot;
    BOOL logging;
    BOOL logFileExists;
    HWND hWnd;
    std::map<HMONITOR, std::vector<AppThumb_t> > thumbnails;
    std::vector<SlotThing_t> slots;
    std::wstring filter;
    BOOL ignoreNext;
} g_programState = {
    FALSE,
    NULL,
    -1,
    FALSE,
    FALSE,
};

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

inline void log(LPTSTR fmt, ...)
{
    if(!g_programState.logging) return;

    if(!g_programState.logFileExists) {
        // replace stdout with log file
        TCHAR tempPath[MAX_PATH + 1];
        auto hr = GetTempPath(MAX_PATH, tempPath);
        if(hr == 0) abort();

        TCHAR tempFile[MAX_PATH + 1];
        UINT hrTFN = GetTempFileName(tempPath, _T("altTabber"), 0, tempFile);
        if(hrTFN == 0 || hrTFN == ERROR_BUFFER_OVERFLOW) abort();

        _tfreopen(tempFile, _T("w"), stdout);

        g_programState.logFileExists = TRUE;
    }

    va_list ap;
    va_start(ap, fmt);

    _vwprintf_p(fmt, ap);
    fflush(stdout);

    va_end(ap);
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_ALTTABBER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALTTABBER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALTTABBER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)0;//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL; //MAKEINTRESOURCE(IDC_ALTTABBER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

typedef struct {
    HMONITOR hMonitor;
    RECT extent;
} MonitorInfo_t;

typedef struct {
    RECT r;
    std::vector<MonitorInfo_t> monitors;
} MonitorGeom_t;

BOOL CALLBACK monitorEnumProc(
    HMONITOR hMonitor,
    HDC hdcMonitor,
    LPRECT lprcMonitor,
    LPARAM dwData)
{
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    mi.dwFlags = 0;
    auto hr = GetMonitorInfo(hMonitor, &mi);
    log(_T("Get monitor info: %d; %ld %ld %ld %ld\n"), hr, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom);
    if(!hr) {
        log(_T("\tlast error: %d\n"), GetLastError());
    }

    MonitorInfo_t mit = { hMonitor, mi.rcMonitor };

    std::vector<MonitorInfo_t>& stuff = *((std::vector<MonitorInfo_t>*)dwData);
    stuff.push_back(mit);
    return TRUE;
}

MonitorGeom_t GetMonitorGeometry()
{
    MonitorGeom_t ret = { { MAXLONG, MAXLONG, -MAXLONG, -MAXLONG } };
    std::vector<MonitorInfo_t> stuff;

    EnumDisplayMonitors(NULL, NULL, monitorEnumProc, (LPARAM)&stuff);

    ret.monitors = stuff;

    for(auto&& i = stuff.begin(); i != stuff.end(); ++i) {
        if(i->extent.left < ret.r.left) ret.r.left = i->extent.left;
        if(i->extent.top < ret.r.top) ret.r.top = i->extent.top;
        if(i->extent.right > ret.r.right) ret.r.right = i->extent.right;
        if(i->extent.bottom > ret.r.bottom) ret.r.bottom = i->extent.bottom;
    }

    std::sort(ret.monitors.begin(), ret.monitors.end(), [](std::vector<MonitorInfo_t>::value_type const& left, std::vector<MonitorInfo_t>::value_type const& right) -> bool {
        if(left.extent.top == right.extent.top) return left.extent.left < right.extent.left;
        return left.extent.top < right.extent.top;
    });

    return ret;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   auto geom = GetMonitorGeometry();
   // SetWindowPos(hWnd, HWND_TOPMOST, geom.x, geom.y, geom.cx, geom.cy);

   hWnd = CreateWindowEx(WS_EX_TOPMOST, szWindowClass, szTitle, 
      WS_POPUP,
      CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, SW_HIDE);
   UpdateWindow(hWnd);

   if(RegisterHotKey(
    	        hWnd,
    	        1,
            MOD_ALT | MOD_CONTROL,
    	        '3'))
   {
        log(_T("win+caps registered\n"));
   }

   g_programState.hWnd = hWnd;

   return TRUE;
}

inline BOOL IsAltTabWindow(HWND hwnd)
{
    TITLEBARINFO ti;
    HWND hwndTry, hwndWalk = NULL;

    if(!IsWindowVisible(hwnd))
        return FALSE;

    hwndTry = GetAncestor(hwnd, GA_ROOTOWNER);
    while(hwndTry != hwndWalk) 
    {
        hwndWalk = hwndTry;
        hwndTry = GetLastActivePopup(hwndWalk);
        if(IsWindowVisible(hwndTry)) 
            break;
    }
    if(hwndWalk != hwnd)
        return FALSE;

    // the following removes some task tray programs and "Program Manager"
    ti.cbSize = sizeof(ti);
    GetTitleBarInfo(hwnd, &ti);
    if(ti.rgstate[0] & STATE_SYSTEM_INVISIBLE)
        return FALSE;

    // Tool windows should not be displayed either, these do not appear in the
    // task bar.
    if(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
        return FALSE;

    return TRUE;
}

BOOL CALLBACK enumWindows(HWND hwnd, LPARAM lParam)
{
    if(hwnd == g_programState.hWnd) return TRUE;
    if(!IsAltTabWindow(hwnd)) return TRUE;

    std::wstring filter = *((std::wstring const*)lParam);

    TCHAR str[257];
    TCHAR str2[257];
    GetWindowText(hwnd, str, 256);
    std::wstring title(str);

#if 0
    DWORD procId;
    if(GetWindowThreadProcessId(hwnd, &procId) > 0) {
        auto hnd = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procId);
        if(hnd != NULL) {
            if(GetModuleFileNameEx(hnd, NULL, str2, 256) > 0) {
                std::wstringstream wss;
                wss << std::wstring(str2) << std::wstring(L" // ") << title;
                title = wss.str();
            }
            CloseHandle(hnd);
        }
    }
#endif

    std::transform(title.begin(), title.end(), title.begin(), ::tolower);
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
    if(!filter.empty() && title.find(filter) == std::wstring::npos) {
        return TRUE;
    }

    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    HTHUMBNAIL hThumb = NULL;
    auto hr = DwmRegisterThumbnail(g_programState.hWnd, hwnd, &hThumb);
    log(_T("register thumbnail for %p on monitor %p: %d\n"), (void*)hwnd, (void*)hMonitor, hr);
    AppThumb_t at = {
        hThumb,
        hwnd
    };
    if(hr == S_OK) g_programState.thumbnails[hMonitor].push_back(at);

    return TRUE;
}

void CreateThumbnails(std::wstring const& filter)
{
    for(auto i = g_programState.thumbnails.begin(); i != g_programState.thumbnails.end(); ++i) {
        for(auto j = i->second.begin(); j != i->second.end(); ++j) {
            DwmUnregisterThumbnail(j->thumb);
        }
    }
    g_programState.thumbnails.clear();

    auto hr = EnumWindows(enumWindows, (LPARAM)&filter);
    log(_T("enum desktop windows: %d\n"), hr);
}

void SetThumbnails()
{
    g_programState.activeSlot = -1;
    g_programState.slots.clear();
    //ShowWindow(g_programState.hWnd, SW_HIDE);
    size_t nthSlot = 0;
    auto mis = GetMonitorGeometry();
    for(size_t i = 0; i < mis.monitors.size(); ++i) {
        auto& mi = mis.monitors[i];
        auto& thumbs = g_programState.thumbnails[mi.hMonitor];
        auto nWindows = thumbs.size();

        unsigned int nTiles = 1;
        while(nTiles < nWindows) nTiles <<= 1;

        long lala = (long)sqrt((double)nTiles) + 1;

        long l1 = lala;
        long l2 = lala;
        while((l1 - 1) * l2 > nWindows) l1--;
        lala = l1 * l2;

        long ws = (mi.extent.right - mi.extent.left) / l1;
        long hs = (mi.extent.bottom - mi.extent.top) / l2;

        for(size_t j = 0; j < thumbs.size(); ++j) {
            long x = (j % l1) * ws + 3;
            long y = (j / l1) * hs + hs / 3;
            long x1 = x + ws - 3;
            long y1 = y + hs - hs / 3;
            RECT r;
            r.left = mi.extent.left - mis.r.left + x;
            r.right = mi.extent.left - mis.r.left + x1;
            r.top = mi.extent.top - mis.r.top + y;
            r.bottom = mi.extent.top - mis.r.top + y1;

            DWM_THUMBNAIL_PROPERTIES thProps;
            thProps.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE;
            thProps.rcDestination = r;
            thProps.fVisible = TRUE;
            DwmUpdateThumbnailProperties(thumbs[j].thumb, &thProps);

            if(thumbs[j].hwnd == g_programState.prevActiveWindow) g_programState.activeSlot = nthSlot;

            SlotThing_t st;
            st.hwnd = thumbs[j].hwnd;
            st.r.left = mi.extent.left - mis.r.left + (j % l1) * ws;
            st.r.top = mi.extent.top - mis.r.top + (j / l1) * hs;
            st.r.right = st.r.left + ws;
            st.r.bottom = st.r.top + hs;
            st.moveUpDownAmount = l1;
            g_programState.slots.push_back(st);

            ++nthSlot;
        }
    }
}

void Cleanup()
{
    for(auto i = g_programState.thumbnails.begin(); i != g_programState.thumbnails.end(); ++i) {
        for(auto j = i->second.begin(); j != i->second.end(); ++j) {
            DwmUnregisterThumbnail(j->thumb);
        }
    }
    g_programState.thumbnails.clear();
    
    if(g_programState.logFileExists) fclose(stdout);
}

void OnPaint(HDC hdc)
{
    HGDIOBJ original = NULL;
    original = SelectObject(hdc, GetStockObject(DC_PEN));
    HPEN pen = CreatePen(PS_SOLID, 5, RGB(0, 255, 0));
    auto originalBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));
    SelectObject(hdc, GetStockObject(DC_BRUSH));

    HFONT font = CreateFont(0, 9, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
        DEFAULT_PITCH | FF_DONTCARE, _T("Courier New"));
    HFONT originalFont = (HFONT)SelectObject(hdc, font);
    
    auto mis = GetMonitorGeometry();
    SetDCBrushColor(hdc, RGB(0, 0, 0));
    Rectangle(hdc, 0, 0, mis.r.right - mis.r.left, mis.r.bottom - mis.r.top);

    SetDCBrushColor(hdc, RGB(255, 0, 0));

    //for(size_t i = 0; i < g_programState.slots.size(); ++i) {
    //    RECT r = (g_programState.slots[i].r);
    //    Rectangle(hdc, r.left, r.top, r.right, r.bottom);
    //}
    if(g_programState.activeSlot >= 0) {
        RECT r = (g_programState.slots[g_programState.activeSlot]).r;
        Rectangle(hdc, r.left, r.top, r.right, r.bottom);
    }

    SelectObject(hdc, GetStockObject(BLACK_BRUSH));

    for(size_t i = 0; i < mis.monitors.size(); ++i) {
        auto& mi = mis.monitors[i];
        auto& thumbs = g_programState.thumbnails[mi.hMonitor];
        auto nWindows = thumbs.size();

        unsigned int nTiles = 1;
        while(nTiles < nWindows) nTiles <<= 1;

        long lala = (long)sqrt((double)nTiles) + 1;

        long l1 = lala;
        long l2 = lala;
        while((l1 - 1) * l2 > nWindows) l1--;
        lala = l1 * l2;

        long ws = (mi.extent.right - mi.extent.left) / l1;
        long hs = (mi.extent.bottom - mi.extent.top) / l2;

        for(size_t j = 0; j < thumbs.size(); ++j) {
            long x = (j % l1) * ws + 3;
            long y = (j / l1) * hs + 3;
            long x1 = x + ws - 3;
            long y1 = y + hs - 2 * hs / 3;
            RECT r;
            r.left = mi.extent.left - mis.r.left + x;
            r.right = mi.extent.left - mis.r.left + x1;
            r.top = mi.extent.top - mis.r.top + y;
            r.bottom = mi.extent.top - mis.r.top + y1;

            HWND hwnd = thumbs[j].hwnd;
            
            TCHAR str[257];
            TCHAR str2[257];
            GetWindowText(hwnd, str, 256);
            std::wstring title(str);
#if 0
            DWORD procId;
            if(GetWindowThreadProcessId(hwnd, &procId) > 0) {
                auto hnd = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procId);
                if(hnd != NULL) {
                    if(GetModuleFileNameEx(hnd, NULL, str2, 256) > 0) {
                        std::wstringstream wss;
                        wss << std::wstring(str2) << std::wstring(L" // ") << title;
                        title = wss.str();
                    }
                    CloseHandle(hnd);
                }
            }
#endif

            DrawText(hdc, str, -1, &r, DT_BOTTOM | DT_LEFT | DT_WORDBREAK);
        }
    }

    SelectObject(hdc, originalFont);
    SelectObject(hdc, originalBrush);
    SelectObject(hdc, original);
}

void MoveNext(DWORD direction)
{
    if(g_programState.activeSlot < 0) {
        if(g_programState.slots.size() > 0) {
            g_programState.activeSlot = 0;
        } else {
            return;
        }
    }
    log(_T("Moving from %ld "), g_programState.activeSlot);
    switch(direction)
    {
    case VK_RIGHT:
        g_programState.activeSlot++;
        log(_T("by %d\n"), 1);
        break;
    case VK_LEFT:
        g_programState.activeSlot--;
        log(_T("by %d\n"), -1);
        break;
    case VK_UP:
        log(_T("by %d\n"), -g_programState.slots[g_programState.activeSlot].moveUpDownAmount);
        g_programState.activeSlot -= g_programState.slots[g_programState.activeSlot].moveUpDownAmount;
        break;
    case VK_DOWN:
        log(_T("by %d\n"), g_programState.slots[g_programState.activeSlot].moveUpDownAmount);
        g_programState.activeSlot += g_programState.slots[g_programState.activeSlot].moveUpDownAmount;
        break;
    }
    if(g_programState.activeSlot < 0) g_programState.activeSlot = g_programState.slots.size() + g_programState.activeSlot;
    g_programState.activeSlot %= g_programState.slots.size();

    if(g_programState.activeSlot >= 0) {
        log(_T("Current active slot: %ld hwnd: %p\n"), g_programState.activeSlot, (void*)g_programState.slots[g_programState.activeSlot].hwnd);
    }
    RedrawWindow(g_programState.hWnd, NULL, NULL, RDW_INVALIDATE);
}

void QuitOverlay()
{
    log(_T("escape pressed; reverting\n"));
    g_programState.showing = FALSE;
    auto monitorGeom = GetMonitorGeometry();
    SetWindowPos(g_programState.hWnd, HWND_TOPMOST, monitorGeom.r.right, monitorGeom.r.top, monitorGeom.r.right + 1, monitorGeom.r.bottom, SWP_HIDEWINDOW);
    if(g_programState.prevActiveWindow) {
        auto hr = SetForegroundWindow(g_programState.prevActiveWindow);
        log(_T("set foreground window to previous result: %d\n"), hr);
    }
}

void SelectCurrent()
{
    if(g_programState.activeSlot >= 0) {
        g_programState.prevActiveWindow = g_programState.slots[g_programState.activeSlot].hwnd;
        QuitOverlay();
    }
}

void SelectByMouse(DWORD lParam)
{
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);
    POINT pt = { xPos, yPos };
    auto found = std::find_if(g_programState.slots.begin(), g_programState.slots.end(), [&](SlotThing_t const& thing) -> bool {
        return PtInRect(&thing.r, pt);
    });
    if(found != g_programState.slots.end()) {
        g_programState.activeSlot = found - g_programState.slots.begin();
        RedrawWindow(g_programState.hWnd, NULL, NULL, RDW_INVALIDATE);
    }
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT: {
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
        if(g_programState.showing)
        {
            HGDIOBJ original = NULL;
            original = SelectObject(hdc, GetStockObject(DC_PEN));
            HPEN pen = CreatePen(PS_SOLID, 5, RGB(0, 255, 0));
            SelectObject(hdc, pen);
            MoveToEx(hdc, 0, 0, NULL);
            auto mi = GetMonitorGeometry();
            LineTo(hdc, mi.r.right - mi.r.left, mi.r.bottom - mi.r.top);
            SelectObject(hdc, original);

            OnPaint(hdc);
        }
		EndPaint(hWnd, &ps);
        break; }
	case WM_DESTROY:
        Cleanup();
		PostQuitMessage(0);
		break;
    case WM_HOTKEY:
        log(_T("hotkey pressed\n"));
        if(g_programState.showing) {
            SelectCurrent();
        } else {
            log(_T("activating switcher\n"));
            g_programState.prevActiveWindow = GetForegroundWindow();
            log(_T("previous window is %p\n"), (void*)g_programState.prevActiveWindow);
            g_programState.showing = TRUE;
            auto monitorGeom = GetMonitorGeometry();
            SetWindowPos(hWnd, HWND_TOPMOST, monitorGeom.r.left, monitorGeom.r.top, monitorGeom.r.right - monitorGeom.r.left, monitorGeom.r.bottom - monitorGeom.r.top, SWP_SHOWWINDOW);
            //SetWindowPos(hWnd, HWND_TOPMOST, monitorGeom.r.left, monitorGeom.r.top, monitorGeom.r.right - monitorGeom.r.left, 100, SWP_SHOWWINDOW);
            SetForegroundWindow(hWnd);
            SetFocus(hWnd);

            g_programState.filter = _T("");
            CreateThumbnails(g_programState.filter);
            SetThumbnails();

            g_programState.ignoreNext = TRUE;
        }
        break;
    case WM_MOUSEWHEEL: {
        int amount = GET_WHEEL_DELTA_WPARAM(wParam);
        if(amount > 0) MoveNext(VK_LEFT);
        else MoveNext(VK_RIGHT);
        break; }
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if(!g_programState.showing) break;
        SelectByMouse(lParam);
        break;
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        if(!g_programState.showing) break;
        SelectByMouse(lParam);
        SelectCurrent();
        break;
    case WM_KEYUP:
        if(g_programState.ignoreNext) {
            g_programState.ignoreNext = false;
            break;
        }
        switch(wParam) {
        case VK_ESCAPE:
            QuitOverlay();
            break;
        case VK_RIGHT:
        case VK_TAB:
            MoveNext(VK_RIGHT);
            break;
        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
            MoveNext(wParam);
            break;
        case VK_RETURN:
            SelectCurrent();
            break;
        case VK_F1:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case VK_F2:
            g_programState.logging = !g_programState.logging;
            break;
        case VK_BACK:
            if(!g_programState.filter.empty()) {
                g_programState.filter = g_programState.filter.substr(0, g_programState.filter.size() - 1);
                
                CreateThumbnails(g_programState.filter);
                SetThumbnails();
                RedrawWindow(g_programState.hWnd, NULL, NULL, RDW_INVALIDATE);
            }
            break;
        default: {
            INT hr = (INT)MapVirtualKey(wParam, MAPVK_VK_TO_CHAR);
            if(hr > 0) {
                g_programState.filter += (wchar_t)hr;
                log(_T("got filter %ls\n"), g_programState.filter.c_str());
                CreateThumbnails(g_programState.filter);
                SetThumbnails();
                RedrawWindow(g_programState.hWnd, NULL, NULL, RDW_INVALIDATE);
            }
            break; }
        }
        break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
