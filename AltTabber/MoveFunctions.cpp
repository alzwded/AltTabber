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

extern ProgramState_t g_programState;
extern void log(LPTSTR fmt, ...);
extern void QuitOverlay();

void MoveCursorOverActiveSlot()
{
    if(g_programState.activeSlot < 0) return;

    auto& r = g_programState.slots[g_programState.activeSlot].r;
    POINT moveHere = {
        (r.right + r.left) / 2,
        (r.bottom + r.top) / 2,
    };
    ClientToScreen(g_programState.hWnd, &moveHere);
    SetCursorPos(moveHere.x, moveHere.y);
}

void MoveNextGeographically(POINT p)
{
    if(g_programState.activeSlot < 0) {
        log(_T("no active slot"));
        return;
    }

    auto& slots = g_programState.slots;
    SlotThing_t& slot = slots[g_programState.activeSlot];
    RECT& r = slot.r;
    log(_T("moving away from %ld %ld %ld %ld\n"),
        r.left, r.top, r.right, r.bottom);
    POINT speculant = {
            p.x * 5
            + (p.x > 0) * r.right
            + (p.x < 0) * r.left
            + (!p.x) * ( (r.left + r.right) / 2)
        ,
            p.y * 5
            + (p.y > 0) * r.bottom
            + (p.y < 0) * r.top
            + (!p.y) * ( (r.top + r.bottom) / 2l)
        ,
    };

    auto found = std::find_if(slots.begin(), slots.end(),
        [&speculant](SlotThing_t& s) -> bool {
            return PtInRect(&s.r, speculant) != FALSE;
        });

    if(found != slots.end()) {
        g_programState.activeSlot = found - slots.begin();
        return;
    }
    /* else */
    log(_T("could not find a slot speculating at %ld %ld, trying wrap around\n"),
        speculant.x, speculant.y);
    RECT wr;
    (void) GetWindowRect(g_programState.hWnd, &wr);
    speculant.x = 
            p.x * 5
            + (p.x > 0) * 0
            + (p.x < 0) * (wr.right - wr.left)
            + (!p.x) * ( (r.left + r.right) / 2)
        ;
    speculant.y =
            p.y * 5
            + (p.y > 0) * 0
            + (p.y < 0) * (wr.bottom - wr.top)
            + (!p.y) * ( (r.top + r.bottom) / 2l)
        ;
    
    auto found2 = std::find_if(slots.begin(), slots.end(),
        [&speculant](SlotThing_t& s) -> bool {
            return PtInRect(&s.r, speculant) != FALSE;
        });
    
    if(found2 != slots.end()) {
        g_programState.activeSlot = found2 - slots.begin();
        return;
    }
    
    log(_T("could not find a slot speculating at %ld %ld, silently failing\n"),
        speculant.x, speculant.y);
}

void MoveNext(DWORD direction)
{
    if(g_programState.activeSlot < 0) {
        if(g_programState.slots.size() > 0) {
            g_programState.activeSlot = g_programState.slots.size() - 1;
        } else {
            return;
        }
    }
    log(_T("Moving from %ld "), g_programState.activeSlot);
    POINT p = { 0, 0 };
    switch(direction)
    {
        case VK_TAB:
            g_programState.activeSlot++;
            log(_T("by %d\n"), 1);
            break;
        case VK_BACK:
            g_programState.activeSlot--;
            log(_T("by %d\n"), -1);
            break;
        case VK_LEFT:
            p.x = -1;
            MoveNextGeographically(p);
            break;
        case VK_RIGHT:
            p.x = 1;
            MoveNextGeographically(p);
            break;
        case VK_UP:
            p.y = -1;
            MoveNextGeographically(p);
            break;
        case VK_DOWN:
            p.y = 1;
            MoveNextGeographically(p);
            break;
    }
    if(g_programState.activeSlot < 0) {
        g_programState.activeSlot =
            g_programState.slots.size() + g_programState.activeSlot;
    }
    g_programState.activeSlot %= g_programState.slots.size();

    if(g_programState.activeSlot >= 0) {
        log(_T("Current active slot: %ld hwnd: %p\n"),
                g_programState.activeSlot,
                (void*)g_programState.slots[g_programState.activeSlot].hwnd);
    }

    MoveCursorOverActiveSlot();

    RedrawWindow(g_programState.hWnd, NULL, NULL, RDW_INVALIDATE);
}

void SelectByMouse(DWORD lParam)
{
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);
    POINT pt = { xPos, yPos };
    auto found = std::find_if(
            g_programState.slots.begin(), g_programState.slots.end(),
            [&](SlotThing_t const& thing) -> BOOL {
                return PtInRect(&thing.r, pt);
            });
    if(found != g_programState.slots.end()) {
        g_programState.activeSlot = found - g_programState.slots.begin();
        RedrawWindow(g_programState.hWnd, NULL, NULL, RDW_INVALIDATE);
    }
}

void SelectCurrent()
{
    if(g_programState.activeSlot >= 0) {
        g_programState.prevActiveWindow =
            g_programState.slots[g_programState.activeSlot]
            .hwnd;
        QuitOverlay();
    }
}