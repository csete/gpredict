# Building Gpredict for Windows (native MSYS2/MinGW-w64)

This describes a **native 64-bit build on Windows** using MSYS2. It
produces a real `gpredict.exe` (GTK-3, Win32 backend) and was verified
with Gpredict 2.7 (`master`), MSYS2 U/CRT (ucrt) toolchain
`mingw-w64-x86_64-gcc` 15.2 and `mingw-w64-x86_64-gtk3` 3.24.51.

The older recipe in `win32/README` (cross-build against the
gtk+-bundle 3.10 zip from win32builder.gnome.org) still exists via
`win32/Makefile`, but it targets 2013-era GTK bundles. The MSYS2
autotools build below is the maintained path.

## 1. Prerequisites

1. Install [MSYS2](https://www.msys2.org/) (default location `C:\msys64`).
2. Open an **MSYS2 MinGW x64** shell (Start menu shortcut, or
   `C:\msys64\msys2_shell.cmd -mingw64`) and install the toolchain and
   dependencies:

   ```sh
   pacman -Syu                 # update core, restart shell if asked
   pacman -S --needed \
     base-devel \
     git \
     intltool \
     mingw-w64-x86_64-toolchain \
     mingw-w64-x86_64-pkgconf \
     mingw-w64-x86_64-glib2 \
     mingw-w64-x86_64-gtk3 \
     mingw-w64-x86_64-libcurl \
     mingw-w64-x86_64-gettext-runtime \
     mingw-w64-x86_64-gpsd      # optional: GPS support via libgps
   ```

   Notes:
   - `base-devel` provides the MSYS-side autotools
     (autoconf/automake/libtool/intltool/gettext/autopoint).
   - `zip` (MSYS package) is only needed by `win32/Makefile`'s `dist`
     target; any zip tool works as a stand-in (e.g. `tar -a -cf`).
   - The MinGW packages (prefixed `mingw-w64-x86_64-`) provide the
     *target* libraries the binary links against. Never link against
     MSYS (`/usr/bin`) DLLs — the resulting exe would require the MSYS
     runtime and would not be portable.

## 2. Configure and build

```sh
cd gpredict
./autogen.sh --prefix=/mingw64
make -j4
```

The binary is `src/gpredict.exe`.

## 3. Install and run

Install into the MinGW tree so the executable sits next to its
`share/gpredict` data (maps, TLE sources, sample QTH/modules), which is
how the Win32 build locates its data files
(`g_win32_get_package_installation_directory_of_module`):

```sh
make install              # installs under C:\msys64\mingw64
gpredict.exe              # from an MSYS2 MinGW x64 shell
```

The first run creates `satdata/`, `modules/`, `hwconf/`, `trsp/` under
`%APPDATA%`-based config dirs and populates them from the shipped
defaults; logs land under the user data dir.

**Gotcha:** depending on the libtool version, `make` leaves the *real*
binary at `src/.libs/gpredict.exe` while `src/gpredict.exe` is a
libtool exec-wrapper stub that may fail silently (exits instantly with
code 0 and no output) when run outside an environment matching the
stub's assumptions. If running `src/gpredict.exe` "does nothing", run
`src/.libs/gpredict.exe` instead — or just `make install` and use
`/mingw64/bin/gpredict.exe`.

## 4. Known Windows quirks handled by the build

These are fixed in-tree; listed here as documentation of why the code
looks the way it does:

| Issue | Fix |
|---|---|
| `#ifdef WIN32` guards never trigger under `-std=c11`: GCC for MinGW only predefines `WIN32` in GNU dialects, never in strict ISO mode | Socket/OS guards changed to `_WIN32` (always predefined by the compiler) in `main.c`, `gtk-rig-ctrl.c`, `gtk-rot-ctrl.c`, `sat-info.c`, `sgpsdp/sgp_time.c` |
| MinGW's `curl/curl.h` itself defines `WIN32` after inclusion, silently switching later `#ifdef WIN32` blocks to the `win32_fetch` backend even when libcurl is linked | TLE/TRSP download code now selects its backend with the build-system macro `HAVE_WIN32_FETCH` (`tle-update.c`, `trsp-update.c`); `win32/Makefile` passes `-DHAVE_WIN32_FETCH` |
| `strcasestr()` is a glibc extension, absent from the Windows CRT | Call sites in `gtk-rig-ctrl.c`/`gtk-rot-ctrl.c` use the existing portable `gpredict_strcasestr()` helper |
| `timegm()` is not declared on Windows unless included as `_mkgmtime` | `sat-info.c` `#define timegm _mkgmtime` guard now triggers on `_WIN32` |
| Winsock2 functions need `ws2_32` on Windows but not on Unix | `configure.ac` uses `AC_SEARCH_LIBS([htons],[ws2_32])` |
| Double-clicking `gpredict.exe` popped up a console window and showed no file icon | On a MinGW host the autotools build links with `-mwindows` and compiles the pre-existing `win32/gpredict.rc` (icon + version info) with `windres` into the exe (`src/Makefile.am`) |

`gmtime_r()` (`sgpsdp/sgp_time.c`) is shimmed on Windows via the same
`_WIN32` guard; the UCRT also provides it, so the shim is harmless.

Pixmaps install under `share/pixmaps/gpredict` (FHS) but the Win32 data
lookup in `compat.c` reads `share/gpredict/pixmaps`; see section 6.

## 5. Optional: cross-build variant

`win32/Makefile` remains for the historical cross/packaging build
(`make` / `make dist` from inside `win32/`, pointed at a GTK bundle or
at `MINGW_ROOT=/mingw64` in an MSYS2 shell — the latter works as-is
against current MSYS2 packages and `make dist` yields the standalone
`gpredict-<version>.zip`). It compiles the extra `win32/win32-fetch.c`
(WinHTTP-based downloader) and defines `HAVE_WIN32_FETCH` so the
TLE/TRSP fetchers use it instead of libcurl.

## 6. Packaging a standalone zip

The autotools install follows the Linux FHS layout (`share/pixmaps/
gpredict/maps`), but the Win32 code in `compat.c` (and the `win32/Makefile`
packaging) expects the pixmaps tree under `share/gpredict/pixmaps`. When
staging from the autotools install, place it accordingly, e.g.:

```sh
cp -rp /mingw64/share/pixmaps/gpredict /mingw64/share/gpredict/pixmaps
```

Then ship `bin/gpredict.exe` together with the required MinGW DLLs
(`ldd /mingw64/bin/gpredict.exe` lists them) plus `share/gpredict`
(data, pixmaps, TLE templates), `share/icons/Adwaita`, `share/locale`,
the gdk-pixbuf loaders under `lib/gdk-pixbuf-2.0` (with a regenerated
`loaders.cache`) and `gspawn-win*-helper.exe` next to the exe for
hyperlinks. `win32/Makefile`'s `dist` target automates exactly this
collection; mirror it if you script your own packaging.
