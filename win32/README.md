# Native Windows Sleep Timer

A C++17 version using Win32 dialog controls, Core Audio for muting, and Windows
power management APIs. Requires Windows 10 or newer; Python, GTK, and NirCmd are
not needed. The executable embeds the existing application icon and a manifest
for native controls and DPI scaling.

## Build

Install Visual Studio with the **Desktop development with C++** workload and
CMake. From the repository root:

```powershell
cmake -S win32 -B win32/build -A x64
cmake --build win32/build --config Release
ctest --test-dir win32/build -C Release --output-on-failure
.\win32\build\Release\sleeptimer.exe
```

With Ninja, run CMake from a Visual Studio developer shell and replace `-A x64`
with `-G Ninja`; the executable will be directly in `win32/build`.

## Windows ARM64

To cross-compile locally, install Visual Studio's ARM64 C++ build tools and use a
separate build directory:

```powershell
cmake -S win32 -B win32/build/arm64 -G "Visual Studio 18 2026" -A ARM64
cmake --build win32/build/arm64 --config Release
```

The [Native Windows ARM64 workflow](../.github/workflows/win32-arm64.yml) builds
Release and runs the timer tests on a Windows ARM64 runner. It runs for relevant
pushes and pull requests, and can also be started manually from GitHub's Actions
tab. Download the `sleeptimer-windows-arm64` artifact from a successful run for a
ZIP containing `sleeptimer.exe`, this README, and the license.

## Use

Choose a duration and Standby, Hibernate, Shut down, or **Pause media**, and
optionally enable muting.
Press **Start** to count down, or **Stop** to cancel and retain the remaining time.
Time and action controls are locked during the countdown. Closing the window or
pressing Escape cancels the timer. The app must remain running for the timer to
perform its action.

The duration defaults to one hour and supports up to 99:59:59. Minutes and seconds
normalize when you leave the field; the arrow controls carry and borrow across
units. The countdown uses an elapsed-time deadline to avoid accumulating timer
message delays, including time spent suspended.

Starting a timer saves its initial duration, action, and mute option in
`%LOCALAPPDATA%\SleepTimer\settings.ini`. These preferences are separate from the
Python version's settings. Standby and Hibernate depend on the machine's power
configuration and Windows permissions. Shutdown allows other applications to
handle unsaved work. Muting affects the default multimedia output and remains
enabled after the power action. Windows errors are shown in a dialog.

When the timer expires, the app sends Windows' explicit media **Pause** command
before attempting the selected power action. Already-paused media stays paused.
The **Pause media** action only requests playback pause and optional muting, so
it can be used on tablets where sleep or hibernate returns `0x00000032`. Media
is also requested to pause if a subsequent power action fails.

The command goes to the foreground application and Windows' normal media-command
routing. Players must support the Windows Pause command; delivery does not
confirm that every browser tab or player has paused. Apps that ignore it may
continue playing. The Mute option additionally silences the default audio output.
See Microsoft's [media-command documentation](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-appcommand).

Standby detects the machine's power capabilities. On Modern Standby (S0
low-power idle) devices, including many ARM tablets, it requests display-off so
Windows can enter low-power idle according to its power policy. On traditional
sleep devices, it uses `SetSuspendState`. The display-off request does not report
whether the machine has reached its lowest-power state; Windows, drivers, and
background activity control that transition.

Older builds can fail with `0x00000032` (request not supported) on Modern Standby
devices. Rebuild or download a version with Modern Standby support. To check the
tablet's supported sleep modes, run `powercfg /a` in a terminal on that device.
`Standby (S0 Low Power Idle)` identifies Modern Standby. See Microsoft's
[power-state documentation](https://learn.microsoft.com/en-us/windows/win32/power/system-power-states)
and [Modern Standby guidance](https://learn.microsoft.com/en-au/answers/questions/809840/how-to-programatically-trigger-modern-standby-%28des).

## Safe verification

```powershell
.\win32\build\Release\sleeptimer.exe --dry-run
```

Dry-run mode uses the same countdown and displays the selected action when it
expires, while skipping media pause, muting, and power changes. It also skips saved
preferences. Set a short duration to check completion, or start a longer timer
and check that Stop and closing the window cancel it. CTest covers time
normalization, limits, deadline rounding, delayed timer messages, progress, and
media Pause delivery to test windows without affecting actual playback.
