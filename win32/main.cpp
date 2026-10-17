#include <windows.h>
#include <commctrl.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <powrprof.h>
#include <shellapi.h>
#include <shlobj.h>
#include <algorithm>
#include <cwchar>
#include <string>

#include "resource.h"
#include "timer.h"

namespace {
constexpr UINT_PTR countdownTimer = 1;
constexpr int timeControls[] = {IDC_HOURS, IDC_MINUTES, IDC_SECONDS};
constexpr int optionControls[] = {IDC_HOURS, IDC_MINUTES, IDC_SECONDS,
    IDC_HOURS_SPIN, IDC_MINUTES_SPIN, IDC_SECONDS_SPIN,
    IDC_STANDBY, IDC_HIBERNATE, IDC_SHUTDOWN, IDC_MUTE};

struct App {
    bool dryRun = false;
    bool running = false;
    bool updating = false;
    unsigned total = 0;
    ULONGLONG deadline = 0;
    std::wstring settings;
    const wchar_t* title() const { return dryRun ? L"Sleep Timer (dry run)" : L"Sleep Timer"; }
};

void showError(HWND window, const wchar_t* operation, DWORD code) {
    wchar_t* detail = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0,
        reinterpret_cast<wchar_t*>(&detail), 0, nullptr);
    wchar_t number[32];
    swprintf_s(number, L"\nError 0x%08lX", code);
    const std::wstring message = std::wstring(operation) + L"\n\n" +
        (detail ? detail : L"Windows did not provide an error description.") + number;
    LocalFree(detail);
    MessageBoxW(window, message.c_str(), L"Sleep Timer", MB_OK | MB_ICONERROR);
}

bool readDuration(HWND window, unsigned& seconds) {
    unsigned values[3] = {};
    for (int i = 0; i < 3; ++i) {
        BOOL valid = FALSE;
        values[i] = GetDlgItemInt(window, timeControls[i], &valid, FALSE);
        if (!valid || values[i] > 99999) return false;
    }
    seconds = timer::duration(values[0], values[1], values[2]);
    return true;
}

void displayDuration(HWND window, App& app, unsigned seconds) {
    app.updating = true;
    SetDlgItemInt(window, IDC_HOURS, seconds / 3600, FALSE);
    SetDlgItemInt(window, IDC_MINUTES, seconds / 60 % 60, FALSE);
    SetDlgItemInt(window, IDC_SECONDS, seconds % 60, FALSE);
    app.updating = false;
}

void updateStart(HWND window, const App& app) {
    unsigned seconds = 0;
    EnableWindow(GetDlgItem(window, IDOK), app.running ||
        (readDuration(window, seconds) && seconds > 0));
}

int selectedMode(HWND window) {
    if (IsDlgButtonChecked(window, IDC_HIBERNATE) == BST_CHECKED) return IDC_HIBERNATE;
    if (IsDlgButtonChecked(window, IDC_SHUTDOWN) == BST_CHECKED) return IDC_SHUTDOWN;
    return IDC_STANDBY;
}

bool settingsPath(HWND window, App& app) {
    PWSTR localAppData = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE,
                                                nullptr, &localAppData);
    if (FAILED(result)) {
        showError(window, L"Could not locate the settings folder.", result);
        return false;
    }
    const std::wstring directory = std::wstring(localAppData) + L"\\SleepTimer";
    CoTaskMemFree(localAppData);
    if (!CreateDirectoryW(directory.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
        showError(window, L"Could not create the settings folder.", GetLastError());
        return false;
    }
    app.settings = directory + L"\\settings.ini";
    return true;
}

void loadSettings(HWND window, App& app) {
    unsigned seconds = 3600;
    int mode = IDC_STANDBY;
    bool mute = false;
    // A dry run neither reads nor writes the user's saved preferences.
    if (!app.dryRun && settingsPath(window, app)) {
        seconds = std::min(GetPrivateProfileIntW(L"default", L"seconds", 3600,
                                                app.settings.c_str()), timer::maxSeconds);
        wchar_t value[32];
        GetPrivateProfileStringW(L"default", L"mode", L"standby", value, 32, app.settings.c_str());
        if (wcscmp(value, L"hibernate") == 0) mode = IDC_HIBERNATE;
        if (wcscmp(value, L"shutdown") == 0) mode = IDC_SHUTDOWN;
        GetPrivateProfileStringW(L"default", L"mute", L"False", value, 32, app.settings.c_str());
        mute = _wcsicmp(value, L"True") == 0;
    }
    displayDuration(window, app, seconds);
    CheckRadioButton(window, IDC_STANDBY, IDC_SHUTDOWN, mode);
    CheckDlgButton(window, IDC_MUTE, mute ? BST_CHECKED : BST_UNCHECKED);
}

