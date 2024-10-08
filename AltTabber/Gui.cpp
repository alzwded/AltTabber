#include "stdafx.h"
#include "AltTabber.h"
#include <WinUser.h>

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

    // Windows 8+ added cloaking
    DWORD dwmAttributes = 0;
    HRESULT dwmHR = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &dwmAttributes, sizeof(dwmAttributes));
    if(dwmHR == S_OK) {
        log(_T("DwmGetWindowAttribute(%d, DWM_CLOAKED) = %08X\n"),
            hwnd, dwmAttributes);

        if(0 != (dwmAttributes & (DWM_CLOAKED_APP | DWM_CLOAKED_SHELL | DWM_CLOAKED_INHERITED)))
            return FALSE;
    } else {
        log(_T("Failed to get DwmGetWindowAttribute DWM_CLOACKED for %d, hr %d\n"),
            hwnd, dwmHR);
    }

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
            } else {
                log(_T("GetModuleFileNameEx failed: %u errno %d\n"), len, GetLastError());
                // okay, if it fails with errno 299 it's because there's
                // an issue with 64bit processes querried from a 32bit process
                // compiling AltTabber for x64 should fix that.
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

    std::transform(title.begin(), title.end(), title.begin(), ::_totlower);
    std::transform(filter.begin(), filter.end(), filter.begin(), ::_totlower);
    if(!filter.empty() && title.find(filter) == std::wstring::npos) {
        return TRUE;
    }

    HMONITOR hMonitor = NULL;

    if(g_programState.compatHacks & JAT_HACK_DEXPOT) {
        hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
        if(!hMonitor) return TRUE;
    } else {
        hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
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
        HICON hIcon = NULL;
        // #1 try to get via getclasslong
        hIcon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
        if(!hIcon) {
            // #2 try to get via WM_GETICON
            DWORD_PTR lresult;
            UINT sleepAmount = (g_programState.sleptThroughNWindows < 5)
                ? 50
                : (g_programState.sleptThroughNWindows < 10)
                    ? 25
                    : (g_programState.sleptThroughNWindows < 20)
                    ? 10
                    : 5;
            auto lr = SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0,
                SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT,
                sleepAmount /*ms*/,
                &lresult);
            if(lr) {
                g_programState.sleptThroughNWindows++;
                hIcon = (HICON)lresult;
            } else {
                log(_T("Sending WM_GETICON to %ld failed; lresult %ld errno %d\n"),
                    hwnd, lresult, GetLastError());
            }
        }
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
    g_programState.sleptThroughNWindows = 0;
    auto hr = EnumDesktopWindows(hDesktop, enumWindows, (LPARAM)&filter);
    CloseDesktop(hDesktop);
    log(_T("enum desktop windows: %d\n"), hr);
}

