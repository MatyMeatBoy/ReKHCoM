"""Native GBA backgrounds and isolated, versioned Tiled editor projects.

Decoder layout follows port/tools/scene_tile_data.py. No original ROM is written
or exposed over HTTP. All generated files belong to the local editor workspace.
"""
import argparse
import copy
from datetime import datetime, timezone
import hashlib
import io
import json
import math
from pathlib import Path
import re
import struct
import threading
import uuid
import zipfile

from PIL import Image

FLIP_H, FLIP_V = 0x80000000, 0x40000000
GID_MASK = 0x0fffffff
MAX_CELLS = 262144


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def stamp():
    return datetime.now(timezone.utc).isoformat()


def integer(value, low, high):
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f'Expected integer in {low}..{high}')
    return value


def write_json(path, value):
    temporary = path.with_name(path.name + '.' + uuid.uuid4().hex + '.tmp')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')
    temporary.replace(path)


class NativeROM:
    def __init__(self, raw):
        self.raw = raw

    def at(self, pointer, size):
        offset = pointer - 0x08000000
        if size < 0 or offset < 0 or offset + size > len(self.raw):
            raise ValueError('ROM pointer outside supplied cartridge')
        return self.raw[offset:offset + size]

    def words(self, pointer, count):
        return struct.unpack('<' + 'I' * count, self.at(pointer, count * 4))

    def lz77(self, pointer):
        head = self.at(pointer, 4)
        if head[0] != 0x10:
            raise ValueError('Unsupported GBA compression header')
        size = int.from_bytes(head[1:], 'little')
        if not 0 < size <= 4 * 1024 * 1024:
            raise ValueError('Decompressed asset too large')
        out, pos = bytearray(), pointer + 4
        while len(out) < size:
            flags = self.at(pos, 1)[0]
            pos += 1
            for bit in range(8):
                if len(out) >= size:
                    break
                if flags & (0x80 >> bit):
                    a, b = self.at(pos, 2)
                    pos += 2
                    count, distance = (a >> 4) + 3, ((a & 15) << 8 | b) + 1
                    if distance > len(out):
                        raise ValueError('Invalid LZ77 backreference')
                    for _ in range(min(count, size - len(out))):
                        out.append(out[-distance])
                else:
                    out.extend(self.at(pos, 1))
                    pos += 1
        return bytes(out)

    def background(self, address):
        header = self.at(address, 48)
        if header[36]:
            raise ValueError('Affine background: native affine layout is not supported yet')
        tiles, tiles2, pal, *maps = struct.unpack_from('<6I', header)
        size, size2, psize, width, height = struct.unpack_from('<HHHBB', header, 24)
        compression = header[46]
        if compression not in (0, 1, 2, 3) or not 0 < width <= 8 or not 0 < height <= 8:
            raise ValueError('Unsupported background dimensions/compression')
        if not 0 < psize <= 512 or psize % 2:
            raise ValueError('Invalid BG palette')
        tile_data = self.lz77(tiles) if compression in (1, 3) else self.at(tiles, size)
        second = (self.lz77(tiles2) if compression in (1, 3) else self.at(tiles2, size2)) if tiles2 else tile_data
        palette_words = list(struct.unpack('<' + 'H' * (psize // 2), self.at(pal, psize)))
        palette = [[(v & 31) * 255 // 31, (v >> 5 & 31) * 255 // 31, (v >> 10 & 31) * 255 // 31] for v in palette_words]
        layers = []
        for name, table in zip(('bg3', 'bg2', 'bg1'), maps):
            if not table:
                continue
            entries = [0] * (width * height * 1024)
            for by in range(height):
                for bx in range(width):
                    block_pointer = self.words(table + (by * width + bx) * 4, 1)[0]
                    block = self.lz77(block_pointer) if compression in (2, 3) else self.at(block_pointer, 2048)
                    if len(block) < 2048:
                        raise ValueError('Incomplete native tilemap')
                    words = struct.unpack_from('<1024H', block)
                    for y in range(32):
                        start = (by * 32 + y) * width * 32 + bx * 32
                        entries[start:start + 32] = words[y * 32:y * 32 + 32]
            layers.append(dict(name=name, entries=entries, tiles=second if name == 'bg1' else tile_data))
        if not layers:
            raise ValueError('Background has no text tilemap layers')
        return dict(width=width * 32, height=height * 32, palette=palette,
                    palette_words=palette_words, layers=layers)


class TileEditor:
    def __init__(self, root, repo, rom=None, symbols=None, region='eu', pieces=None, sprites=None):
        self.root, self.repo = Path(root).resolve(), Path(repo).resolve()
        self.region, self.lock = region, threading.RLock()
        for folder in ('assets', 'projects'):
            (self.root / folder).mkdir(parents=True, exist_ok=True)
        self.rom_path = Path(rom) if rom else self.repo / 'roms' / {'eu':'B8CP.gba', 'us':'B8CE.gba', 'jp':'B8CJ.gba'}[region]
        self.symbol_path = Path(symbols) if symbols else self.repo / 'build' / region / 'pc/rom_symbols.s'
        self.pieces_path, self.sprites_path = Path(pieces) if pieces else None, Path(sprites) if sprites else None
        self.native, self.source, self.scenes, self.cache, self.known_tilesets = None, None, [], {}, {}
        self.reason = None
        self._load_sources()

    def _load_sources(self):
        if not self.rom_path.is_file() or not self.symbol_path.is_file():
            self.reason = 'Configure a local cartridge and its matching regional rom_symbols.s; no sample art is substituted.'
            return
        raw, syms = self.rom_path.read_bytes(), self.symbol_path.read_bytes()
        symbols = {name:int(address, 16) for name, address in re.findall(r'\.set _(\w+), (0x[0-9a-fA-F]+)', syms.decode('utf-8'))}
        source = (self.repo / 'src/evt/event_index_data.c').read_text(encoding='utf-8')
        names = re.findall(r'(NULL|&gEventBg\w+)', source.split('gEventBackgroundDefs[] = {', 1)[1].split('};', 1)[0])
        event_source = (self.repo / 'src/evt/events.c').read_text(encoding='utf-8')
        event_names = {int(i):name for i, name in re.findall(r'events/(\d+)_([^"/]+)\.inc', event_source)}
        worlds = ['traverse_town', 'monstoro', 'halloween_town', 'wonderland', 'atlantica', 'agrabah', 'neverland', 'coliseum', 'hollowbastion', 'twilight_town', 'destiny_islands', 'castle_oblivion']
        self.native = NativeROM(raw)
        self.source = dict(rom_sha256=digest(raw), symbols_sha256=digest(syms), region=self.region,
                           event_index_sha256=digest(source.encode()))
        for event, symbol in enumerate(names):
            symbol = symbol.lstrip('&')
            if symbol == 'NULL' or symbol not in symbols:
                continue
            event_name = event_names.get(event, '')
            world = next((w for w in worlds if w in event_name), 'castle_oblivion' if re.match(r'\d+f_', event_name) else 'unknown')
            address = symbols[symbol]
            header = self.native.at(address, 48)
            self.scenes.append(dict(id=f'{event:03}', symbol=symbol, world=world,
                name=f'{event:03} · {world.replace("_", " ").title()} · {symbol}',
                supported=not bool(header[36]), width=header[30] * 32, height=header[31] * 32,
                address=address, source=self.source))

    def scene_list(self):
        return dict(available=bool(self.native), reason=self.reason, scenes=self.scenes,
                    categories=['background', 'piece', 'sprite', 'hud'])

    def _scene(self, scene):
        value = str(scene)
        if value.isdigit():
            value = f'{int(value):03}'
        found = next((s for s in self.scenes if s['id'] == value), None)
        if not found:
            raise ValueError('Unknown or unavailable native scene')
        return found

    def _assets(self, key):
        folder = self.root / 'assets' / key
        folder.mkdir(exist_ok=True)
        return folder

    def _atlas(self, layer, background):
        # Only native tile/palette pairs actually referenced by this layer. No
        # fabricated colours or blank placeholder entries are appended.
        variants = sorted({(entry & 1023, entry >> 12) for entry in layer['entries']})
        identity = layer['tiles'] + json.dumps([variants, background['palette_words']]).encode()
        key = digest(identity)
        columns, tilecount = 32, len(variants)
        atlas = Image.new('RGBA', (columns * 8, math.ceil(tilecount / columns) * 8))
        indices = []
        for local_id, (tile, bank) in enumerate(variants):
            tile_pixels = []
            raw = layer['tiles'][tile * 32:(tile + 1) * 32]
            if len(raw) != 32:
                raise ValueError('Tile index exceeds its native tile bank')
            for y in range(8):
                for x in range(8):
                    index = raw[y * 4 + x // 2] >> ((x % 2) * 4) & 15
                    color = bank * 16 + index
                    if color >= len(background['palette']):
                        raise ValueError('Tile references an unloaded palette bank')
                    rgba = (*background['palette'][color], 255 if index else 0)
                    atlas.putpixel((local_id % columns * 8 + x, local_id // columns * 8 + y), rgba)
                    tile_pixels.append(index)
            indices.append(dict(tileid=local_id, native_tile=tile, palette_bank=bank, pixels=tile_pixels))
        folder = self._assets(key)
        atlas.save(folder / 'atlas.png')
        write_json(folder / 'indices.json', dict(format='GBA-4bpp', tilewidth=8, tileheight=8,
            palette=background['palette'], palette_words=background['palette_words'], tiles=indices,
            source=self.source, tiles_sha256=digest(layer['tiles'])))
        tileset = dict(name=layer['name'], tilewidth=8, tileheight=8, columns=columns, tilecount=tilecount,
            image=f'/editor-assets/{key}/atlas.png', imagewidth=atlas.width, imageheight=atlas.height,
            source_sha256=key, palette=background['palette'], indices_url=f'/editor-assets/{key}/indices.json',
            properties=[dict(name='native_source_sha256', type='string', value=key)])
        self.known_tilesets[key] = copy.deepcopy(tileset)
        write_json(folder / 'tileset.json', tileset)
        return tileset, {(tile, bank):i for i, (tile, bank) in enumerate(variants)}, atlas

    def scene(self, scene):
        with self.lock:
            info = self._scene(scene)
            if info['id'] in self.cache:
                return copy.deepcopy(self.cache[info['id']])
            bg = self.native.background(info['address'])
            tilemap = dict(type='map', version='1.10', tiledversion='1.11.0', orientation='orthogonal',
                renderorder='right-down', infinite=False, width=bg['width'], height=bg['height'],
                tilewidth=8, tileheight=8, layers=[], tilesets=[])
            items, firstgid = [], 1
            combined = Image.new('RGBA', (bg['width'] * 8, bg['height'] * 8))
            layer_images = {}
            for index, layer in enumerate(bg['layers']):
                tileset, variants, atlas = self._atlas(layer, bg)
                tileset['firstgid'] = firstgid
                tilemap['tilesets'].append(tileset)
                gids = []
                image = Image.new('RGBA', combined.size)
                for offset, entry in enumerate(layer['entries']):
                    local = variants[(entry & 1023, entry >> 12)]
                    gid = firstgid + local
                    if entry & 1024:
                        gid |= FLIP_H
                    if entry & 2048:
                        gid |= FLIP_V
                    gids.append(gid)
                    x, y = local % tileset['columns'] * 8, local // tileset['columns'] * 8
                    tile = atlas.crop((x, y, x + 8, y + 8))
                    if entry & 1024:
                        tile = tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
                    if entry & 2048:
                        tile = tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
                    image.paste(tile, (offset % bg['width'] * 8, offset // bg['width'] * 8))
                tilemap['layers'].append(dict(id=index + 1, name=layer['name'], type='tilelayer', visible=True,
                    opacity=1, x=0, y=0, width=bg['width'], height=bg['height'], data=gids))
                combined.alpha_composite(image)
                layer_images[layer['name']] = image
                layer_key = digest(json.dumps([self.source, info['symbol'], layer['name']]).encode())
                folder = self._assets(layer_key)
                image.save(folder / 'preview.png')
                items.append(dict(id=f'{info["id"]}-{layer["name"]}', name=f'{info["symbol"]} · {layer["name"]}',
                    scene=info['id'], world=info['world'], category='background', image_url=f'/editor-assets/{layer_key}/preview.png',
                    tileset=tileset, stamp=dict(width=bg['width'], height=bg['height'], data=gids, layer=layer['name'])))
                firstgid += tileset['tilecount']
            preview_key = digest(json.dumps([self.source, info['symbol'], 'combined']).encode())
            combined.save(self._assets(preview_key) / 'preview.png')
            tilemap['nextlayerid'], tilemap['nextobjectid'] = len(tilemap['layers']) + 1, 1
            project = dict(id=None, revision=0, scene=info['id'], world=info['world'],
                source={**self.source, 'bg_symbol':info['symbol'], 'address':info['address']}, map=tilemap, commands=[],
                preview_url=f'/editor-assets/{preview_key}/preview.png', updated_at=stamp(),
                transform=dict(space='native-bg-tilemap', tile_size=8, origin=[0, 0],
                    runtime_camera_applied=False, note='ROM coordinates; runtime scrolling, affine, fades and HD extensions are separate.'))
            if self.pieces_path and self.pieces_path.is_file():
                definitions = json.loads(self.pieces_path.read_text(encoding='utf-8'))
                for piece in definitions:
                    if str(piece['scene']).zfill(3) != info['id']:
                        continue
                    items.append(self._piece(piece, info, tilemap, layer_images))
            result = dict(project=project, items=items)
            self.cache[info['id']] = result
            return copy.deepcopy(result)

    def _piece(self, piece, info, tilemap, images):
        layer = next(l for l in tilemap['layers'] if l['name'] == piece['layer'])
        x, y = integer(piece['x'], 0, layer['width'] - 1), integer(piece['y'], 0, layer['height'] - 1)
        w, h = integer(piece['width'], 1, layer['width'] - x), integer(piece['height'], 1, layer['height'] - y)
        data = [layer['data'][(y + row) * layer['width'] + x + col] for row in range(h) for col in range(w)]
        image = images[layer['name']].crop((x * 8, y * 8, (x + w) * 8, (y + h) * 8))
        if not image.getbbox():
            raise ValueError('Named piece ROI contains no visible native pixels')
        key = digest(json.dumps([info['symbol'], piece, self.source], sort_keys=True).encode())
        image.save(self._assets(key) / 'preview.png')
        tileset = next(t for t in tilemap['tilesets'] if t['firstgid'] <= (data[0] & GID_MASK) < t['firstgid'] + t['tilecount'])
        return dict(id=key, name=piece['name'], scene=info['id'], world=info['world'], category='piece',
            image_url=f'/editor-assets/{key}/preview.png', tileset=tileset,
            stamp=dict(width=w, height=h, data=data, layer=layer['name']),
            source_roi=dict(x=x, y=y, width=w, height=h, units='tiles', evidence=piece.get('evidence', 'explicit ROI')))

    def catalog(self, scene=None, world=None, scope='scene', category=None):
        selected = self._scene(scene) if scene is not None else None
        target_world = (world or (selected['world'] if selected else '')).lower().replace(' ', '_')
        candidates = [s for s in self.scenes if (scope == 'global' or
            scope == 'world' and s['world'].lower() == target_world or scope == 'scene' and selected and s['id'] == selected['id'])]
        items, errors, seen = [], [], set()
        for info in candidates:
            if info['symbol'] in seen:
                continue
            seen.add(info['symbol'])
            try:
                items += self.scene(info['id'])['items']
            except (ValueError, struct.error) as error:
                errors.append(dict(scene=info['id'], reason=str(error)))
        if category:
            items = [item for item in items if item['category'] == category]
        return dict(items=items, errors=errors, available=bool(self.native), reason=self.reason)

    def _project_path(self, project_id):
        if not isinstance(project_id, str) or not re.fullmatch(r'[a-f0-9]{32}', project_id):
            raise ValueError('Invalid project ID')
        return self.root / 'projects' / (project_id + '.json')

    def new_project(self, scene):
        project = self.scene(scene)['project']
        project['id'] = uuid.uuid4().hex
        return self.save_project(project, 0)

    def load_project(self, project_id):
        with self.lock:
            path = self._project_path(project_id)
            if not path.is_file():
                raise ValueError('Project not found')
            return json.loads(path.read_text(encoding='utf-8'))

    def projects(self):
        result = []
        with self.lock:
            for path in (self.root / 'projects').glob('*.json'):
                if not re.fullmatch(r'[a-f0-9]{32}', path.stem):
                    continue
                project = json.loads(path.read_text(encoding='utf-8'))
                result.append({key:project.get(key) for key in ('id','name','scene','world','revision','updated_at')})
        return sorted(result, key=lambda p:p['updated_at'], reverse=True)

    def import_map(self, scene, tilemap):
        project = self.scene(scene)['project']
        project.update(id=uuid.uuid4().hex, map=copy.deepcopy(tilemap))
        return self.save_project(project, 0)

    def validate_map(self, tilemap):
        if not isinstance(tilemap, dict) or tilemap.get('orientation') != 'orthogonal' or tilemap.get('infinite'):
            raise ValueError('Only finite orthogonal Tiled maps are supported')
        width, height = integer(tilemap.get('width'), 1, 1024), integer(tilemap.get('height'), 1, 1024)
        if width * height > MAX_CELLS or tilemap.get('tilewidth') != 8 or tilemap.get('tileheight') != 8:
            raise ValueError('Expected bounded native 8x8 map')
        if not isinstance(tilemap.get('tilesets'), list) or len(tilemap['tilesets']) > 64:
            raise ValueError('Invalid tilesets')
        ranges, canonical = [], []
        for ts in tilemap['tilesets']:
            key = ts.get('source_sha256')
            if key not in self.known_tilesets:
                # Rehydrate a persisted native atlas without trusting arbitrary paths.
                if not isinstance(key, str) or not re.fullmatch('[a-f0-9]{64}', key):
                    raise ValueError('Unknown native tileset')
                metadata = self.root / 'assets' / key / 'tileset.json'
                if not metadata.is_file():
                    raise ValueError('Load the native source scene before importing this tileset')
                self.known_tilesets[key] = json.loads(metadata.read_text(encoding='utf-8'))
            native = copy.deepcopy(self.known_tilesets[key])
            first = integer(ts.get('firstgid'), 1, 0x0fffffff)
            end = first + native['tilecount']
            if any(first < b and a < end for a, b in ranges):
                raise ValueError('Overlapping tileset GIDs')
            ranges.append((first, end))
            native['firstgid'] = first
            canonical.append(native)
        if not isinstance(tilemap.get('layers'), list) or not 1 <= len(tilemap['layers']) <= 32:
            raise ValueError('Expected 1..32 tile layers')
        layer_ids = set()
        for layer in tilemap['layers']:
            layer_id = integer(layer.get('id'), 1, 1000000)
            if layer_id in layer_ids:
                raise ValueError('Duplicate layer ID')
            layer_ids.add(layer_id)
            if layer.get('type') != 'tilelayer' or layer.get('width') != width or layer.get('height') != height:
                raise ValueError('Only full-size native tile layers are supported')
            if layer.get('x', 0) != 0 or layer.get('y', 0) != 0:
                raise ValueError('Layer tile offsets are not supported')
            opacity = layer.get('opacity', 1)
            if type(opacity) not in (int, float) or not math.isfinite(opacity) or not 0 <= opacity <= 1:
                raise ValueError('Invalid layer opacity')
            data = layer.get('data')
            if not isinstance(data, list) or len(data) != width * height:
                raise ValueError('Tile layer data length does not match map dimensions')
            for gid in data:
                integer(gid, 0, 0xffffffff)
                value = gid & GID_MASK
                if gid & 0x30000000 or value and not any(a <= value < b for a, b in ranges):
                    raise ValueError('Unsupported flip flags or out-of-range tile GID')
        tilemap['tilesets'] = canonical
        return tilemap

    def save_project(self, project, expected_revision):
        with self.lock:
            project = copy.deepcopy(project)
            path = self._project_path(project.get('id'))
            previous = self.load_project(project['id']) if path.exists() else None
            expected = integer(expected_revision, 0, 1000000000)
            if expected != (previous['revision'] if previous else 0):
                raise ValueError('Project revision changed; reload before saving')
            native = self.scene(project['scene'])['project']
            if project.get('source') != native['source']:
                raise ValueError('Native source hashes changed or were modified')
            project['map'] = self.validate_map(project['map'])
            commands = project.get('commands', [])
            if not isinstance(commands, list) or len(commands) > 5000 or len(json.dumps(commands)) > 4 * 1024 * 1024:
                raise ValueError('Command history exceeds project limit')
            project['revision'], project['updated_at'] = expected + 1, stamp()
            project['world'] = native['world']
            if previous:
                archive = self.root / 'projects' / f'{project["id"]}.r{expected}.json'
                if not archive.exists():
                    write_json(archive, previous)
            write_json(path, project)
            # Canonical descriptors allow safe cross-scene imports after restart.
            for ts in project['map']['tilesets']:
                write_json(self._assets(ts['source_sha256']) / 'tileset.json', self.known_tilesets[ts['source_sha256']])
            return project

    def render_project(self, project):
        tilemap = project['map']
        result = Image.new('RGBA', (tilemap['width'] * 8, tilemap['height'] * 8))
        atlases = [(ts, Image.open(self.asset_path(ts['image'])).convert('RGBA')) for ts in tilemap['tilesets']]
        for layer in tilemap['layers']:
            if not layer.get('visible', True):
                continue
            image = Image.new('RGBA', result.size)
            for offset, gid in enumerate(layer['data']):
                value = gid & GID_MASK
                if not value:
                    continue
                ts, atlas = next((ts, atlas) for ts, atlas in atlases if ts['firstgid'] <= value < ts['firstgid'] + ts['tilecount'])
                local = value - ts['firstgid']
                x, y = local % ts['columns'] * 8, local // ts['columns'] * 8
                tile = atlas.crop((x, y, x + 8, y + 8))
                if gid & FLIP_H:
                    tile = tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
                if gid & FLIP_V:
                    tile = tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
                image.paste(tile, (offset % tilemap['width'] * 8, offset // tilemap['width'] * 8))
            opacity = layer.get('opacity', 1)
            if opacity != 1:
                image.putalpha(image.getchannel('A').point(lambda value:round(value * opacity)))
            result.alpha_composite(image)
        return result

    def export(self, project_id, archive=False):
        project = self.load_project(project_id)
        tilemap = copy.deepcopy(project['map'])
        tilemap['properties'] = [dict(name='editor_source', type='string', value=json.dumps(project['source'])),
                                 dict(name='editor_project', type='string', value=project_id)]
        if not archive:
            return json.dumps(tilemap, ensure_ascii=False, indent=2).encode('utf-8')
        data = io.BytesIO()
        with zipfile.ZipFile(data, 'w', zipfile.ZIP_DEFLATED) as bundle:
            for tileset in tilemap['tilesets']:
                key = tileset['source_sha256']
                tileset['image'] = f'assets/{key}.png'
                bundle.write(self.root / 'assets' / key / 'atlas.png', tileset['image'])
                index = self.root / 'assets' / key / 'indices.json'
                if index.exists():
                    bundle.write(index, f'assets/{key}.indices.json')
            bundle.writestr('map.tmj', json.dumps(tilemap, ensure_ascii=False, indent=2))
            bundle.writestr('project.json', json.dumps(project, ensure_ascii=False, indent=2))
            image = io.BytesIO()
            self.render_project(project).save(image, format='PNG')
            bundle.writestr('preview.png', image.getvalue())
            bundle.writestr('README.txt', 'Native 8x8 tiles extracted from your local cartridge. map.tmj is editable in Tiled.\nOriginal cartridge and human approvals are unchanged. Runtime integration needs a separately reviewed recipe.\n')
        return data.getvalue()

    def asset_path(self, url):
        match = re.fullmatch(r'/editor-assets/([a-f0-9]{64})/(atlas\.png|preview\.png|indices\.json)', url)
        if not match:
            raise ValueError('Invalid editor asset path')
        path = self.root / 'assets' / match[1] / match[2]
        if not path.is_file():
            raise ValueError('Editor asset not found')
        return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--rom', type=Path)
    parser.add_argument('--symbols', type=Path)
    parser.add_argument('--region', choices=['eu', 'us', 'jp'], default='eu')
    parser.add_argument('--scene', default='054')
    parser.add_argument('--pieces', type=Path)
    args = parser.parse_args()
    editor = TileEditor(args.data, args.repo, args.rom, args.symbols, args.region, args.pieces)
    project = editor.new_project(args.scene)
    print(json.dumps(dict(id=project['id'], scene=project['scene'], revision=project['revision'],
        layers=len(project['map']['layers']), tiles=sum(t['tilecount'] for t in project['map']['tilesets']))))


if __name__ == '__main__':
    main()
