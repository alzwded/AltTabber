// AltTabber.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "AltTabber.h"

#define MAX_LOADSTRING 100

#define MY_NOTIFICATION_ICON 2
#define MY_NOTIFY_ICON_MESSAGE_ID (WM_USER + 88)
#define MY_CLOSE_BTN_ID (WM_USER + 89)

extern void log(LPTSTR fmt, ...);
extern MonitorGeom_t GetMonitorGeometry();
extern void SynchronizeWithRegistry();
extern void ActivateSwitcher();
extern void SelectCurrent();
extern void MoveNext(DWORD);
extern void SelectByMouse(DWORD);
extern void QuitOverlay();
extern void PurgeThumbnails();
extern void CreateThumbnails(std::wstring const&);
extern void SetThumbnails();
extern void OnPaint(HDC);
extern void MoveCursorOverActiveSlot(); 

ProgramState_t g_programState = {
    FALSE,
    NULL,
    -1,
    FALSE,
    NULL,
#ifdef JAT_OLD_HOTKEY
    { MOD_ALT | MOD_CONTROL, '3' },
#else
    { MOD_ALT, VK_OEM_3 },
#endif
    0,
};

// Global Variables:
HINSTANCE hInst;                                // current instance
TCHAR szTitle[MAX_LOADSTRING];                    // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


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

static inline void Cleanup()
{
    PurgeThumbnails();
    
    if(g_programState.freopened != NULL) fclose(g_programState.freopened);
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

    wcex.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALTTABBER));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = NULL; //MAKEINTRESOURCE(IDC_ALTTABBER);
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
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
BOOL InitInstance(HINSTANCE hInstance, int)
{
    HWND hWnd;

    hInst = hInstance; // Store instance handle in our global variable

    auto geom = GetMonitorGeometry();
    // SetWindowPos(hWnd, HWND_TOPMOST, geom.x, geom.y, geom.cx, geom.cy);

    hWnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_COMPOSITED | WS_EX_LAYERED,
            szWindowClass,
            szTitle, 
            WS_POPUP,
            geom.r.left, geom.r.top,
            geom.r.right - geom.r.left, geom.r.bottom - geom.r.top,
            NULL,
            NULL,
            hInstance,
            NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, SW_HIDE);
    UpdateWindow(hWnd);

    SynchronizeWithRegistry();

    if(RegisterHotKey(
                hWnd,
                1,
                g_programState.hotkey.modifiers,
                g_programState.hotkey.key))
    {
        log(_T("hotkey registered successfully\n"));
    }
    else
    {
        log(_T("failed to register hotkey\n"));
        MessageBox(hWnd,
                _T("Failed to register hotkey.\n"                           )
                _T("\n"                                                     )
                _T("This usually means that there is already a program\n"   )
                _T("running that has registered the same hotkey.\n"         )
                _T("\n"                                                     )
                _T("Possible resolutions:\n"                                )
                _T("- Close the already running instance of AltTabber\n"    )
                _T("- If there is another application that has registered\n")
                _T("  the current hotkey, change it in that application\n"  )
                _T("- Change AltTabber's hotkey by editing the registry\n"  )
                _T("  Refer to the README.md document (which you can find\n")
                _T("  at http://github.com/alzwded/AltTabber ) on how to\n" )
                _T("  accomplish this.\n"                                   )
                _T("\n"                                                     )
                _T("The application will now exit"                          )
            ,
            _T("AltTabber - Failed to register hotkey"),
            MB_OK | MB_ICONERROR);
        Cleanup();
        PostQuitMessage(0);
    }

    g_programState.hWnd = hWnd;

    NOTIFYICONDATA nid = {};
    ZeroMemory(&nid,sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.uID = MY_NOTIFICATION_ICON;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE ;
    nid.hWnd = hWnd;
    nid.uCallbackMessage = MY_NOTIFY_ICON_MESSAGE_ID;
#define TIP (_T("AltTabber\n- Ctrl-Alt-3 to open overlay\n- Alt-F4 when overlay is active to quit"))
    _tcsncpy_s(nid.szTip, TIP, _tcslen(TIP));
#undef TIP
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    HRESULT sniHr = Shell_NotifyIcon(NIM_ADD, &nid);
    log(_T("Shell_NotifyIcon result: %ld errno %ld\n"), sniHr, GetLastError());

    auto hrFW = FindWindow(_T("ThunderRT6Main"), _T("Dexpot"));
    if(hrFW) {
        g_programState.compatHacks |= JAT_HACK_DEXPOT;
    }

    return TRUE;
}