bool saveSettings(HWND window, App& app, unsigned seconds) {
    if (app.dryRun) return true;
    if (app.settings.empty() && !settingsPath(window, app)) return false;
    const int mode = selectedMode(window);
    const wchar_t* name = mode == IDC_HIBERNATE ? L"hibernate" :
        mode == IDC_SHUTDOWN ? L"shutdown" : L"standby";
    if (!WritePrivateProfileStringW(L"default", L"seconds", std::to_wstring(seconds).c_str(), app.settings.c_str()) ||
        !WritePrivateProfileStringW(L"default", L"mode", name, app.settings.c_str()) ||
        !WritePrivateProfileStringW(L"default", L"mute",
            IsDlgButtonChecked(window, IDC_MUTE) == BST_CHECKED ? L"True" : L"False", app.settings.c_str())) {
        showError(window, L"Could not save the timer settings.", GetLastError());
        return false;
    }
    return true;
}

// Restore the process token's original privilege state on every return path.
struct ShutdownPrivilege {
    HANDLE token = nullptr;
    TOKEN_PRIVILEGES previous = {};
    bool enabled = false;

    DWORD enable() {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            return GetLastError();
        TOKEN_PRIVILEGES requested = {};
        requested.PrivilegeCount = 1;
        requested.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!LookupPrivilegeValueW(nullptr, SE_SHUTDOWN_NAME, &requested.Privileges[0].Luid))
            return GetLastError();
        DWORD size = sizeof(previous);
        if (!AdjustTokenPrivileges(token, FALSE, &requested, sizeof(previous), &previous, &size))
            return GetLastError();
        const DWORD error = GetLastError();
        enabled = error == ERROR_SUCCESS;
        return error;
    }

    ~ShutdownPrivilege() {
        if (enabled) AdjustTokenPrivileges(token, FALSE, &previous, 0, nullptr, nullptr);
        if (token) CloseHandle(token);
    }
};

HRESULT muteAudio() {
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioEndpointVolume* volume = nullptr;
    HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&enumerator));
    if (SUCCEEDED(result)) result = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
    if (SUCCEEDED(result)) result = device->Activate(__uuidof(IAudioEndpointVolume),
        CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void**>(&volume));
    if (SUCCEEDED(result)) result = volume->SetMute(TRUE, nullptr);
    if (volume) volume->Release();
    if (device) device->Release();
    if (enumerator) enumerator->Release();
    return result;
}

void stopTimer(HWND window, App& app) {
    KillTimer(window, countdownTimer);
    app.running = false;
    for (int id : optionControls) EnableWindow(GetDlgItem(window, id), TRUE);
    SetDlgItemTextW(window, IDOK, L"&Start");
    SetDlgItemTextW(window, IDC_STATUS, app.dryRun ? L"Dry run: power and audio skipped" : L"Ready");
    SetWindowTextW(window, app.title());
    SendDlgItemMessageW(window, IDC_PROGRESS, PBM_SETPOS, 0, 0);
    updateStart(window, app);
}

