#pragma once

#include <windows.h>

// Send a dedicated Pause command, never a Play/Pause toggle. Unhandled commands
// travel through DefWindowProc to the shell's media-command hook. Successful
// delivery does not guarantee that a player supports the command.
inline DWORD requestMediaPause(HWND target, HWND source) {
    if (!target) return ERROR_INVALID_WINDOW_HANDLE;
    DWORD_PTR handled = 0;
    SetLastError(ERROR_SUCCESS);
    if (!SendMessageTimeoutW(target, WM_APPCOMMAND, reinterpret_cast<WPARAM>(source),
            MAKELPARAM(0, APPCOMMAND_MEDIA_PAUSE), SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
            2000, &handled)) {
        const DWORD error = GetLastError();
        return error == ERROR_SUCCESS ? ERROR_GEN_FAILURE : error;
    }
    return ERROR_SUCCESS;
}
