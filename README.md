# Sleep Timer

<img title="Screenshot" src="screenshot.png" width="546px"/>

## Windows

### Native Win32 / WinAPI (C++)

The [native Windows version](win32/README.md) is in `win32/`. It uses Windows
controls and power/audio APIs directly, without Python, GTK, or NirCmd. See its
README for build instructions and a safe dry-run mode.

### Native Python

Install [Visual Studio 2015](https://www.visualstudio.com/de-de/downloads/download-visual-studio-vs.aspx)
with C++ support, [Python 3](https://www.python.org/downloads/) and [PyGObject for Windows](http://sourceforge.net/projects/pygobjectwin32/) with "Base packages" and "Gtk+".

### MSYS2

Open the Mingw64 shell and type:

```
pacman -S mingw-w64-x86_64-python3-gobject mingw-w64-x86_64-gtk3
python3 main.py
```

To create a distributable exe file:

```
pacman -S mingw-w64-x86_64-python3-cx_Freeze
python3 setup.py cx_Freeze
```
