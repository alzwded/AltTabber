
#include "stdafx.h"
#include "AltTabber.h"

extern ProgramState_t g_programState;
extern void log(LPTSTR fmt, ...);

void SynchronizeWithRegistry()
{
#define SUBKEY (_T("Software\\jakkal\\AltTabber"))
    HKEY phk = NULL;
    DWORD disposition = 0;
    auto hr = RegCreateKeyEx(HKEY_CURRENT_USER,
        SUBKEY,
        0, 
        NULL,
        0,
        KEY_READ | KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WRITE,
        NULL,
        &phk,
        &disposition);

    if(hr != ERROR_SUCCESS) {
        log(_T("RegCreateKey failed %d: errno: %d\n"), hr, GetLastError());
        return;
    }
    
    DWORD dModifiers = (DWORD)g_programState.hotkey.modifiers;
    DWORD dKey = (DWORD)g_programState.hotkey.key;
    DWORD dSize = sizeof(DWORD);

    switch(disposition) {
    case REG_CREATED_NEW_KEY: {
        // set default values
        hr = RegSetValueEx(phk,
            _T("modifiers"),
            0,
            REG_DWORD,
            (BYTE*)&dModifiers,
            sizeof(DWORD));
        if(hr != ERROR_SUCCESS) {
            log(_T("RegSetValue failed %d: errno %d\n"), hr, GetLastError());
            goto finished;
        }
        hr = RegSetValueEx(phk,
            _T("key"),
            0,
            REG_DWORD,
            (BYTE*)&dKey,
            sizeof(DWORD));
        if(hr != ERROR_SUCCESS) {
            log(_T("RegSetValue failed %d: errno %d\n"), hr, GetLastError());
            goto finished;
        }
        break; }
    case REG_OPENED_EXISTING_KEY:
        // read values
        dSize = sizeof(DWORD);
        hr = RegQueryValueEx(phk,
            _T("modifiers"),
            0,
            NULL,
            (BYTE*)&dModifiers,
            &dSize);
        if(hr != ERROR_SUCCESS) {
            log(_T("RegQueryValue failed %d: errno %d\n"), hr, GetLastError());
            goto finished;
        }
        dSize = sizeof(DWORD);
        hr = RegQueryValueEx(phk,
            _T("key"),
            0,
            NULL,
            (BYTE*)&dKey,
            &dSize);
        if(hr != ERROR_SUCCESS) {
            log(_T("RegQueryValue failed %d: errno %d\n"), hr, GetLastError());
            goto finished;
        }

        g_programState.hotkey.modifiers = (UINT)(ULONG)dModifiers;
        g_programState.hotkey.key = (UINT)(ULONG)dKey;
        break;
    }

finished:
    RegCloseKey(phk);
}
