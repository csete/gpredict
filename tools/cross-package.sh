#!/bin/sh
# Assemble a standalone Windows package from a Linux cross-build.
#
#   cross-package.sh BUILDDIR SYSROOT OUTDIR
#
#   BUILDDIR  autotools build tree (top level of gpredict)
#   SYSROOT   tree produced by tools/msys2-sysroot.py (contains mingw64/);
#             when running inside MSYS2 itself, pass `/` (there the mingw64
#             tree lives at /mingw64)
#   OUTDIR    staging directory that is created and filled
#
# Mirrors the layout that src/compat.c and win32/Makefile expect:
# exe + DLLs at the root, share/gpredict/{data,pixmaps}, Adwaita icons,
# gdk-pixbuf loaders.

set -e

BUILDDIR=$(CDPATH= cd -- "$1" && pwd)
SYSROOT=$(CDPATH= cd -- "${2%/}/mingw64" && pwd)
if command -v realpath >/dev/null 2>&1; then
    OUT=$(realpath -m -- "$3")
else
    mkdir -p "$(dirname -- "$3")"
    OUT=$(CDPATH= cd -- "$(dirname -- "$3")" && pwd)/$(basename -- "$3")
fi

if [ -z "${OBJDUMP:-}" ]; then
    for cand in x86_64-w64-mingw32-objdump objdump; do
        if command -v "$cand" >/dev/null 2>&1; then OBJDUMP=$cand; break; fi
    done
fi
[ -n "$OBJDUMP" ] || { echo "no objdump found; set OBJDUMP" >&2; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/lib/gdk-pixbuf-2.0/2.10.0" "$OUT/share"

# --- executables -------------------------------------------------------------
if [ -f "$BUILDDIR/src/.libs/gpredict.exe" ]; then
    exe="$BUILDDIR/src/.libs/gpredict.exe"
else
    exe="$BUILDDIR/src/gpredict.exe"
fi
cp -p "$exe" "$OUT/"

# --- gdk-pixbuf loaders -------------------------------------------------------
# Staged *before* the closure walk and seeded as closure roots: loader
# DLLs are dlopen'ed by gdk-pixbuf at runtime, so they never show up in
# the exe's import table — and the SVG loader drags in the whole
# librsvg chain (librsvg-2-2.dll & friends) that would otherwise be
# missing, making all SVG loading fail with "Couldn't recognize the
# image file format".
if [ -d "$SYSROOT/lib/gdk-pixbuf-2.0/2.10.0/loaders" ]; then
    mkdir -p "$OUT/lib/gdk-pixbuf-2.0/2.10.0/loaders"
    cp -p "$SYSROOT"/lib/gdk-pixbuf-2.0/2.10.0/loaders/*pixbufloader*.dll \
          "$OUT/lib/gdk-pixbuf-2.0/2.10.0/loaders/"
fi

# --- DLL dependency closure ---------------------------------------------------
queue="$(basename "$exe")"
for l in "$OUT"/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll; do
    [ -e "$l" ] || continue
    queue="$queue lib/gdk-pixbuf-2.0/2.10.0/loaders/$(basename "$l")"
done
copied=""
while [ -n "$queue" ]; do
    next=""
    for cur in $queue; do
        for m in $("$OBJDUMP" -p "$OUT/$cur" 2>/dev/null | awk '/DLL Name/ {print $3}'); do
            m_lc=$(printf '%s' "$m" | tr '[:upper:]' '[:lower:]')
            case "$m_lc" in
                windows.system32*|windows\system32*|ntdll.dll|kernel32.dll|kernelbase.dll|\
                msvcrt.dll|advapi32.dll|ole32.dll|oleaut32.dll|user32.dll|gdi32.dll|\
                shell32.dll|ws2_32.dll|wldap32.dll|crypt32.dll|rpcrt4.dll|sechost.dll|\
                bcrypt.dll|dbghelp.dll|version.dll|winmm.dll|dwmapi.dll|\
                setupapi.dll|cfgmgr32.dll|hid.dll|winspool.drv|comdlg32.dll|\
                comctl32.dll|imm32.dll|uuid.dll|powrprof.dll|iphlpapi.dll|\
                dnsapi.dll|userenv.dll|profapi.dll|kernel.appcore.dll|\
                bcryptprimitives.dll|ucrtbase.dll|api-ms-win-*|ext-ms-*) continue ;;
            esac
            if [ -f "$SYSROOT/bin/$m" ]; then
                cp -p "$SYSROOT/bin/$m" "$OUT/"
                case " $copied " in *" $m "*) ;; *)
                    copied="$copied $m"
                    next="$next $m" ;;
                esac
            fi
        done
    done
    queue=$next
done

# --- data / pixmaps in the layout compat.c expects on Win32 -------------------
mkdir -p "$OUT/share/gpredict/data/satdata"
cp -p "$BUILDDIR/data/locations.dat" "$BUILDDIR/data/sample.qth" \
      "$BUILDDIR/data/Amateur.mod" "$OUT/share/gpredict/data/" 2>/dev/null || true
cp -p "$BUILDDIR"/data/satdata/*.cat "$BUILDDIR"/data/satdata/*.dat \
      "$OUT/share/gpredict/data/satdata/" 2>/dev/null || true
cp -rp "$BUILDDIR/pixmaps" "$OUT/share/gpredict/pixmaps"

# --- GTK theming / icons / schemas / helpers ----------------------------------
[ -d "$SYSROOT/etc" ] && cp -rp "$SYSROOT/etc" "$OUT/"
if [ -d "$SYSROOT/share/icons/Adwaita" ]; then
    mkdir -p "$OUT/share/icons/Adwaita"
    cp -rp "$SYSROOT/share/icons/Adwaita/scalable" "$OUT/share/icons/Adwaita/" 2>/dev/null || true
    cp -p "$SYSROOT/share/icons/Adwaita/index.theme" "$OUT/share/icons/Adwaita/" 2>/dev/null || true
fi
if [ -f "$SYSROOT/share/glib-2.0/schemas/gschemas.compiled" ]; then
    mkdir -p "$OUT/share/glib-2.0/schemas"
    cp -p "$SYSROOT/share/glib-2.0/schemas/gschemas.compiled" "$OUT/share/glib-2.0/schemas/"
fi
for h in gspawn-win64-helper.exe gspawn-win64-helper-console.exe; do
    [ -f "$SYSROOT/bin/$h" ] && cp -p "$SYSROOT/bin/$h" "$OUT/"
done

echo "staged: $OUT"
