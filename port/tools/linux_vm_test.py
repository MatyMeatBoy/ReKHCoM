"""Smoke-test a Linux bundle inside a QEMU VM (Debian nocloud image).

The shared folder is exposed read-only as a FAT disk; the VM copies it to
its own disk, runs the commands given with --run, and writes the files listed
with --fetch as a tar archive onto a raw "outbox" disk, which is read back here
and saved to --out (the serial console is read a byte at a time: a 28 MB PS5
package in base64 through it took hours).

    python port/tools/linux_vm_test.py --share <work>/linux/vmshare \
        --run "cd /root/share/ReKHCoM && KHCOM_FAST=1 ./ReKHCoM.sh" --fetch /root/share/ReKHCoM/kh_com.log
"""
import argparse
import queue
import re
import subprocess
import sys
import threading
import time
from pathlib import Path
from workdir import WORK_DIR  # the heavy work folder (REKHCOM_WORK)

QEMU = (WORK_DIR / 'toolchain/qemu/runtime')
IMAGES = {
    'x86_64': (WORK_DIR / 'linux/vm/debian-12-nocloud-amd64.qcow2'),
    # newer C library and Mesa than the Linux bundle (Debian 12), like SteamOS: the system-library launch (#140)
    'x86_64-trixie': (WORK_DIR / 'linux/vm/debian-13-nocloud-amd64.qcow2'),
    'aarch64': (WORK_DIR / 'linux/vm/debian-12-nocloud-arm64.qcow2'),
}


class Console:
    def __init__(self, proc):
        self.proc = proc
        self.q = queue.Queue()
        self.buf = ''
        threading.Thread(target=self._pump, daemon=True).start()

    def _pump(self):
        while True:
            b = self.proc.stdout.read(1)
            if not b:
                self.q.put(None)
                return
            self.q.put(b.decode('latin-1'))

    def expect(self, pattern, timeout):
        end = time.time() + timeout
        while time.time() < end:
            # Only the recent output can hold the prompt or marker: searching the
            # whole buffer on every byte made long outputs quadratic.
            base = max(0, len(self.buf) - 4096)
            m = re.search(pattern, self.buf[base:])
            if m:
                out, self.buf = self.buf[:base + m.end()], self.buf[base + m.end():]
                return out
            try:
                c = self.q.get(timeout=0.5)
            except queue.Empty:
                continue
            if c is None:
                raise SystemExit('QEMU exited:\n' + self.buf[-2000:])
            self.buf += c
        raise SystemExit(f'timeout waiting for {pattern!r}:\n' + self.buf[-3000:])

    def send(self, text):
        # The emulated UART drops input that arrives faster than it drains.
        data = text.encode()
        for i in range(0, len(data), 8):
            self.proc.stdin.write(data[i:i + 8])
            self.proc.stdin.flush()
            time.sleep(0.03)

    def run(self, cmd, timeout=600):
        marker = f'__DONE_{int(time.time() * 1000)}__'
        self.send(f'{cmd}; echo {marker}$?\n')
        out = self.expect(marker + r'(\d+)', timeout)
        code = int(re.search(marker + r'(\d+)', out).group(1))
        return out, code


