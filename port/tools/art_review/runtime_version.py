"""Fingerprint the local app; no package installation or network required."""
import hashlib
from pathlib import Path


def build_id():
    root=Path(__file__).resolve().parent
    digest=hashlib.sha256()
    for path in sorted([*root.glob('*.py'),*root.glob('*.ps1'),*root.glob('requirements.txt'),*root.glob('web/*')]):
        if path.is_file():
            digest.update(path.relative_to(root).as_posix().encode())
            digest.update(path.read_bytes().replace(b'\r\n',b'\n'))
    return digest.hexdigest()[:20]


if __name__=='__main__':print(build_id())
