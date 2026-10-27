"""Process-level launcher fallback contract, no ROM or game build required."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
host = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='rekhcom-boot-') as tmp:
    data = Path(tmp)
    marker = data / 'entered.txt'
    image = data / 'synthetic.bin'
    blob = bytearray(0x3000)
    struct.pack_into('<9I', blob, 0, 0x35474B52, 0x10003000, 0x10001000,
                     0x10002000, 0x10003000, 0x10008000, 0x10010000, 0x10010000, 0)
    blob[0x1000:0x1006] = bytes.fromhex('b851000000c3')  # mov eax,81; ret (guest-requested quit-all)
    image.write_bytes(blob)
    env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_HOST_HIDDEN='1')
    def run(path, extra=None):
        result = subprocess.run([str(host), str(path), '--data-dir', str(data), '--boot-status', str(marker)],
                                env=dict(env, **(extra or {})), stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                creationflags=0x08004000, timeout=15)
        return result.returncode
    marker.write_text('stale')
    assert run(data / 'missing.bin') == 2
    assert not marker.exists(), 'stale marker survived pre-guest error'
    assert run(image, {'SDL_RENDER_DRIVER': 'nonexistent-rekhcom-driver'}) == 4
    assert not marker.exists(), 'pre-renderer failure marked guest entered'
    assert run(image) == 81
    assert marker.read_bytes() == b'guest-entered\n', 'guest quit must suppress fallback'
print('Windows64 boot marker process contract passed')
