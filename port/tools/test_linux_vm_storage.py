"""Storage regressions for the VM runner; no QEMU process is started."""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

import linux_vm_test as runner


class VMStorageTests(unittest.TestCase):
    def attempt_boot(self, share, *arguments):
        process = Mock()
        process.poll.return_value = 1
        console = Mock()
        console.expect.side_effect = RuntimeError('simulated boot failure')
        with patch.object(runner.sys, 'argv', ['linux_vm_test', '--share', str(share), *arguments]), \
                patch.object(runner.subprocess, 'Popen', return_value=process) as popen, \
                patch.object(runner.subprocess, 'check_call') as create, \
                patch.object(runner, 'Console', return_value=console):
            with self.assertRaisesRegex(RuntimeError, 'simulated boot failure'):
                runner.main()
        return popen.call_args.args[0], create

    def test_no_fetch_does_not_allocate_or_attach_outbox_even_if_boot_fails(self):
        with tempfile.TemporaryDirectory() as temporary:
            share = Path(temporary) / 'share'
            share.mkdir()
            (share / 'input.txt').write_text('fixture')
            command, _ = self.attempt_boot(share)
            self.assertFalse((share.parent / 'share-outbox.img').exists())
            self.assertFalse(any('outbox.img' in str(argument) for argument in command))
            self.assertTrue(share.with_suffix('.tar').is_file())

    def test_no_fetch_preserves_outbox_left_by_an_earlier_failed_fetch(self):
        with tempfile.TemporaryDirectory() as temporary:
            share = Path(temporary) / 'share'
            share.mkdir()
            outbox = share.parent / 'share-outbox.img'
            outbox.write_bytes(b'earlier fetch evidence')
            self.attempt_boot(share)
            self.assertEqual(outbox.stat().st_size, len(b'earlier fetch evidence'))
            self.assertEqual(outbox.read_bytes(), b'earlier fetch evidence')

    def test_new_persistent_disk_uses_16g_and_existing_disk_is_never_resized(self):
        with tempfile.TemporaryDirectory() as temporary:
            share = Path(temporary) / 'share'
            share.mkdir()
            disk = share.parent / 'persistent.qcow2'
            _, create = self.attempt_boot(share, '--disk', str(disk))
            self.assertEqual(create.call_args.args[0][-1], '16G')
            disk.write_bytes(b'existing image with its own capacity')
            _, create = self.attempt_boot(share, '--disk', str(disk), '--disk-size', '8G')
            create.assert_not_called()
            self.assertEqual(disk.read_bytes(), b'existing image with its own capacity')

    def test_fetch_keeps_transfer_capacity_and_failed_boot_evidence(self):
        with tempfile.TemporaryDirectory() as temporary:
            share = Path(temporary) / 'share'
            share.mkdir()
            command, _ = self.attempt_boot(share, '--fetch', '/root/package.zip')
            outbox = share.parent / 'share-outbox.img'
            self.assertEqual(outbox.stat().st_size, 1 << 30)
            self.assertTrue(any('outbox.img' in str(argument) for argument in command))


if __name__ == '__main__':
    unittest.main()
