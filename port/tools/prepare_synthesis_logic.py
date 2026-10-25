"""Extract production function bodies verbatim for the small synthesis host test.
No rewrite of InitEventKeyCard, total update or payment; allocation/DMA are
test doubles, game structures and the adapter are included directly.
"""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build/pc-atlas'


def extract(source, name):
    text = source.read_text(encoding='utf-8')
    import re
    match = re.search(r'^\w+[ *]+\b' + name + r'\([^\n]*\) \{', text, re.M)
    assert match, name
    start = match.start()
    pos = text.index('{', start)
    depth = 1
    end = pos + 1
    while depth:
        if text[end] == '{': depth += 1
        if text[end] == '}': depth -= 1
        end += 1
    return text[start:end]


if __name__ == '__main__':
    OUT.mkdir(parents=True, exist_ok=True)
    chunks, provenance = [], []
    for file, names in [
        ('src/card/card_prize_card_init.c', ['InitEventKeyCard', 'UpdateEventKeyTotal']),
        ('src/map/map.c', ['GetEventKey', 'PayEventKey']),
    ]:
        for name in names:
            body = extract(ROOT / file, name)
            chunks.append(body)
            provenance.append(dict(file=file, symbol=name, sha256=hashlib.sha256(body.encode()).hexdigest()))
    (OUT / 'synthesis_real_functions.inc').write_text('\n\n'.join(chunks) + '\n', encoding='utf-8')
    (OUT / 'synthesis_logic_provenance.json').write_text(json.dumps(provenance, indent=2), encoding='utf-8')
    (OUT / 'synthesis-logic.ninja').write_text(
        'include build/pc-atlas/build.ninja\n'
        'rule synthesis_logic\n'
        '  command = $cc $game_cflags -Ibuild/pc-atlas $in -o $out\n'
        'build build/pc-atlas/test_synthesis_logic.exe: synthesis_logic '
        'port/tools/test_synthesis_requirement_hd_logic.c | '
        'build/pc-atlas/synthesis_real_functions.inc src/card/synthesis_requirement_hd.inc\n',
        encoding='utf-8')
    print('Extracted 4 production functions verbatim; no game launched')
