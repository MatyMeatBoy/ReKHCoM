"""Native launcher packaging preserves relative importer resources and Unix modes."""
import importlib.util
from pathlib import Path
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('linux_package', ROOT/'port/tools/build_linux.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as tmp:
    dest = Path(tmp)/'package with spaces'
    m.install_native_launcher(dest)
    header=(ROOT/'port/include/port/port_version.h').read_text(encoding='utf-8')
    version=next(line.split('"')[1] for line in header.splitlines() if line.startswith('#define REKHCOM_VERSION '))
    assert (dest/'ReKHCoM.version').read_text().strip() == version
    assert (dest/'launcher/port/data/hd_sprites.tsv').read_bytes() == (ROOT/'port/data/hd_sprites.tsv').read_bytes()
    for name in ('khhd_import.py','khhd_pkg.py','khhd_scd.py','khhd_subs.py','khhd_sprites.py','khhd_posters.py'):
        assert (dest/'launcher/port/tools'/name).is_file(), name
    assert b'\r\n' not in (dest/'ReKHCoM Launcher.sh').read_bytes()
    assert not list(dest.rglob('*.sav')) and not list(dest.rglob('*.gba'))
    archive=Path(tmp)/'linux.tar.gz'
    m.make_tar(dest,archive)
    with tarfile.open(archive) as t:
        assert t.getmember('ReKHCoM Launcher.sh').mode == 0o755
        assert t.getmember('launcher/rekhcom_launcher.py').mode == 0o644
print('Native Linux launcher/import package resources and permissions PASS')
