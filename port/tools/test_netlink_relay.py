"""Behavioral contracts for the public relay; no game files or live services."""
import asyncio
import random
import socket
import unittest
from unittest.mock import patch

from netlink_relay import Relay

A = ('127.0.0.1', 10001)
B = ('127.0.0.1', 10002)
C = ('127.0.0.2', 10003)


class Transport:
    def __init__(self):
        self.sent = []

    def sendto(self, data, addr):
        self.sent.append((data, addr))


class Contracts(unittest.TestCase):
    def setUp(self):
        self.now = 100.0
        self.relay = Relay()
        self.transport = Transport()
        self.relay.connection_made(self.transport)
        self.clock = patch('netlink_relay.time.monotonic', side_effect=lambda: self.now)
        self.clock.start()
        self.addCleanup(self.clock.stop)

    def send(self, data, addr=A):
        self.relay.datagram_received(data, addr)

    def host(self, code=b'ROOM', addr=A):
        self.send(b'J\1' + code, addr)

    def announce(self, code=b'ROOM', addr=A, version=3):
        self.send(b'A' + bytes([version, 0]) + code.ljust(16, b'\0') + b'Player', addr)

    def test_active_role_cannot_be_stolen(self):
        self.host()
        self.host(addr=C)
        self.assertEqual(self.relay.rooms['ROOM'][1][0], A)
        self.assertNotIn(C, self.relay.where)

    def test_moving_address_leaves_no_ghost_in_old_room(self):
        self.host()
        self.announce()
        self.host(b'OTHER')
        self.assertNotIn('ROOM', self.relay.rooms)
        self.assertNotIn('ROOM', self.relay.public)
        self.assertEqual(self.relay.where[A], ('OTHER', 1))

    def test_failed_move_preserves_original_membership(self):
        self.host()
        self.host(b'OTHER', C)
        self.host(b'OTHER')
        self.assertEqual(self.relay.where[A], ('ROOM', 1))

    def test_expire_individual_dead_peer(self):
        self.host()
        self.send(b'J\2ROOM', B)
        self.now += 29
        self.host()
        self.now += 2
        self.relay.sweep()
        self.assertNotIn(B, self.relay.where)
        self.assertEqual(list(self.relay.rooms['ROOM']), [1])

    def test_room_capacity_is_bounded(self):
        self.relay = Relay(max_rooms=1)
        self.relay.connection_made(self.transport)
        self.host()
        self.host(b'OTHER', C)
        self.assertEqual(len(self.relay.rooms), 1)

    def test_invalid_frames_cannot_create_rooms_or_forward(self):
        for packet in (b'J\1', b'J\0ROOM', b'J\1bad!', b'J\1' + b'X'*17,
                       b'J\1\xff', b'J\1R\0OOM', b'J\2ORPHAN', b'Dpayload'):
            self.send(packet)
        self.assertEqual(self.relay.rooms, {})
        self.assertEqual(self.transport.sent, [])

    def test_public_list_has_no_amplification_and_filters_versions(self):
        self.host()
        self.announce()
        self.send(b'L\3', C)
        self.assertLessEqual(len(self.transport.sent[-1][0]), 2)
        self.send(b'L\3' + bytes(510), C)
        data = self.transport.sent[-1][0]
        self.assertEqual(data[:2], b'R\1')
        self.assertLessEqual(len(data), 512)
        self.send(b'L\4' + bytes(510), C)
        self.assertEqual(self.transport.sent[-1][0], b'R\0')

    def test_same_nat_clients_forward_and_other_room_is_isolated(self):
        self.host()
        self.send(b'J\2ROOM', B)
        self.host(b'OTHER', C)
        self.send(b'Dpayload', A)
        self.assertEqual(self.transport.sent[-1], (b'Dpayload', B))
        count = len(self.transport.sent)
        self.send(b'Dsecret', C)
        self.assertEqual(len(self.transport.sent), count)

    def test_oversize_and_invalid_announce_are_ignored(self):
        self.host()
        self.send(b'J\2ROOM', B)
        n = len(self.transport.sent)
        self.send(b'D' + bytes(513))
        self.send(b'A\3\x09' + b'ROOM'.ljust(16, b'\0') + b'Name')
        self.assertEqual(len(self.transport.sent), n)
        self.assertEqual(self.relay.public, {})

    def test_rate_limit_recovers_and_tracking_is_bounded(self):
        self.relay = Relay(rate=1, burst=2, max_ips=2)
        self.relay.connection_made(self.transport)
        for _ in range(6):
            self.send(b'L\3')
        self.assertEqual(len(self.transport.sent), 2)
        self.now += 1
        self.send(b'L\3')
        self.assertEqual(len(self.transport.sent), 3)
        for i in range(10):
            self.send(b'L\3', (f'10.0.0.{i}', 1000))
        self.assertLessEqual(len(self.relay.rates), 2)
        self.now += 61
        self.relay.sweep()
        self.assertEqual(self.relay.rates, {})

    def test_list_cannot_exceed_game_receive_buffer(self):
        for i in range(20):
            addr = (f'10.0.0.{i}', 2000)
            code = ('R%d' % i).encode()
            self.host(code, addr)
            self.send(b'A\3\0' + code.ljust(16, b'\0') + b'N'*24, addr)
        self.send(b'L\3' + bytes(510), C)
        self.assertLessEqual(len(self.transport.sent[-1][0]), 512)
        self.assertGreater(self.transport.sent[-1][0][1], 0)

    def test_malformed_fuzz_does_not_throw_or_grow_state(self):
        rng = random.Random(47800)
        for _ in range(5000):
            self.send(rng.randbytes(rng.randrange(1025)))
        self.assertLessEqual(len(self.relay.rooms), 128)


class SocketTest(unittest.IsolatedAsyncioTestCase):
    async def test_real_udp_two_players_and_room_listing(self):
        loop = asyncio.get_running_loop()
        transport, _ = await loop.create_datagram_endpoint(Relay, local_addr=('127.0.0.1', 0))
        self.addCleanup(transport.close)
        endpoint = transport.get_extra_info('sockname')
        clients = [socket.socket(socket.AF_INET, socket.SOCK_DGRAM) for _ in range(2)]
        for client in clients:
            client.setblocking(False)
            client.bind(('127.0.0.1', 0))
            self.addCleanup(client.close)

        async def exchange(client, packet):
            await loop.sock_sendto(client, packet, endpoint)
            return (await asyncio.wait_for(loop.sock_recvfrom(client, 512), 2))[0]

        self.assertEqual(await exchange(clients[0], b'J\1SOCKET'), b'O\1')
        self.assertEqual(await exchange(clients[1], b'J\2SOCKET'), b'O\2')
        await loop.sock_sendto(clients[0], b'Dreal-packet', endpoint)
        data, _ = await asyncio.wait_for(loop.sock_recvfrom(clients[1], 512), 2)
        self.assertEqual(data, b'Dreal-packet')


if __name__ == '__main__':
    unittest.main()
