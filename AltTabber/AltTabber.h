#pragma once

#include "resource.h"

#include <dwmapi.h>

#include <vector>
#include <map>
#include <string>

typedef enum {
    APP_THUMB_COMPAT = 0,
    APP_THUMB_AERO,
} app_thumb_e_t;

typedef struct {
    app_thumb_e_t type;
    HWND hwnd;
    union {
        HTHUMBNAIL thumb;
        HICON icon;
    };
} AppThumb_t;

typedef struct {
    HWND hwnd;
    RECT r;
    int moveUpDownAmount;
} SlotThing_t;

typedef struct {
    BOOL showing;
    HWND prevActiveWindow;
    signed long activeSlot;
    BOOL logging;
    FILE* freopened;
    struct {
        UINT modifiers;
        UINT key;
    } hotkey;

    HWND hWnd;
    std::map<HMONITOR, std::vector<AppThumb_t> > thumbnails;
    std::vector<SlotThing_t> slots;
    std::wstring filter;
} ProgramState_t;

typedef struct {
    HMONITOR hMonitor;
    RECT extent;
} MonitorInfo_t;

typedef struct {
    RECT r;
    std::vector<MonitorInfo_t> monitors;
} MonitorGeom_t;