template<typename F>
void PerformSlotting(F&& functor)
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
    InterlockedIncrement(&g_programState.rebuildingSlots);
    g_programState.slots.clear();
    //ShowWindow(g_programState.hWnd, SW_HIDE);
    size_t nthSlot = 0;

    MonitorGeom_t mis = GetMonitorGeometry();

    PerformSlotting([&](MonitorInfo_t& mi, size_t j, long l1, long, long hs, long ws) {
            AppThumb_t& thumb = g_programState.thumbnails[mi.hMonitor][j];

            long x = ((long)j % l1) * ws + 3;
            long y = ((long)j / l1) * hs + hs / 3 + 3;
            long x1 = x + ws - 6;
            long y1 = y + hs - hs / 3 - 6;
            RECT r;
            r.left = mi.extent.left - mis.r.left + x;
            r.right = mi.extent.left - mis.r.left + x1;
            r.top = mi.extent.top - mis.r.top + y;
            r.bottom = mi.extent.top - mis.r.top + y1;

            if(thumb.type == APP_THUMB_AERO) {
                DWM_THUMBNAIL_PROPERTIES thProps;
                thProps.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE;

                SIZE ws1;
                HRESULT haveThumbSize = DwmQueryThumbnailSourceSize(thumb.thumb, &ws1);
                if(haveThumbSize == S_OK) {
                    SIZE rs;
                    rs.cx = r.right - r.left;
                    rs.cy = r.bottom - r.top;

                    RECT dRect;
                    dRect.left = r.left;
                    dRect.top = r.top;

                    float rRap = (float)rs.cx / (float)rs.cy;
                    float sRap = (float)ws1.cx / (float)ws1.cy;
                    if(sRap > rRap) {
                        dRect.right = r.right;
                        LONG h = (LONG)(rs.cx / sRap);
                        LONG delta = (rs.cy - h) / 2;

                        dRect.top += delta;
                        dRect.bottom = dRect.top + h;
                    } else {
                        dRect.bottom = r.bottom;
                        LONG w = (LONG)(rs.cy * sRap);
                        LONG delta = (rs.cx - w) / 2;

                        dRect.left += delta;
                        dRect.right = dRect.left + w;
                    }

                    thProps.rcDestination = dRect;
                } else {
                    thProps.rcDestination = r;
                    log(_T("DwmQueryThumbnailSourceSize failed %d: errno %d\n"),
                        haveThumbSize, GetLastError());
                }
                thProps.fVisible = TRUE;
                DwmUpdateThumbnailProperties(thumb.thumb, &thProps);
            }

            if(thumb.hwnd == g_programState.prevActiveWindow) {
                g_programState.activeSlot = (long)nthSlot;
            }

            SlotThing_t st;
            st.hwnd = thumb.hwnd;
            st.r.left = mi.extent.left - mis.r.left + (j % l1) * ws;
            st.r.top = mi.extent.top - mis.r.top + ((long)j / l1) * hs;
            st.r.right = st.r.left + ws;
            st.r.bottom = st.r.top + hs;
            st.moveUpDownAmount = l1;
            g_programState.slots.push_back(st);

            ++nthSlot;
    });

    if(g_programState.activeSlot < 0 && g_programState.slots.size() > 0) {
        g_programState.activeSlot = 0;
    }

    InterlockedDecrement(&g_programState.rebuildingSlots);

    if(g_programState.uiaProvider) g_programState.uiaProvider->Invalidate();
}

void OnPaint(HDC hdc)
{
    HGDIOBJ original = NULL;
    original = SelectObject(hdc, GetStockObject(DC_PEN));
    //HPEN pen = CreatePen(PS_SOLID, 5, RGB(0, 255, 0));
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

    PerformSlotting([&](MonitorInfo_t& mi, size_t j, long l1, long, long hs, long ws) {
            AppThumb_t& thumb = g_programState.thumbnails[mi.hMonitor][j];
            HWND hwnd = thumb.hwnd;
            
            long x = ((long)j % l1) * ws + 3;
            long y = ((long)j / l1) * hs + 3;
            long x1 = x + ws - 6;
            long y1 = y + hs - 2 * hs / 3 - 6;
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
            r.right -= 6;
            r.top += 3;
            r.bottom -= 6;
            DrawText(hdc, str, -1, &r, DT_BOTTOM | DT_LEFT | DT_WORDBREAK);

            if(thumb.type == APP_THUMB_COMPAT) {
                ICONINFO iconInfo;
                auto hr = GetIconInfo(thumb.icon, &iconInfo);
                if(!hr) {
                    log(_T("GetIconInfo failed; errno %d\n"), GetLastError());
                    DrawIcon(hdc, r.left + 3, r.bottom + 6, thumb.icon);
                } else {
                    BITMAP bmp;
                    ZeroMemory(&bmp, sizeof(BITMAP));
                    auto nBytes = GetObject(iconInfo.hbmMask, sizeof(BITMAP), &bmp);
                    if(nBytes != sizeof(BITMAP)) {
                        log(_T("failed to retrieve bitmap from hicon for %p; errno %d\n"),
                            (void*)thumb.hwnd, GetLastError());
                    }
                    
                    SIZE size = { 
                        bmp.bmWidth,
                        bmp.bmHeight,
                    };

                    if(iconInfo.hbmColor != NULL) {
                        size.cy /= 2;
                    }

                    log(_T("bitmap %p size: %ld x %ld\n"),
                        (void*)thumb.icon, size.cx, size.cy);

                    POINT location = {
                        (r.right + r.left) / 2 - size.cx / 2,
                        r.top + 2 * hs / 3 - size.cy,
                    };

                    DeleteBitmap(iconInfo.hbmColor);
                    DeleteBitmap(iconInfo.hbmMask);

                    DrawIcon(hdc, location.x, location.y, thumb.icon);
                }
            }
    });

    DeleteObject(font);
    SetBkMode(hdc, prevBkMode);
    SelectObject(hdc, originalFont);
    SelectObject(hdc, originalBrush);
    SelectObject(hdc, original);
}