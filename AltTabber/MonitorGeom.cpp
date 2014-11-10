#include "stdafx.h"
#include "AltTabber.h"

#include <cstdio>

#include <algorithm>
#include <iterator>

#include <Dwmapi.h>
#include <Windowsx.h>
#include <sstream>
#include <Shellapi.h>
#include <Psapi.h>

extern void log(LPTSTR fmt, ...);

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
    log(
            _T("Get monitor info: %d; %ld %ld %ld %ld\n"),
            hr,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right, mi.rcMonitor.bottom);
    if(!hr) {
        log(_T("\tlast error: %d\n"), GetLastError());
    }

    MonitorInfo_t mit = { hMonitor, mi.rcMonitor };

    std::vector<MonitorInfo_t>& stuff =
        *((std::vector<MonitorInfo_t>*)dwData);
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

    std::sort(
            ret.monitors.begin(), ret.monitors.end(),
            [](
                std::vector<MonitorInfo_t>::value_type const& left,
                std::vector<MonitorInfo_t>::value_type const& right)
            -> bool {
                if(left.extent.top == right.extent.top) {
                    return left.extent.left < right.extent.left;
                }
                return left.extent.top < right.extent.top;
            });

    return ret;
}