#!/usr/bin/env python3
"""Bounded UDP room relay for the original two-player link protocol.

J role code -> O count; A version region code[16] name[0..24];
L version padding -> R count entries; D payload -> the other room member.
List replies never exceed the request length or the game's 512-byte buffer.
Older 2-byte list requests receive an empty list (manual room codes still work).
No authentication/encryption: a public room code is not a password. Occupied
roles cannot be replaced; address changes must wait for the old lease to expire.
Run one process/port per game; never share this room namespace across games.
"""
import argparse
import asyncio
import collections
import re
import time

IDLE = 30
PUBLIC_FRESH = 5
LIST_MAX = 20
MAX_DATAGRAM = 513  # PORT_NETLINK_MAX + D prefix
LIST_BYTES = 512   # PORT_NETLINK_MAX in the client
RATE_IDLE = 60
CODE = re.compile(rb'[A-Z0-9]{1,16}\Z')


class Relay(asyncio.DatagramProtocol):
    def __init__(self, *, max_rooms=128, peer_timeout=IDLE, rate=600,
                 burst=1200, max_ips=4096):
        if min(max_rooms, peer_timeout, rate, burst, max_ips) <= 0:
            raise ValueError('relay limits must be positive')
        self.rooms = {}
        self.where = {}
        self.public = {}
        self.rates = collections.OrderedDict()
        self.max_rooms, self.peer_timeout = max_rooms, peer_timeout
        self.rate, self.burst, self.max_ips = rate, burst, max_ips
        self.global_tokens = 20000.0
        self.global_at = None
        self.next_sweep = 0.0
        self.stats = collections.Counter()

    def connection_made(self, transport):
        self.transport = transport

    def error_received(self, exc):
        # UDP ICMP errors affect a destination, not the relay's event loop.
        self.stats['socket_errors'] += 1

    def _allow(self, ip, now):
        if self.global_at is not None:
            self.global_tokens = min(20000, self.global_tokens + max(0, now-self.global_at)*10000)
        self.global_at = now
        if self.global_tokens < 1:
            return False
        self.global_tokens -= 1
        entry = self.rates.get(ip)
        if entry is None:
            if len(self.rates) >= self.max_ips:
                return False
            tokens = float(self.burst)
        else:
            tokens, at = entry
            tokens = min(self.burst, tokens + max(0, now-at)*self.rate)
        self.rates[ip] = (max(0, tokens-1), now)
        self.rates.move_to_end(ip)
        return tokens >= 1

    def _remove(self, addr):
        place = self.where.pop(addr, None)
        if place is None:
            return
        code, role = place
        room = self.rooms.get(code)
        if room is not None and room.get(role, (None,))[0] == addr:
            del room[role]
            if role == 1:
                self.public.pop(code, None)
            if not room:
                del self.rooms[code]

    def datagram_received(self, data, addr):
        if not 2 <= len(data) <= MAX_DATAGRAM:
            return
        now = time.monotonic()
        if now >= self.next_sweep:
            self.sweep(now)
            self.next_sweep = now + 1
        kind = data[:1]
        # Validate before allocating per-IP state.
        if kind == b'J':
            if not 3 <= len(data) <= 18 or data[1] not in (1, 2) or not CODE.fullmatch(data[2:]):
                return
        elif kind == b'A':
            if not 19 <= len(data) <= 43 or data[2] > 2:
                return
            raw = data[3:19].rstrip(b'\0')
            if not CODE.fullmatch(raw) or data[3:19] != raw.ljust(16, b'\0'):
                return
            try:
                name = data[19:].decode('utf-8')
            except UnicodeDecodeError:
                return
            if any(ord(c) < 32 or ord(c) == 127 for c in name):
                return
        elif kind == b'L':
            if len(data) > LIST_BYTES or any(data[2:]):
                return
        elif kind == b'D':
            if addr not in self.where:
                return
        else:
            return
        if not self._allow(addr[0], now):
            self.stats['rate_drops'] += 1
            return
        if kind == b'J':
            role, code = data[1], data[2:].decode('ascii')
            room = self.rooms.get(code, {})
            old = room.get(role)
            if old and old[0] != addr:
                self.stats['occupied'] += 1
                return
            if role == 2 and 1 not in room:
                return
            if code not in self.rooms and len(self.rooms) >= self.max_rooms:
                return
            # Bound identities behind one IP as well as total rooms. Allow NAT peers.
            if addr not in self.where and sum(a[0] == addr[0] for a in self.where) >= 16:
                return
            if self.where.get(addr) != (code, role):
                self._remove(addr)
            room = self.rooms.setdefault(code, {})
            room[role] = (addr, now)
            self.where[addr] = (code, role)
            self.transport.sendto(b'O' + bytes([len(room)]), addr)
        elif kind == b'A':
            version, region = data[1], data[2]
            code = data[3:19].rstrip(b'\0').decode('ascii')
            host = self.rooms.get(code, {}).get(1)
            if host and host[0] == addr:
                self.public[code] = (version, region, data[19:], now)
        elif kind == b'L':
            out, used = [], 2
            for code, (v, region, name, at) in self.public.items():
                if now-at > PUBLIC_FRESH or len(self.rooms.get(code, {})) != 1 or v != data[1]:
                    continue
                entry = bytes([region]) + code.encode('ascii').ljust(16, b'\0') + bytes([len(name)]) + name
                if used + len(entry) > min(len(data), LIST_BYTES):
                    continue
                out.append(entry)
                used += len(entry)
                if len(out) == LIST_MAX:
                    break
            self.transport.sendto(b'R' + bytes([len(out)]) + b''.join(out), addr)
        else:
            code, role = self.where[addr]
            room = self.rooms[code]
            # The reverse index is not itself proof of current room ownership.
            if room.get(role, (None,))[0] != addr:
                return
            room[role] = (addr, now)
            other = room.get(3-role)
            if other:
                self.transport.sendto(data, other[0])
                self.stats['forwarded'] += 1

    def sweep(self, now=None):
        now = time.monotonic() if now is None else now
        expired = [addr for room in self.rooms.values() for addr, at in room.values()
                   if now-at >= self.peer_timeout]
        for addr in expired:
            self._remove(addr)
        for code in [code for code, value in self.public.items() if now-value[3] > PUBLIC_FRESH]:
            self.public.pop(code, None)
        # Ordered by last packet; cleanup is bounded and does not scan busy identities.
        while self.rates:
            ip, (_, at) = next(iter(self.rates.items()))
            if now-at <= RATE_IDLE:
                break
            del self.rates[ip]


async def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--port', type=int, default=47800)
    ap.add_argument('--bind', default='0.0.0.0')
    ap.add_argument('--max-rooms', type=int, default=128)
    ap.add_argument('--peer-timeout', type=int, default=IDLE)
    ap.add_argument('--rate', type=int, default=600)
    ap.add_argument('--burst', type=int, default=1200)
    args = ap.parse_args()
    if not 1 <= args.port <= 65535 or min(args.max_rooms, args.peer_timeout, args.rate, args.burst) <= 0:
        ap.error('port must be 1..65535 and limits positive')
    loop = asyncio.get_running_loop()
    transport, relay = await loop.create_datagram_endpoint(
        lambda: Relay(max_rooms=args.max_rooms, peer_timeout=args.peer_timeout,
                      rate=args.rate, burst=args.burst), local_addr=(args.bind, args.port))
    print(f'netlink relay beta UDP {args.port}, max rooms {args.max_rooms}', flush=True)
    try:
        while True:
            await asyncio.sleep(1)
            relay.sweep()
    finally:
        transport.close()


if __name__ == '__main__':
    asyncio.run(main())