static void ShowContextMenu(int x, int y)
{    
    if(g_programState.activeSlot >= 0
        && (unsigned long)g_programState.activeSlot < g_programState.slots.size())
    {
        HMENU ctxMenu = CreatePopupMenu();
        AppendMenu(ctxMenu, MF_STRING, MY_CLOSE_BTN_ID, _T("Close"));
        TrackPopupMenu(ctxMenu, 
            TPM_RIGHTBUTTON,
            x, y,
            0, g_programState.hWnd, NULL);
        DestroyMenu(ctxMenu);
    }
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case MY_NOTIFY_ICON_MESSAGE_ID: {
        auto what = LOWORD(lParam);
        switch(what) {
        case WM_LBUTTONUP:
            ActivateSwitcher();
            break;
        }
        break; }
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
        case MY_CLOSE_BTN_ID:
            if(g_programState.activeSlot >= 0) {
                auto& slot = g_programState.slots[g_programState.activeSlot];
                PostMessage(slot.hwnd, WM_SYSCOMMAND, SC_CLOSE, -1);
                // clear the filter because of use case
                if(g_programState.slots.size() <= 2) {
                    g_programState.filter = L"";
                }
                // rebuild thumbnails because filter was changed
                // and there are maybe dangling slots
                CreateThumbnails(g_programState.filter);
                SetThumbnails();
                // force redraw window (the labels)
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
            }
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
        // only waiting for one, so skip over trying to decode it
        // even if multiple keys are registered as hotkeys, they'd
        // all do the same thing anyway
        log(_T("hotkey pressed\n"));
        if(g_programState.showing) {
            SelectCurrent();
        } else {
            ActivateSwitcher();
        }
        break;
    case WM_MOUSEWHEEL: {
        int amount = GET_WHEEL_DELTA_WPARAM(wParam);
        if(amount > 0) MoveNext(VK_BACK);
        else MoveNext(VK_TAB);
        break; }
    case WM_RBUTTONUP:
        if(!g_programState.showing) break;
        SelectByMouse((DWORD)lParam);
        {
            POINT p = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ClientToScreen(g_programState.hWnd, &p);
            ShowContextMenu(p.x, p.y);
        }
        break;
    case WM_LBUTTONUP:
        if(!g_programState.showing) break;
        SelectByMouse((DWORD)lParam);
        break;
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        if(!g_programState.showing) break;
        SelectByMouse((DWORD)lParam);
        SelectCurrent();
        break;
    case WM_CHAR:
        switch(wParam) {
        case 0x1B: // esc
        case 0x08: // backspace
        case 0x09: // tab
        case 0x0D: // enter
        // case 0x0A: // LF
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        g_programState.filter += (wchar_t)wParam;
        log(_T("got filter %ls\n"), g_programState.filter.c_str());
        CreateThumbnails(g_programState.filter);
        SetThumbnails();
        RedrawWindow(g_programState.hWnd, NULL, NULL, RDW_INVALIDATE);
        MoveCursorOverActiveSlot();
        break;
    case WM_KEYDOWN:
        switch(wParam) {
        case VK_APPS: // menu key
            if(g_programState.activeSlot >= 0) {
                RECT r = g_programState.slots[g_programState.activeSlot].r;
                ShowContextMenu(r.left, r.top);
            }
            break;
        case VK_ESCAPE:
            QuitOverlay();
            break;
        case VK_TAB:
            if(GetAsyncKeyState(VK_SHIFT)) {
                MoveNext(VK_BACK);
            } else {
                MoveNext(VK_TAB);
            }
            break;
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
            MoveNext((DWORD)wParam);
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
                g_programState.filter =
                    g_programState.filter.substr(
                            0, g_programState.filter.size() - 1);
                
                CreateThumbnails(g_programState.filter);
                SetThumbnails();
                RedrawWindow(g_programState.hWnd, NULL, NULL, RDW_INVALIDATE);
                MoveCursorOverActiveSlot();
            }
            break;
#if 0
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
#else
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
#endif
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
