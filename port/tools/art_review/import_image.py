"""Add a delivered image without changing its older review versions."""
import argparse
import json
from pathlib import Path
from server import ROOT, Store

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('image', type=Path)
    p.add_argument('--title', required=True)
    p.add_argument('--version', required=True)
    p.add_argument('--provenance', default='')
    p.add_argument('--parent')
    p.add_argument('--revision-info',type=Path,help='JSON: feedback IDs, changes, technique, checks, limitations')
    p.add_argument('--data', type=Path, default=ROOT / 'build/art-review')
    a = p.parse_args()
    result = Store(a.data).add(a.image.read_bytes(), a.title, a.version, a.provenance, parent_id=a.parent,
        revision_info=json.loads(a.revision_info.read_text(encoding='utf-8')) if a.revision_info else None)
    print(result['id'])
