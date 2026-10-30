"""Run the native Linux launcher's CLI in disposable data directories."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT/'port/linux/launcher/rekhcom_launcher.py'


class NativeOnline(unittest.TestCase):
    def test_select_beta_disable_and_preserve_other_settings(self):
        with tempfile.TemporaryDirectory() as temporary:
            data = Path(temporary)
            ini = data/'kh_com.ini'
            ini.write_bytes(b'online_name=Player\ngraphics_mode=1\n')
            (data/'netlink.cfg').write_text('lobby:custom.example:1234\n')
            for option, expected in [('--online-beta', 'lobby:45.8.207.150:47800'), ('--online-off', 'off')]:
                result = subprocess.run([sys.executable, str(SCRIPT), '--data', str(data), option],
                                        capture_output=True, text=True, timeout=10)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual((data/'netlink.cfg').read_text().strip(), expected)
                self.assertEqual(ini.read_bytes(), b'online_name=Player\ngraphics_mode=1\n')
                self.assertFalse(list(data.glob('.netlink-*')))

    def test_no_selection_does_not_change_custom_relay(self):
        with tempfile.TemporaryDirectory() as temporary:
            data = Path(temporary)
            config = data/'netlink.cfg'
            config.write_bytes(b'relay:custom:47801:ROOM:host\n')
            result = subprocess.run([sys.executable, str(SCRIPT), '--data', str(data), '--help'],
                                    capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 0)
            self.assertEqual(config.read_bytes(), b'relay:custom:47801:ROOM:host\n')


if __name__ == '__main__':
    unittest.main()
