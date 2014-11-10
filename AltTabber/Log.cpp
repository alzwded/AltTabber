#include "stdafx.h"
#include "AltTabber.h"

extern ProgramState_t g_programState;

void log(LPTSTR fmt, ...)
{
    if(!g_programState.logging) return;

    if(!g_programState.freopened) {
        // replace stdout with log file
        TCHAR tempPath[MAX_PATH + 1];
        auto hr = GetTempPath(MAX_PATH, tempPath);
        if(hr == 0) abort();

        TCHAR tempFile[MAX_PATH + 1];
        UINT hrTFN = GetTempFileName(
                tempPath,
                _T("altTabber"),
                0,
                tempFile);
        if(hrTFN == 0 || hrTFN == ERROR_BUFFER_OVERFLOW) abort();

        _tfreopen_s(
                &g_programState.freopened,
                tempFile,
                _T("w"),
                stdout);
    }

    va_list ap;
    va_start(ap, fmt);

    _vwprintf_p(fmt, ap);
    fflush(stdout);

    va_end(ap);
}