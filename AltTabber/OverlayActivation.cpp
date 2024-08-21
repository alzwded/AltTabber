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
#if 1
    hrSWP = SetWindowPos(g_programState.hWnd,
            HWND_TOPMOST,
            monitorGeom.r.left, monitorGeom.r.top,
            monitorGeom.r.right - monitorGeom.r.left, monitorGeom.r.bottom - monitorGeom.r.top,
            SWP_NOSENDCHANGING);
#else
    // DEBUG CODE DELETE ME
    hrSWP = SetWindowPos(g_programState.hWnd,
            HWND_TOPMOST,
            monitorGeom.r.left, monitorGeom.r.top,
            monitorGeom.r.right - monitorGeom.r.left, monitorGeom.r.bottom - monitorGeom.r.top - 1080,
            SWP_NOSENDCHANGING);
#endif
    log(_T("SetWindowPos returned %d: errno %d\n"), hrSWP, GetLastError());

    g_programState.filter = _T("");
    CreateThumbnails(g_programState.filter);
    SetThumbnails();

    MoveCursorOverActiveSlot();

    if(g_programState.uiaProvider) g_programState.uiaProvider->SelectionChanged();
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

            auto hr1 = PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            //auto hr1 = SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            //auto hr1 = OpenIcon(hwnd);
            log(_T("restoring %p hr = %ld\n"), (void*)hwnd, hr1);
        }

        // I don't really need to do this since windows seem to sometimes do
        // it automatically, but it's curtosy (or however you spell it)
        
		// update 2016: virtualized coordinates are more convoluted than I thought
		//              GetWindowRect gives me logical coordinates, which may or may
		//              not be real. GetCursorPos returns the same coordinates as
		//              not dpi aware applications, despite me declaring >this< as 
		//              dpi aware. GetPhysicalCursorPos seems to return the real
		//              mouse position (which is what I hope).
		// TODO test on windows 10
        RECT r;
        BOOL hrGWR = GetWindowRect(hwnd, &r);
		{
			log(_T("transformed [%d,%d,%d,%d] "), r.left, r.top, r.right, r.bottom);
			POINT p = {r.left, r.top};
			LogicalToPhysicalPoint(hwnd, &p);
			r.left = p.x;
			r.top = p.y;
			p.x = r.right;
			p.y = r.bottom;
			LogicalToPhysicalPoint(hwnd, &p);
			r.right = p.x;
			r.bottom = p.y;

			log(_T("to [%d,%d,%d,%d]\n"), r.left, r.top, r.right, r.bottom);
		}
        POINT p;

        BOOL hrGCP = GetPhysicalCursorPos(&p);
		log(_T("cursor at %d,%d\n"), p.x, p.y);

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

    if(g_programState.uiaProvider) g_programState.uiaProvider->SelectionChanged();
}
