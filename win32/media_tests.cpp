#include "media.h"
#include <iostream>

namespace {
struct Player {
    bool playing = true;
    unsigned pauseRequests = 0;
    unsigned otherCommands = 0;
    HWND source = nullptr;
};

LRESULT CALLBACK playerProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    if (message == WM_APPCOMMAND) {
        auto* player = reinterpret_cast<Player*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        player->source = reinterpret_cast<HWND>(wParam);
        if (GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_MEDIA_PAUSE) {
            player->playing = false;
            ++player->pauseRequests;
        } else {
            ++player->otherCommands;
        }
        // Consume every media command so tests cannot affect other applications.
        return TRUE;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
}

int main() {
    int failures = 0;
    const auto check = [&](bool result, const char* description) {
        if (!result) { std::cerr << description << '\n'; ++failures; }
    };
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW type = {};
    type.lpfnWndProc = playerProc;
    type.hInstance = instance;
    type.lpszClassName = L"SleepTimerMediaTestPlayer";
    if (!RegisterClassW(&type)) return 1;

    Player player;
    // Message-only windows do not show UI or participate in the shell's media routing.
    const HWND window = CreateWindowExW(0, type.lpszClassName, L"", 0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, instance, &player);
    if (!window) return 1;
    check(requestMediaPause(window, window) == ERROR_SUCCESS, "pause message is delivered");
    check(!player.playing && player.pauseRequests == 1, "playing media receives Pause");
    check(player.source == window, "originating window is preserved");
    check(requestMediaPause(window, window) == ERROR_SUCCESS, "repeat request is delivered");
    check(!player.playing && player.pauseRequests == 2 && player.otherCommands == 0,
        "already paused media never receives a Play/Pause toggle");

    WNDCLASSW childType = {};
    childType.lpfnWndProc = DefWindowProcW;
    childType.hInstance = instance;
    childType.lpszClassName = L"SleepTimerMediaTestChild";
    if (!RegisterClassW(&childType)) return 1;
    const HWND child = CreateWindowExW(0, childType.lpszClassName, L"", WS_CHILD,
        0, 0, 0, 0, window, nullptr, instance, nullptr);
    check(child != nullptr, "child receiver is created");
    if (child) {
        check(requestMediaPause(child, child) == ERROR_SUCCESS, "child receives pause request");
        check(player.pauseRequests == 3 && player.source == child,
            "unhandled command forwards to parent with its origin intact");
    }
    check(requestMediaPause(nullptr, window) == ERROR_INVALID_WINDOW_HANDLE,
        "missing target reports failure");
    DestroyWindow(window);
    UnregisterClassW(childType.lpszClassName, instance);
    UnregisterClassW(type.lpszClassName, instance);
    return failures == 0 ? 0 : 1;
}
