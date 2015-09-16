from cx_Freeze import setup, Executable
import os, site, sys

include_dll_path = os.path.join(site.getsitepackages()[1], "gnome")

missing_dll = [
    'libgtk-3-0.dll',
    'libgdk-3-0.dll',
    'libatk-1.0-0.dll',
    'libcairo-gobject-2.dll',
    'libgdk_pixbuf-2.0-0.dll',
    'libjpeg-8.dll',
    'libpango-1.0-0.dll',
    'libpangocairo-1.0-0.dll',
    'libpangoft2-1.0-0.dll',
    'libpangowin32-1.0-0.dll',
    'libjasper-1.dll',
    'librsvg-2-2.dll',
    'libharfbuzz-0.dll',
    'libtiff-5.dll',
    'libwebp-5.dll',
]

## We need to add all the libraries too (for themes, etc..)
gtk_libs = ['etc', 'lib', 'share/icons']


include_files = [(os.path.join(include_dll_path, x), x) for x in missing_dll]
include_files.append(('main.glade', 'main.glade'))

## Let's add gtk libraries folders and files
for lib in gtk_libs:
    include_files.append((os.path.join(include_dll_path, lib), lib))

buildOptions = dict(
    compressed=False,
    includes=["gi"],
    packages=["gi"],
    include_files=include_files
    )

setup(
    name="Sleeptimer",
    author="Jan Niklas Hasse",
    version="1.0",
    description="Sleeptimer",
    options={
        "build_exe": buildOptions,
    },
    executables=[
        Executable(
            "main.py",
            targetName="sleeptimer.exe",
            base="Win32GUI" if sys.platform == "win32" else None,
            icon="time.ico",
        )
    ]
)
