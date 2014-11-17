#include "stdafx.h"
#include "AltTabber.h"

extern ProgramState_t g_programState;
extern void log(LPTSTR fmt, ...);
extern MonitorGeom_t GetMonitorGeometry();
extern void CreateThumbnails(std::wstring const& filter);
extern void SetThumbnails();
extern void MoveCursorOverActiveSlot();

void ActivateSwitcher()
{
    log(_T("activating switcher\n"));
    g_programState.prevActiveWindow = GetForegroundWindow();
    log(_T("previous window is %p\n"),
            (void*)g_programState.prevActiveWindow);
    g_programState.showing = TRUE;
    auto monitorGeom = GetMonitorGeometry();
    SetForegroundWindow(g_programState.hWnd);
    SetFocus(g_programState.hWnd);
    auto hrSWP = SetWindowPos(g_programState.hWnd,
            NULL,
            0, 0,
            0, 0,
            SWP_SHOWWINDOW | SWP_NOSENDCHANGING);
    log(_T("SetWindowPos returned %d: errno %d\n"), hrSWP, GetLastError());
    hrSWP = SetWindowPos(g_programState.hWnd,
            HWND_TOPMOST,
            monitorGeom.r.left, monitorGeom.r.top,
            monitorGeom.r.right - monitorGeom.r.left, monitorGeom.r.bottom - monitorGeom.r.top,
            SWP_NOSENDCHANGING);
    log(_T("SetWindowPos returned %d: errno %d\n"), hrSWP, GetLastError());

    g_programState.filter = _T("");
    CreateThumbnails(g_programState.filter);
    SetThumbnails();

    MoveCursorOverActiveSlot();
}

void QuitOverlay()
{
    log(_T("escape pressed; reverting\n"));
    g_programState.showing = FALSE;
    auto monitorGeom = GetMonitorGeometry();
    SetWindowPos(g_programState.hWnd,
            0,
            0, 0,
            0, 0,
            SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOSENDCHANGING);
    if(g_programState.prevActiveWindow) {
        HWND hwnd = g_programState.prevActiveWindow;
        auto hr = SetForegroundWindow(hwnd);
        log(_T("set foreground window to previous result: %d\n"), hr);
        if(IsIconic(hwnd)) {
            // note to self: apparently only the owner of the window
            // can re-maximize it/restore it properly; calling anything
            // else like SetWindowPlacement or ShowWindow results in
            // its internal min/max state being broken; by sending
            // that window an actual message, things seem to work fine

            auto hr = PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            //auto hr = SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            //auto hr = OpenIcon(hwnd);
            log(_T("restoring %p hr = %ld\n"), (void*)hwnd, hr);
        }

        // I don't really need to do this since windows seem to sometimes do
        // it automatically, but it's curtosy (or however you spell it)
        
        RECT r;
        BOOL hrGWR = GetWindowRect(hwnd, &r);
        POINT p;
        BOOL hrGCP = GetCursorPos(&p);

        auto& mis = monitorGeom;
        if(hrGWR != FALSE && hrGCP != FALSE && !PtInRect(&r, p)) {
            POINT tentativePoint = {
                (r.right + r.left) / 2,
                (r.bottom + r.top) / 2,
            };
            auto found = std::find_if(mis.monitors.begin(), mis.monitors.end(),
                [&tentativePoint](MonitorInfo_t& mon) -> bool {
                    return PtInRect(&mon.extent, tentativePoint) != FALSE;
                });
            if(found != mis.monitors.end()) {
                log(_T("Moving the mouse centered on the window\n"));
                SetCursorPos(tentativePoint.x, tentativePoint.y);
            } else {
                auto hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi;
                mi.cbSize = sizeof(MONITORINFO);
                mi.dwFlags = 0;
                GetMonitorInfo(hMonitor, &mi);
                
                RECT newTarget;
                BOOL hrIR = IntersectRect(&newTarget, &mi.rcWork, &r);

                if(hrIR) {
                    log(_T("Moving the mouse centered on the intersection of the window and the monitor it's on\n"));
                    SetCursorPos(
                        (newTarget.left + newTarget.right) / 2,
                        (newTarget.top + newTarget.bottom) / 2);
                } else {
                    log(_T("Moving the mouse centered on the monitor it's on\n"));
                    SetCursorPos(
                        (mi.rcWork.left + mi.rcWork.right) / 2,
                        (mi.rcWork.top + mi.rcWork.bottom) / 2);
                }
            }
        }
    }
}