def main():
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--machine', choices=IMAGES, default='x86_64')
    ap.add_argument('--share', type=Path, required=True)
    ap.add_argument('--run', action='append', default=[])
    ap.add_argument('--bg-run', action='append', default=[],
                    help='run in the background inside the VM, printing its log every --poll seconds; '
                         'abort when the log stops changing for --stall seconds')
    ap.add_argument('--poll', type=int, default=30)
    ap.add_argument('--stall', type=int, default=900)
    ap.add_argument('--fetch', action='append', default=[])
    ap.add_argument('--out', type=Path, default=Path('.'))
    ap.add_argument('--accel', default='whpx,kernel-irqchip=off')
    ap.add_argument('--disk', type=Path, help='persistent qcow2 overlay (created from the image, kept between runs)')
    ap.add_argument('--disk-size', default='40G')
    ap.add_argument('--net', action='store_true', help='user-mode network (apt, downloads)')
    ap.add_argument('--run-timeout', type=int, default=1800, help='seconds allowed for each --run command')
    ap.add_argument('--fresh-share', action='store_true', help='empty /root/share first (a persistent --disk keeps it)')
    ap.add_argument('--mem', default='2048')
    ap.add_argument('--cpus', default='2')
    args = ap.parse_args()
    # The shared folder travels as a tar archive used as a raw disk (tar
    # blocks are 512 bytes): fast, and keeps the executable bits.
    import tarfile
    image = args.share.with_suffix('.tar')
    with tarfile.open(image, 'w', format=tarfile.GNU_FORMAT) as t:
        for f in sorted(args.share.rglob('*')):
            info = t.gettarinfo(f, f.relative_to(args.share).as_posix())
            info.mode = 0o755 if f.is_dir() or f.suffix in ('', '.sh') or '.so' in f.name else 0o644
            if f.is_file():
                with open(f, 'rb') as fh:
                    t.addfile(info, fh)
            else:
                t.addfile(info)
    with open(image, 'ab') as fh:
        fh.write(b'\0' * (-fh.tell() % 1048576))
    share = str(image).replace('\\', '/')
    system = f'file={IMAGES[args.machine]},if=virtio,snapshot=on'
    if args.disk:
        if not args.disk.exists():
            subprocess.check_call([str(QEMU / 'qemu-img.exe'), 'create', '-f', 'qcow2', '-F', 'qcow2',
                                   '-b', str(IMAGES[args.machine]), str(args.disk), args.disk_size])
        # discard: what the guest frees (fstrim below) leaves the overlay instead of staying allocated
        system = f'file={args.disk},if=virtio,discard=unmap,detect-zeroes=unmap'
    nic = ['-nic', 'user,model=virtio-net-pci'] if args.net else ['-nic', 'none']
    # Outbox for --fetch: a raw disk (/dev/vdc) the guest writes a tar archive to.
    outbox = image.with_name(image.stem + '-outbox.img')
    with open(outbox, 'wb') as fh:
        fh.truncate(1 << 30)
    outdrive = ['-drive', 'file=' + str(outbox).replace(chr(92), '/') + ',format=raw,if=virtio']
    if args.machine.startswith('x86_64'):
        cmd = [str(QEMU / 'qemu-system-x86_64.exe'), '-m', args.mem, '-smp', args.cpus, '-accel', args.accel,
               '-accel', 'tcg', '-drive', system,
               '-drive', f'file={share},format=raw,if=virtio,readonly=on', *outdrive, '-nographic', *nic]
    else:
        # cortex-a53 like the H700; virtio-gpu gives the kernel a /dev/fb0.
        cmd = [str(QEMU / 'qemu-system-aarch64.exe'), '-M', 'virt', '-cpu', 'cortex-a57', '-m', args.mem, '-smp', args.cpus,
               '-device', 'virtio-gpu-pci',
               '-bios', str(QEMU / 'share/edk2-aarch64-code.fd'),
               '-drive', system,
               '-drive', f'file={share},format=raw,if=virtio,readonly=on', *outdrive, '-nographic', *nic]
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    con = Console(proc)
    try:
        con.expect(r'login: ', 900)
        con.send('root\n')
        con.expect(r'# ', 120)
        con.send('stty -echo cols 250; export PS1="# "\n')
        con.expect(r'# ', 30)
        fresh = 'rm -rf /root/share && ' if args.fresh_share else ''
        for step in (fresh + 'mkdir -p /root/share && tar -xf /dev/vdb -C /root/share', 'ls /root/share', 'uname -m'):
            out, code = con.run(step, 300)
            print(f'# {step} [exit {code}]\n{out.strip()[-600:]}', flush=True)
        for c in args.run:
            out, code = con.run(c, args.run_timeout)
            print(f'$ {c}\n{out.strip()}\n[exit {code}]', flush=True)
        for c in args.bg_run:
            # In the background, so its progress can be watched and a hang seen.
            con.run(f'rm -f /root/.bgexit; ( ( {c} ) > /root/.bglog 2>&1; echo $? > /root/.bgexit ) & :', 30)
            last, since, started = None, time.time(), time.time()
            while True:
                time.sleep(args.poll)
                out, _ = con.run('echo EXIT=$(cat /root/.bgexit 2>/dev/null); tail -c 400 /root/.bglog | tail -2', 60)
                code = re.search(r'EXIT=(\d+)', out)
                tail = re.sub(r'\x1b\[[0-9;?]*[A-Za-z]|EXIT=\d*', '', out.rsplit('__DONE_', 1)[0])
                tail = '\n'.join(l for l in tail.splitlines() if l.strip() and not l.startswith('#')).strip()
                print(f'[{int(time.time() - started)} s] {tail[-200:]}', flush=True)
                if code:
                    out, _ = con.run('tail -40 /root/.bglog', 60)
                    print(f'$ {c}\n{out.strip()}\n[exit {code.group(1)}]', flush=True)
                    break
                if tail != last:
                    last, since = tail, time.time()
                elif time.time() - since > args.stall:
                    out, _ = con.run('tail -60 /root/.bglog; ps aux --sort=-%cpu | head -15', 60)
                    print(f'STALLED (no new output for {args.stall} s):\n{out}', flush=True)
                    break
        args.out.mkdir(parents=True, exist_ok=True)
        if args.fetch:
            names = ' '.join(f.lstrip('/') for f in args.fetch)
            out, code = con.run(f'tar -cf /dev/vdc -C / {names}; sync', 600)
            print(f'# outbox tar [exit {code}] {out.strip()[-300:]}', flush=True)
        if args.disk:
            con.run('rm -rf /root/share; apt-get clean; fstrim -v /', 600)  # keep the persistent disk small
        con.send('poweroff\n')
        proc.wait(120)
        if args.fetch:
            import tarfile
            with tarfile.open(outbox) as t:
                for f in args.fetch:
                    try:
                        data = t.extractfile(f.lstrip('/')).read()
                    except (KeyError, AttributeError):
                        print('could not fetch', f)
                        continue
                    (args.out / Path(f).name).write_bytes(data)
                    print('fetched', f, len(data), 'bytes')
        outbox.unlink(missing_ok=True)
    finally:
        if proc.poll() is None:
            proc.kill()


if __name__ == '__main__':
    main()
