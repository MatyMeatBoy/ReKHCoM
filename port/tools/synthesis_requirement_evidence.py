"""Assemble labelled, unretouched crops of real runtime captures for Atlas.
Also verify HD-off equality and write the review metadata. Does not launch
the game, upload anything, or change an older review.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--normal', type=Path, default=ROOT / 'build/synthesis-fix-final')
    parser.add_argument('--variants', type=Path, default=ROOT / 'build/synthesis-fix-variants')
    parser.add_argument('--out', type=Path, default=ROOT / 'build/synthesis-fix-delivery')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    checks = []
    for frame in (300, 380, 470, 510):
        with Image.open(args.normal / 'hd-off' / f'out_{frame:05d}.bmp') as actual, \
             Image.open(args.normal / 'baseline-off' / f'out_{frame:05d}.bmp') as before:
            assert actual.size == before.size and actual.tobytes() == before.tobytes(), frame
        checks.append(f'HD off frame {frame}: pixel-identical to baseline')
    with Image.open(args.normal / 'hd-on/out_00470.bmp') as im:
        im.save(args.out / 'synthesis-runtime-470.png')
    sheet = Image.new('RGB', (660, 746), '#24243a')
    draw = ImageDraw.Draw(sheet)
    sources = []
    for index, (name, frame, label) in enumerate([
        ('red', 470, 'Fixture real: rojo, <=7, frame 470'),
        ('green', 470, 'Fixture real: verde, =12, frame 470'),
        ('blue', 470, 'Fixture real: azul, TOTAL 10, frame 470'),
        ('blue', 510, 'Misma ejecucion: TOTAL 9, frame 510'),
    ]):
        source = args.variants / name / 'hd-on' / f'out_{frame:05d}.bmp'
        x, y = (index % 2) * 330, (index // 2) * 360
        draw.text((x + 8, y + 8), label, fill='white')
        with Image.open(source) as im:
            sheet.paste(im.crop((805, 210, 1125, 530)), (x + 5, y + 30))
        sources.append(dict(path=str(source), frame=frame,
                            sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
    draw.text((8, 724), 'Recortes sin retoque; fixtures controlados. Pago real probado en arnes.', fill='white')
    sheet.save(args.out / 'synthesis-runtime-variants.png')
    info = dict(
        response_to=[dict(submission='20261025-125559-473c1c',
                          image_id='c08283ad450f4455937a3f1e1ead3302', cloud_id=cloud)
                     for cloud in ('a849c7d8-4d91-484e-ba31-1c186915558c',
                                   '23d13c7a-d2c6-4911-914a-246099bbafe7')],
        changes=[
            'Captura real de puerta normal: la carta HD sustituye al dorso y requisito GBA en su misma posicion; no se coloca a un lado.',
            'Seleccion dinamica por regla, valor y color; TOTAL usa drawnTotal. Fixtures con InitEventKeyCard real: rojo <=7, verde =12 y azul TOTAL 10->9.',
            'Numeros y operadores ajustados como grupo dentro de la silueta, con margen lateral; el 12= ya no usa todo el ancho de su caja OAM.',
            'Staging prefiere los diez gCardValueDigitTiles del mod local autorizado; no se afirma una reproduccion exacta de 1.5HD.',
            'Banda inferior neutra segun la comparacion GBA y propuesta anterior; color de condicion solo en el contorno, centro transparente para conservar la luz.',
        ],
        technique='Identidades HD sobre copias de entradas ObjTiles; OAM, VRAM, paletas, geometria, afines y prioridades originales. Sin cambios PPU/texto.',
        checks=['Build EU Ninja -j1; dummy y PPU 1 thread; mando filtrado 0xffff/0xffff',
                'Arnes: 2000 selecciones; 20 inicializaciones reales; cuerpos reales GetEventKey/PayEventKey/UpdateEventKeyTotal: pago 1 deja 9 y pagar 9 completa.',
                'Log runtime blue: TOTAL 10 frame470, UpdateEventKeyTotal frame490, TOTAL 9 frame510',
                *checks],
        limitations=[
            'Las variantes son fixtures de un build con PORT_SYNTHESIS_TEST_FIXTURE; no una partida natural. La captura principal usa build normal sin ese define.',
            'El fixture de TOTAL fuerza el nuevo saldo y ejecuta UpdateEventKeyTotal real; PayEventKey real se verifica aparte en el arnes con asignadores/DMA simulados.',
            'Rango 0..99; fuera de rango o assets ausentes conserva GBA. Kind especifico conserva su renderer existente.',
            'No se declara cobertura completa de todas las puertas ni de cancelacion/reapertura. Assets pendientes de revision y staging explicito. No se ha visto una captura del menu de 1.5HD para afirmar distancias exactas.',
        ], sources=sources)
    (args.out / 'revision.json').write_text(json.dumps(info, indent=2, ensure_ascii=False), encoding='utf-8')
    (args.out / 'report.json').write_text(json.dumps(dict(checks=checks, variants=sources), indent=2), encoding='utf-8')
    print(args.out)