void finishTimer(HWND window, App& app) {
    // Capture the selected action before re-enabling controls, which can move
    // keyboard focus and update an automatic radio-button group.
    const int mode = selectedMode(window);
    const bool mute = IsDlgButtonChecked(window, IDC_MUTE) == BST_CHECKED;
    stopTimer(window, app);
    CheckRadioButton(window, IDC_STANDBY, IDC_SHUTDOWN, mode);
    if (app.dryRun) {
        const std::wstring message = std::wstring(L"Dry run complete: ") +
            (mode == IDC_HIBERNATE ? L"hibernate" : mode == IDC_SHUTDOWN ? L"shutdown" : L"standby") +
            (mute ? L" with mute." : L".") +
            L"\nPower and audio changes were skipped.";
        MessageBoxW(window, message.c_str(), L"Sleep Timer - dry run", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ShutdownPrivilege privilege;
    const DWORD error = privilege.enable();
    if (error != ERROR_SUCCESS) {
        showError(window, L"Could not enable the Windows shutdown privilege.", error);
        return;
    }
    if (mute) {
        const HRESULT result = muteAudio();
        if (FAILED(result)) {
            showError(window, L"Could not mute the default audio output. The power action was cancelled.", result);
            return;
        }
    }
    const BOOL success = mode == IDC_SHUTDOWN
        ? ExitWindowsEx(EWX_POWEROFF, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_FLAG_PLANNED)
        : SetSuspendState(mode == IDC_HIBERNATE, FALSE, FALSE);
    if (!success) {
        showError(window, L"The requested power action failed.", GetLastError());
        return;
    }
    EndDialog(window, 0);
}

void startTimer(HWND window, App& app) {
    unsigned seconds = 0;
    if (!readDuration(window, seconds) || seconds == 0) return;
    displayDuration(window, app, seconds);
    if (!saveSettings(window, app, seconds)) return;
    if (!SetTimer(window, countdownTimer, 100, nullptr)) {
        showError(window, L"Could not start the countdown.", GetLastError());
        return;
    }
    app.total = seconds;
    app.deadline = GetTickCount64() + static_cast<ULONGLONG>(seconds) * 1000;
    app.running = true;
    // Starting with Enter may leave focus on a radio button. Move focus before
    // disabling it so the dialog manager cannot select another power action.
    SetFocus(GetDlgItem(window, IDOK));
    for (int id : optionControls) EnableWindow(GetDlgItem(window, id), FALSE);
    SetDlgItemTextW(window, IDOK, L"&Stop");
    SetDlgItemTextW(window, IDC_STATUS, app.dryRun ? L"Dry run countdown" : L"Countdown running");
    SendDlgItemMessageW(window, IDC_PROGRESS, PBM_SETPOS, 10000, 0);
}

INT_PTR CALLBACK dialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(window, DWLP_USER));
    if (message == WM_INITDIALOG) {
        app = reinterpret_cast<App*>(lParam);
        SetWindowLongPtrW(window, DWLP_USER, lParam);
        SetWindowTextW(window, app->title());
        HICON icon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_TIMER));
        SendMessageW(window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
        SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
        for (int i = 0; i < 3; ++i) {
            SendDlgItemMessageW(window, timeControls[i], EM_SETLIMITTEXT, i == 0 ? 2 : 5, 0);
            SendDlgItemMessageW(window, IDC_HOURS_SPIN + i, UDM_SETBUDDY,
                reinterpret_cast<WPARAM>(GetDlgItem(window, timeControls[i])), 0);
            SendDlgItemMessageW(window, IDC_HOURS_SPIN + i, UDM_SETRANGE32, 0, i == 0 ? 99 : 59);
        }
        SendDlgItemMessageW(window, IDC_PROGRESS, PBM_SETRANGE32, 0, 10000);
        loadSettings(window, *app);
        stopTimer(window, *app);
        return TRUE;
    }
    if (!app) return FALSE;
    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED) {
            if (app->running) stopTimer(window, *app);
            else startTimer(window, *app);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            KillTimer(window, countdownTimer);
            EndDialog(window, 0);
            return TRUE;
        }
        if (LOWORD(wParam) >= IDC_HOURS && LOWORD(wParam) <= IDC_SECONDS &&
            !app->updating && !app->running) {
            if (HIWORD(wParam) == EN_KILLFOCUS) {
                unsigned seconds = 0;
                if (readDuration(window, seconds)) displayDuration(window, *app, seconds);
            }
            updateStart(window, *app);
        }
        break;
    case WM_NOTIFY: {
        const auto* header = reinterpret_cast<const NMHDR*>(lParam);
        if (header->code == UDN_DELTAPOS && header->idFrom >= IDC_HOURS_SPIN &&
            header->idFrom <= IDC_SECONDS_SPIN) {
            const auto* delta = reinterpret_cast<const NMUPDOWN*>(lParam);
            unsigned seconds = 0;
            if (!app->running && readDuration(window, seconds)) {
                const int unit = header->idFrom == IDC_HOURS_SPIN ? 3600 :
                    header->idFrom == IDC_MINUTES_SPIN ? 60 : 1;
                displayDuration(window, *app, timer::duration(0, 0,
                    static_cast<long long>(seconds) + static_cast<long long>(delta->iDelta) * unit));
                updateStart(window, *app);
            }
            SetWindowLongPtrW(window, DWLP_MSGRESULT, 1);
            return TRUE;
        }
        break;
    }
    case WM_TIMER:
        if (wParam == countdownTimer && app->running) {
            const unsigned remaining = timer::secondsLeft(app->deadline, GetTickCount64());
            displayDuration(window, *app, remaining);
            SendDlgItemMessageW(window, IDC_PROGRESS, PBM_SETPOS, timer::progress(remaining, app->total), 0);
            wchar_t title[96];
            swprintf_s(title, L"%02u:%02u:%02u - %ls", remaining / 3600, remaining / 60 % 60,
                remaining % 60, app->title());
            SetWindowTextW(window, title);
            if (remaining == 0) finishTimer(window, *app);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        KillTimer(window, countdownTimer);
        EndDialog(window, 0);
        return TRUE;
    }
    return FALSE;
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    App app;
    int argc = 0;
    PWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 1;
    bool validArguments = true;
    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"--dry-run") == 0) app.dryRun = true;
        else validArguments = false;
    }
    LocalFree(argv);
    if (!validArguments) {
        MessageBoxW(nullptr, L"Usage: sleeptimer.exe [--dry-run]", L"Sleep Timer", MB_OK | MB_ICONINFORMATION);
        return 1;
    }
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com)) {
        showError(nullptr, L"Could not initialize Windows audio services.", com);
        return 1;
    }
    INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_UPDOWN_CLASS | ICC_PROGRESS_CLASS};
    if (!InitCommonControlsEx(&controls)) {
        showError(nullptr, L"Could not initialize Windows controls.", GetLastError());
        CoUninitialize();
        return 1;
    }
    const INT_PTR result = DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_TIMER), nullptr,
                                           dialogProc, reinterpret_cast<LPARAM>(&app));
    if (result == -1) showError(nullptr, L"Could not create the Sleep Timer window.", GetLastError());
    CoUninitialize();
    return result == -1 ? 1 : 0;
}
