"""Fail closed when reviewed scene art/renderer inputs or approval versions change."""
import argparse
import hashlib
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
MANIFEST = "port/progress/SCENE_FRAMING_HONORS.json"

def inputs(root):
    # Conservative shared renderer gate: any changed art recipe requires review.
    return sorted({"port/src/port_ppu.c", "port/src/port_guard_armor.c"} |
                  {p.relative_to(root).as_posix() for p in (root/"port/include/port").rglob("*")
                   if p.is_file() and (p.suffix == ".inc" or "tiles" in p.name)})

def signature(root, entry, files):
    h = hashlib.sha256(json.dumps({k: entry[k] for k in
        ("event", "asset_id", "status", "revision", "art_sha256")}, sort_keys=True).encode())
    for name in files:
        h.update(name.encode()); h.update(b"\0")
        # Checkout newline conventions must not invalidate the same source.
        h.update((root/name).read_bytes().replace(b"\r\n", b"\n")); h.update(b"\0")
    return h.hexdigest()

def generate(root, output):
    manifest = json.loads((root/MANIFEST).read_text())
    files = inputs(root)
    allowed = [e["event"] for e in manifest["entries"] if e["status"] == "honors" and
               e.get("reviewed_signature") == signature(root, e, files)]
    cases = " ".join("case %d:" % e for e in allowed)
    body = "/* Generated; changed art defaults to Ajuste until reviewed again. */\n"
    body += "#define PORT_SCENE_HONORS_CASES " + cases + "\n"
    output.parent.mkdir(parents=True, exist_ok=True)
    if not output.exists() or output.read_text() != body: output.write_text(body)
    return allowed

if __name__ == "__main__":
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument("--out",type=Path,required=True)
    a=ap.parse_args(); print("Scene honors admitted:", generate(ROOT,a.out))
