"""Exercise the actual build generator against changed art and approval records."""
import json
from pathlib import Path
import shutil
import tempfile
from gen_scene_framing import ROOT, MANIFEST, inputs, generate
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp)
    for name in inputs(ROOT)+[MANIFEST]:
        target=root/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target)
    out=root/'grants.h';expected=generate(root,out);assert len(expected)==10
    stamp=out.stat().st_mtime_ns;assert generate(root,out)==expected;assert out.stat().st_mtime_ns==stamp
    art=root/'port/src/port_ppu.c';original=art.read_bytes();art.write_bytes(original+b'\n/* changed art recipe */\n')
    assert generate(root,out)==[], 'Changed renderer must not inherit any shared approval'
    art.write_bytes(original);assert generate(root,out)==expected
    manifest=root/MANIFEST;m=json.loads(manifest.read_text());m['entries'][0]['revision']+=1
    manifest.write_text(json.dumps(m));assert generate(root,out)==expected[1:]
    m['entries'][0]['revision']-=1;m['entries'][0]['status']='approved';manifest.write_text(json.dumps(m))
    assert generate(root,out)==expected[1:]
print('PASS build generator: exact reviewed versions, changed art/revision/status fail closed, stable output')
