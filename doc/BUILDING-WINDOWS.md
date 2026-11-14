# Building Gpredict for Windows

There are two maintained ways to build the Windows 64-bit `gpredict.exe`
(GTK-3, Win32 backend):

1. **Cross-build on Linux** — distro MinGW-w64 toolchain plus a
   dependency sysroot assembled from MSYS2 repository packages,
   section 5. This is the path CI uses: the `windows-cross` job in
   `.github/workflows/ci.yml` uploads the standalone zip as the
   `gpredict-windows-x86_64-cross` artifact.
2. **Native build on Windows with MSYS2/MinGW-w64**, sections 1-3.
   Convenient for day-to-day development on a Windows machine. CI does
   not run it: a Linux runner cross-compiling to Windows removes the
   need for a Windows build server, so there is no Windows CI job.

Both flows are verified with Gpredict 2.7 (`master`): the native build
with MSYS2 UCRT `mingw-w64-x86_64-gcc` 15.2 / `mingw-w64-x86_64-gtk3`
3.24.51, and the Linux cross-build on Ubuntu 24.04 with
`gcc-mingw-w64` 13 — the resulting exe and its standalone tree start
the GUI and load maps on a stock Windows install.

The older recipe in `win32/README` (cross-build against the
gtk+-bundle 3.10 zip from win32builder.gnome.org) still exists via
`win32/Makefile`, but it targets 2013-era GTK bundles. The two autotools
builds — native (sections 1-3) and Linux cross (section 5) — are the
maintained paths.

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
lookup in `compat.c` reads `share/gpredict/pixmaps`; see section 7.

## 5. Cross-building on Linux (MSYS2 sysroot)

Gpredict can be cross-compiled for Windows from any Linux box using the
system MinGW-w64 toolchain plus a Windows dependency sysroot assembled
on the fly from MSYS2 repository packages (they are plain `.pkg.tar.zst`
archives, no MSYS2 runtime involved):

```sh
sudo apt-get install mingw-w64 autoconf automake libtool intltool \
    autopoint gettext pkg-config python3 zstd zip

python3 tools/msys2-sysroot.py ../sysroot     # gtk3 + libcurl + gettext deps
./autogen.sh
./configure --host=x86_64-w64-mingw32 --prefix=/mingw64 \
    PKG_CONFIG_LIBDIR=../sysroot/mingw64/lib/pkgconfig
make -j$(nproc)

sh tools/cross-package.sh . ../sysroot stage/gpredict-x86_64-w64-mingw32
(cd stage && zip -qr ../gpredict-win64.zip gpredict-x86_64-w64-mingw32)
```

`tools/msys2-sysroot.py` resolves the transitive dependency set from the
repo's pacman DB and rewrites the `/mingw64` prefixes in the `.pc` files;
`tools/cross-package.sh` collects the exe, the DLL closure (via
`x86_64-w64-mingw32-objdump`, walking from the exe *and* from the
staged gdk-pixbuf loader DLLs so the SVG/librsvg chain ships),
pixmaps/data in the `compat.c` layout, gdk-pixbuf loaders, themes and
gspawn helpers. Pin package names/versions
as script arguments in CI for reproducibility. This is what the
`windows-cross` job in `.github/workflows/ci.yml` does; the workflow
uploads the zip as the `gpredict-windows-x86_64-cross` build artifact.

Debian/Ubuntu mingw-w64 builds link the exe against `msvcrt` while the
sysroot libraries are UCRT-built; this works because UCRT keeps the CRT
`FILE` ABI, and stdio across that boundary (a handle opened by glib's
`g_fopen` and closed by application code) was probed. If a real CRT
clash ever surfaces, swap the distro gcc for a UCRT-native toolchain
(e.g. llvm-mingw) — the sysroot and packaging flow stay the same.

The alternative historical path, `win32/Makefile` pointed at an existing
MSYS2 (`MINGW_ROOT=/mingw64` or `../../msys64/mingw64`), also works
inside an MSYS2 shell and additionally exercises the WinHTTP-based
`win32_fetch` downloader instead of libcurl.

## 6. Optional: cross-build variant (win32/Makefile, Windows host)

`win32/Makefile` remains for the historical cross/packaging build
(`make` / `make dist` from inside `win32/`, pointed at a GTK bundle or
at `MINGW_ROOT=/mingw64` in an MSYS2 shell — the latter works as-is
against current MSYS2 packages and `make dist` yields the standalone
`gpredict-<version>.zip`). It compiles the extra `win32/win32-fetch.c`
(WinHTTP-based downloader) and defines `HAVE_WIN32_FETCH` so the
TLE/TRSP fetchers use it instead of libcurl.

## 7. Packaging a standalone zip

The fastest path is `tools/cross-package.sh`; it automates everything
described below: the exe (taken from `src/.libs/`, avoiding the libtool
stub), the full DLL closure *including* the dlopen'ed gdk-pixbuf
loaders and their librsvg/SVG chain, the data in the `compat.c`
layout, icons, locales, and gtk schemas — deliberately *without* a
`loaders.cache`, so gdk-pixbuf relocates its loaders from the DLL
location at runtime:

```sh
sh tools/cross-package.sh . / stage/gpredict-x86_64-w64-mingw32
```

The second argument is `/`: inside an MSYS2 shell the dependency tree
already lives at `/mingw64`. (On Linux, pass the sysroot produced by
`tools/msys2-sysroot.py` instead — section 5.)

Then zip the stage directory — install `zip` (`pacman -S zip`) or use
PowerShell:

```sh
PSDIR=$(cygpath -w "$PWD/stage")
powershell.exe -NoProfile -Command \
  "Compress-Archive -Path '$PSDIR\\gpredict-x86_64-w64-mingw32' -DestinationPath '$(cygpath -w "$PWD")\\gpredict-x86_64-w64-mingw32.zip'"
```

The result (~30 MB, GUI-subsystem exe with embedded icon) runs from
any folder on a stock Windows install, no installer or runtime
needed.

Background — what must end up where: the autotools install follows
the Linux FHS layout (`share/pixmaps/gpredict/maps`), but the Win32
code in `compat.c` (and the `win32/Makefile` packaging) expects the
pixmaps tree under `share/gpredict/pixmaps`. When staging from the
autotools install manually, place it accordingly, e.g.:

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
