#!/usr/bin/env python3
r"""A stand-in for Discord's local IPC, to check the game's Rich Presence (port/src/port_discord.c) without Discord.

Serves one Windows named pipe, answers the handshake with READY and writes every frame it receives to a JSON lines
file. Run it, then the game with KHCOM_DISCORD_IPC set to the same pipe and KHCOM_DISCORD_APP_ID to any number.

    python port/tools/fake_discord_ipc.py --out frames.jsonl --seconds 60     # pipe \\.\pipe\rekhcom-test-ipc
"""
import argparse
import ctypes
import json
import struct
import time
from ctypes import wintypes

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
k32.CreateNamedPipeW.restype = wintypes.HANDLE
k32.CreateNamedPipeW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD,
                                 wintypes.DWORD, wintypes.DWORD, wintypes.LPVOID]
k32.ConnectNamedPipe.argtypes = [wintypes.HANDLE, wintypes.LPVOID]
k32.ReadFile.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
k32.WriteFile.argtypes = [wintypes.HANDLE, wintypes.LPCVOID, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
k32.PeekNamedPipe.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.DWORD, wintypes.LPVOID,
                              ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
INVALID = wintypes.HANDLE(-1).value


def read(h, n):
    buf = ctypes.create_string_buffer(n)
    got = wintypes.DWORD()
    data = b''
    while len(data) < n:
        if not k32.ReadFile(h, ctypes.byref(buf, len(data)), n - len(data), ctypes.byref(got), None) or not got.value:
            return None
        data = buf.raw[:len(data) + got.value]
    return data


def write(h, op, payload):
    body = json.dumps(payload).encode()
    data = struct.pack('<II', op, len(body)) + body
    k32.WriteFile(h, data, len(data), ctypes.byref(wintypes.DWORD()), None)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--pipe', default=r'\\.\pipe\rekhcom-test-ipc')
    ap.add_argument('--out', required=True)
    ap.add_argument('--seconds', type=float, default=60)
    args = ap.parse_args()
    h = k32.CreateNamedPipeW(args.pipe, 3, 0, 1, 65536, 65536, 0, None)  # duplex, byte mode, blocking
    if h == INVALID:
        raise SystemExit(f'CreateNamedPipe failed ({ctypes.get_last_error()})')
    print('listening', args.pipe, flush=True)
    k32.ConnectNamedPipe(h, None)
    end = time.time() + args.seconds
    with open(args.out, 'w', encoding='utf-8') as out:
        while time.time() < end:
            avail = wintypes.DWORD()
            if not k32.PeekNamedPipe(h, None, 0, None, ctypes.byref(avail), None):
                break
            if avail.value < 8:
                time.sleep(0.05)
                continue
            op, size = struct.unpack('<II', read(h, 8))
            frame = json.loads(read(h, size))
            out.write(json.dumps({'t': round(time.time(), 2), 'op': op, 'frame': frame}) + '\n')
            out.flush()
            if op == 0:
                write(h, 1, {'cmd': 'DISPATCH', 'evt': 'READY', 'data': {'v': 1}})
            elif frame.get('cmd') == 'SET_ACTIVITY':
                write(h, 1, {'cmd': 'SET_ACTIVITY', 'nonce': frame.get('nonce'), 'data': {}})
    print('done', flush=True)


if __name__ == '__main__':
    main()
