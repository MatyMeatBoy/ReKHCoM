"""Test sweeps' case folders: one copy of the game per sweep (<work>/_bin), each case gets hard links to it.

A copy per case filled the disk (23 MB x hundreds of cases); the shared copy also keeps the build's own khcom.exe
free to relink while a sweep runs. Where links fail (another drive, FAT) the case gets a plain copy as before.
"""
import os
import shutil
import threading

_lock = threading.Lock()


def place(exe_dir, work, case_dir, files=('khcom.exe', 'SDL3.dll')):
    shared = work / '_bin'
    with _lock:
        shared.mkdir(parents=True, exist_ok=True)
        for f in files:
            src, dst = exe_dir / f, shared / f
            s = src.stat()
            if not dst.exists() or (dst.stat().st_size, int(dst.stat().st_mtime)) != (s.st_size, int(s.st_mtime)):
                try:
                    shutil.copy2(src, dst)
                except PermissionError:
                    # the build was relinked mid-sweep while cases still run the shared copy (hard links): the
                    # sweep goes on with the copy it started with (23/10: events jp stopped here)
                    if not dst.exists():
                        raise
    for f in files:
        try:
            os.link(shared / f, case_dir / f)
        except OSError:
            shutil.copy2(shared / f, case_dir / f)
