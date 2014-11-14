#include "stdafx.h"
#include "AltTabber.h"

extern ProgramState_t g_programState;
extern void log(LPTSTR fmt, ...);
extern MonitorGeom_t GetMonitorGeometry();

static inline BOOL IsAltTabWindow(HWND hwnd)
{
    if(!IsWindowVisible(hwnd))
        return FALSE;

    TCHAR str[MAX_PATH + 1];
    GetWindowText(hwnd, str, MAX_PATH);
    
    log(_T("window %ls has style %lX and exstyle %lX\n"),
        str,
        GetWindowLong(hwnd, GWL_STYLE),
        GetWindowLong(hwnd, GWL_EXSTYLE));
    log(_T("parent: %p\n"), GetParent(hwnd));

    // this prevents stuff like the copy file dialog from showing up
    // but it also prevents stuff like tunngle's border windows from
    // showing up; I've opted to hide all of them.
    HWND hwndTry, hwndWalk = NULL;
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

    // this prevents borderless windows from showing up (like steam or remote desktop)
#if 0
    TITLEBARINFO ti;

    // the following removes some task tray programs and "Program Manager"
    ti.cbSize = sizeof(ti);
    GetTitleBarInfo(hwnd, &ti);
    if(ti.rgstate[0] & STATE_SYSTEM_INVISIBLE)
        return FALSE;
#endif

    // Tool windows should not be displayed either, these do not appear in the
    // task bar.
    if(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
        return FALSE;

    if(GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
        return FALSE;
#if 0
    LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
    if((style & (WS_VISIBLE | WS_EX_TOOLWINDOW | WS_POPUP | WS_CAPTION
        | WS_DLGFRAME | WS_OVERLAPPED | WS_TILED | WS_SYSMENU | WS_THICKFRAME
        )) == 0)
    {
        return FALSE;
    }
#endif

    return TRUE;
}

static BOOL GetImagePathName(HWND hwnd, std::wstring& imagePathName)
{
    BOOL hr = 0;
    TCHAR str2[MAX_PATH + 1];
    DWORD procId;
    if(GetWindowThreadProcessId(hwnd, &procId) > 0) {
        auto hnd = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE,
                procId);
        if(hnd != NULL) {
            UINT len;
            if((len = GetModuleFileNameEx(hnd, NULL, str2, MAX_PATH)) > 0) {
                imagePathName.assign(&str2[0], &str2[len]);
                hr = 1;
            }
            CloseHandle(hnd);
        }
    }

    return hr;
}

static BOOL CALLBACK enumWindows(HWND hwnd, LPARAM lParam)
{
    if(hwnd == g_programState.hWnd) return TRUE;
    if(!IsAltTabWindow(hwnd)) return TRUE;

    std::wstring filter = *((std::wstring const*)lParam);

    TCHAR str[257];
    GetWindowText(hwnd, str, 256);
    std::wstring title(str);

    std::wstring imagePathName;
    if(GetImagePathName(hwnd, imagePathName)) {
        std::wstringstream wss;
        wss << imagePathName << std::wstring(L" // ") << title;
        title = wss.str();
    }

    log(_T("the label is: %ls\n"), title.c_str());

    std::transform(title.begin(), title.end(), title.begin(), ::tolower);
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
    if(!filter.empty() && title.find(filter) == std::wstring::npos) {
        return TRUE;
    }

    HMONITOR hMonitor = NULL;

    if(g_programState.compatHacks & JAT_HACK_DEXPOT) {
        hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
        if(!hMonitor) return TRUE;
    } else {
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    }

    HTHUMBNAIL hThumb = NULL;
    auto hr = DwmRegisterThumbnail(g_programState.hWnd, hwnd, &hThumb);
    log(_T("register thumbnail for %p on monitor %p: %d\n"),
            (void*)hwnd,
            (void*)hMonitor,
            hr);
    if(hr == S_OK) {
        AppThumb_t at = {
            APP_THUMB_AERO,
            hwnd,
        };
        at.thumb = hThumb;
        g_programState.thumbnails[hMonitor].push_back(at);
    } else {
        AppThumb_t at = {
            APP_THUMB_COMPAT,
            hwnd,
        };
        HICON hIcon = (HICON)GetClassLong(hwnd, GCL_HICON);
        at.icon = hIcon;
        g_programState.thumbnails[hMonitor].push_back(at);
    }

    return TRUE;
}

void PurgeThumbnails()
{
    for(auto i = g_programState.thumbnails.begin();
            i != g_programState.thumbnails.end();
            ++i)
    {
        auto thumbs = i->second;
        decltype(thumbs) rest(thumbs.size());
        std::remove_copy_if(
                thumbs.begin(), thumbs.end(),
                std::inserter(rest, rest.end()),
                [](AppThumb_t const& thumb) -> bool {
                    return thumb.type != APP_THUMB_AERO;
                });
        std::for_each(rest.begin(), rest.end(),
                [](AppThumb_t const& thumb) {
                    DwmUnregisterThumbnail(thumb.thumb);
                });
    }
    g_programState.thumbnails.clear();
}

void CreateThumbnails(std::wstring const& filter)
{
    PurgeThumbnails();
    auto hDesktop = OpenInputDesktop(0, FALSE, GENERIC_READ);
    if(!hDesktop) {
        log(_T("open desktop failed; errno = %d\n"), GetLastError());
        return;
    }
    auto hr = EnumDesktopWindows(hDesktop, enumWindows, (LPARAM)&filter);
    CloseDesktop(hDesktop);
    log(_T("enum desktop windows: %d\n"), hr);
}

template<typename F>
void PerformSlotting(F& functor)
{
    auto mis = GetMonitorGeometry();
    for(size_t i = 0; i < mis.monitors.size(); ++i) {
        auto& mi = mis.monitors[i];
        auto& thumbs = g_programState.thumbnails[mi.hMonitor];
        auto nWindows = thumbs.size();
        
        unsigned int nTiles = 1;
        while(nTiles < nWindows) nTiles <<= 1;
        if(nTiles != nWindows) nTiles = max(1, nTiles >> 1);
        
        long lala = (long)(sqrt((double)nTiles) + 0.5);

        long l1 = lala;
        long l2 = lala;
        while(((unsigned)l1) * ((unsigned)l2) < nWindows) l1++;
        lala = l1 * l2;

        long ws = (mi.extent.right - mi.extent.left) / l1;
        long hs = (mi.extent.bottom - mi.extent.top) / l2;

        for(size_t j = 0; j < thumbs.size(); ++j) {
            functor(mi, j, l1, l2, hs, ws);
        }
    }
}

void SetThumbnails()
{
    g_programState.activeSlot = -1;
    g_programState.slots.clear();
    //ShowWindow(g_programState.hWnd, SW_HIDE);
    size_t nthSlot = 0;

    MonitorGeom_t mis = GetMonitorGeometry();

    PerformSlotting([&](MonitorInfo_t& mi, size_t j, long l1, long l2, long hs, long ws) {
            AppThumb_t& thumb = g_programState.thumbnails[mi.hMonitor][j];

            long x = (j % l1) * ws + 3;
            long y = (j / l1) * hs + hs / 3;
            long x1 = x + ws - 3;
            long y1 = y + hs - hs / 3;
            RECT r;
            r.left = mi.extent.left - mis.r.left + x;
            r.right = mi.extent.left - mis.r.left + x1;
            r.top = mi.extent.top - mis.r.top + y;
            r.bottom = mi.extent.top - mis.r.top + y1;

            if(thumb.type == APP_THUMB_AERO) {
                DWM_THUMBNAIL_PROPERTIES thProps;
                thProps.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE;
                thProps.rcDestination = r;
                thProps.fVisible = TRUE;
                DwmUpdateThumbnailProperties(thumb.thumb, &thProps);
            }

            if(thumb.hwnd == g_programState.prevActiveWindow) {
                g_programState.activeSlot = nthSlot;
            }

            SlotThing_t st;
            st.hwnd = thumb.hwnd;
            st.r.left = mi.extent.left - mis.r.left + (j % l1) * ws;
            st.r.top = mi.extent.top - mis.r.top + (j / l1) * hs;
            st.r.right = st.r.left + ws;
            st.r.bottom = st.r.top + hs;
            st.moveUpDownAmount = l1;
            g_programState.slots.push_back(st);

            ++nthSlot;
    });

    if(g_programState.activeSlot < 0 && g_programState.slots.size() > 0) {
        g_programState.activeSlot = 0;
    }
}

void OnPaint(HDC hdc)
{
    HGDIOBJ original = NULL;
    original = SelectObject(hdc, GetStockObject(DC_PEN));
    HPEN pen = CreatePen(PS_SOLID, 5, RGB(0, 255, 0));
    auto originalBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));
    SelectObject(hdc, GetStockObject(DC_BRUSH));

    LONG fSize = 12l;
    fSize = MulDiv(fSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    if(fSize != 12l) log(_T("font size scaled to %ld\n"), fSize);
    HFONT font = CreateFont(
            fSize, 0, 0, 0, 0, 0, 0, 0,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, 
            DEFAULT_PITCH | FF_DONTCARE,
            _T("Courier New"));
    HFONT originalFont = (HFONT)SelectObject(hdc, font);
    
    auto mis = GetMonitorGeometry();
    SetDCBrushColor(hdc, RGB(0, 0, 0));
    log(_T("rectangle is %ld %ld %ld %ld\n"), mis.r.left, mis.r.top, mis.r.right, mis.r.bottom);
    //auto hrRectangle = Rectangle(hdc, 0, 0, mis.r.right - mis.r.left, mis.r.bottom - mis.r.top);
    //log(_T("rectangle returned %d: errno %d\n"), hrRectangle, GetLastError());
    RECT winRect;
    GetWindowRect(g_programState.hWnd, &winRect);
    log(_T("window rect %ld %ld %ld %ld\n"), winRect.left, winRect.top, winRect.right, winRect.bottom);
    auto hrRectangle = Rectangle(hdc, 0, 0, winRect.right - winRect.left, winRect.bottom - winRect.top);
    log(_T("rectangle returned %d: errno %d\n"), hrRectangle, GetLastError());

    SetDCBrushColor(hdc, RGB(255, 0, 0));

    //for(size_t i = 0; i < g_programState.slots.size(); ++i) {
    //    RECT r = (g_programState.slots[i].r);
    //    Rectangle(hdc, r.left, r.top, r.right, r.bottom);
    //}
    if(g_programState.activeSlot >= 0) {
        RECT r = (g_programState.slots[g_programState.activeSlot]).r;
        Rectangle(hdc, r.left, r.top, r.right, r.bottom);
    }

    SelectObject(hdc, GetStockObject(WHITE_BRUSH));
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    int prevBkMode = SetBkMode(hdc, TRANSPARENT);

    PerformSlotting([&](MonitorInfo_t& mi, size_t j, long l1, long l2, long hs, long ws) {
            AppThumb_t& thumb = g_programState.thumbnails[mi.hMonitor][j];
            HWND hwnd = thumb.hwnd;
            
            long x = (j % l1) * ws + 3;
            long y = (j / l1) * hs + 3;
            long x1 = x + ws - 3;
            long y1 = y + hs - 2 * hs / 3 - 2;
            RECT r;
            r.left = mi.extent.left - mis.r.left + x;
            r.right = mi.extent.left - mis.r.left + x1;
            r.top = mi.extent.top - mis.r.top + y;
            r.bottom = mi.extent.top - mis.r.top + y1;
            
            TCHAR str[257];
            GetWindowText(hwnd, str, 256);
            std::wstring title(str);

            Rectangle(hdc, r.left, r.top, r.right, r.bottom);
            r.left += 3;
            r.right -= 3;
            r.top += 3;
            r.bottom -= 3;
            DrawText(hdc, str, -1, &r, DT_BOTTOM | DT_LEFT | DT_WORDBREAK);

            if(thumb.type == APP_THUMB_COMPAT) {
                DrawIcon(hdc, r.left + 3, r.bottom + 3, thumb.icon);
            }
    });

    DeleteObject(font);
    SetBkMode(hdc, prevBkMode);
    SelectObject(hdc, originalFont);
    SelectObject(hdc, originalBrush);
    SelectObject(hdc, original);
}