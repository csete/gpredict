#!/usr/bin/env python3
"""Build a Windows cross-compile sysroot from MSYS2 repo packages.

Downloads the pacman sync DB of the MSYS2 mingw x86_64 repository,
resolves the transitive dependencies of the requested packages and
extracts them into a self-contained tree:

    <dest>/mingw64/{bin,include,lib,share}

The prefix baked into every .pc file is rewritten from /mingw64 to the
absolute destination path, so a plain pkg-config on any Linux host can
cross-build against the tree with the system mingw-w64 toolchain:

    ./configure --host=x86_64-w64-mingw32 \
        PKG_CONFIG_LIBDIR=<dest>/mingw64/lib/pkgconfig

Usage:
    msys2-sysroot.py DEST [PKG ...]

Default package list: gtk3 + libcurl + gettext-runtime (what Gpredict
needs). Pin exact package file names in CI for reproducible builds.
"""

import io
import os
import re
import subprocess
import sys
import tarfile
import urllib.request

REPO = "https://repo.msys2.org/mingw/x86_64"
DEFAULT_PKGS = [
    "mingw-w64-x86_64-gtk3",
    "mingw-w64-x86_64-curl",
    "mingw-w64-x86_64-gettext-runtime",
]


def fetch(url):
    req = urllib.request.Request(url, headers={"User-Agent": "gpredict-cross"})
    with urllib.request.urlopen(req) as r:
        return r.read()


def parse_desc(text):
    """Parse one pacman desc stanza -> (filename, depends, names)."""
    cur = None
    fname = None
    depends = []
    names = set()
    for line in text.splitlines():
        if line.startswith("%") and line.rstrip().endswith("%"):
            cur = line.strip()
            continue
        line = line.strip()
        if not line:
            continue
        if cur == "%FILENAME%":
            fname = line
        elif cur in ("%NAME%", "%NAMES%", "%PROVIDES%"):
            names |= {re.split(r"[=<>]", x)[0] for x in line.split()}
        elif cur == "%DEPENDS%":
            depends += line.split()
    return fname, depends, names


def parse_db(blob):
    """Parse pacman sync DB. Returns (entries, index).

    Modern MSYS2 sync DBs are zstd-compressed tar archives containing one
    `desc` file per package; older ones are a flat concatenated text DB.
    entries: {filename: {"depends": [...], "names": set()}}
    index:   {lowercased name -> filename}
    """
    entries = {}

    def add(text):
        fname, depends, names = parse_desc(text)
        if fname:
            entries[fname] = {"depends": depends, "names": names}

    if blob[:4] == b"\x28\xb5\x2f\xfd":  # zstd magic
        blob = subprocess.run(["zstd", "-dc"], input=blob, check=True,
                              stdout=subprocess.PIPE).stdout

    bio = io.BytesIO(blob)
    try:
        with tarfile.open(fileobj=bio, mode="r:") as tf:
            for member in tf:
                if member.name.endswith("/desc"):
                    add(tf.extractfile(member).read().decode("utf-8", "replace"))
        return _index(entries)
    except tarfile.TarError:
        pass

    bio.seek(0)
    add(bio.read().decode("utf-8", "replace"))
    return _index(entries)


def _index(entries):
    index = {}
    for fname, ent in entries.items():
        for name in ent["names"]:
            index.setdefault(name.lower(), fname)
    return entries, index


def resolve(entries, index, roots):
    selected = set()
    queue = [p.lower() for p in roots]
    missing = []
    while queue:
        name = queue.pop()
        fname = index.get(name)
        if fname is None:
            missing.append(name)
            continue
        if fname in selected:
            continue
        selected.add(fname)
        for dep in entries[fname]["depends"]:
            dep_name = re.split(r"[=<>]", dep)[0].lower()
            if dep_name.startswith("msys:"):
                continue  # MSYS(POSIX)-side runtime, not needed when cross-building
            queue.append(dep_name)
    if missing:
        print("unresolvable dependencies: " + " ".join(missing), file=sys.stderr)
        sys.exit(1)
    return selected


def main():
    if len(sys.argv) < 2:
        print(__doc__, file=sys.stderr)
        return 1
    dest = os.path.abspath(sys.argv[1])
    roots = sys.argv[2:] or DEFAULT_PKGS

    entries, index = parse_db(fetch(f"{REPO}/mingw64.db"))
    selected = resolve(entries, index, roots)
    print(f"{len(selected)} packages -> {dest}")

    os.makedirs(dest, exist_ok=True)
    for i, fname in enumerate(sorted(selected), 1):
        print(f"[{i}/{len(selected)}] {fname}")
        data = fetch(f"{REPO}/{fname}")
        subprocess.run(["tar", "--zstd", "-xf", "-", "-C", dest],
                       input=data, check=True)

    sysroot = os.path.join(dest, "mingw64")
    pcdir = os.path.join(sysroot, "lib", "pkgconfig")
    if os.path.isdir(pcdir):
        for pc in sorted(os.listdir(pcdir)):
            if not pc.endswith(".pc"):
                continue
            path = os.path.join(pcdir, pc)
            with open(path) as f:
                text = f.read()
            new = re.sub(r"(?<!\w)/mingw64", sysroot, text)
            if new != text:
                with open(path, "w") as f:
                    f.write(new)
    print(f"sysroot ready: {sysroot}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
