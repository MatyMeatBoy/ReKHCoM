'use strict';
/* Tile editor: paints native 8x8 GBA tiles from the scene's own tilesets onto its tile layers.
   The backend (tile_editor.py) validates every save; this page keeps undo/redo as cell diffs. */
const FLIP_H = 0x80000000, FLIP_V = 0x40000000, GID_MASK = 0x0fffffff, T = 8;
const $ = id => document.getElementById(id);
const state = { scenes: [], project: null, images: new Map(), layer: 0, tool: 'pencil', flipH: false, flipV: false,
  brush: { w: 1, h: 1, data: [1] }, undo: [], redo: [], dirty: false, zoom: 2, grid: false, painting: null, sel: null };

function status(text, error) { const s = $('status'); s.textContent = text; s.className = error ? 'err' : ''; }

async function api(path, body) {
  const r = await fetch(path, body ? { method: 'POST', headers: { 'Content-Type': 'application/json', 'X-Art-Review': 'local' }, body: JSON.stringify(body) } : {});
  const data = await r.json().catch(() => ({}));
  if (!r.ok) throw new Error(data.error || r.statusText);
  return data;
}

function loadImage(url) {
  if (state.images.has(url)) return state.images.get(url);
  const p = new Promise((ok, fail) => { const i = new Image(); i.onload = () => ok(i); i.onerror = fail; i.src = url; });
  state.images.set(url, p);
  return p;
}

const map = () => state.project.map;
const layer = () => map().layers[state.layer];
function tilesetOf(gid) {
  const v = gid & GID_MASK;
  return map().tilesets.find(t => t.firstgid <= v && v < t.firstgid + t.tilecount);
}
function isRomDonor(gid) {
  const ts = tilesetOf(gid);
  return !!ts && !(ts.non_donor_tileids || []).includes((gid & GID_MASK) - ts.firstgid);
}
/* The palette shows the active layer's own tileset (the first one its cells use). */
function layerTileset() {
  const used = layer().data.find(g => g & GID_MASK);
  return (used && tilesetOf(used)) || map().tilesets[Math.min(state.layer, map().tilesets.length - 1)];
}

function drawTile(ctx, gid, x, y, s) {
  const ts = tilesetOf(gid);
  if (!ts || !ts.img) return;
  const local = (gid & GID_MASK) - ts.firstgid, sx = local % ts.columns * T, sy = Math.floor(local / ts.columns) * T;
  ctx.save();
  ctx.translate(x + (gid & FLIP_H ? s : 0), y + (gid & FLIP_V ? s : 0));
  ctx.scale(gid & FLIP_H ? -1 : 1, gid & FLIP_V ? -1 : 1);
  ctx.drawImage(ts.img, sx, sy, T, T, 0, 0, s, s);
  ctx.restore();
}

function render() {
  const c = $('map');
  if (!state.project) { c.width = c.height = 0; $('empty').hidden = false; return; }
  $('empty').hidden = true;
  const m = map(), s = T * state.zoom;
  c.width = m.width * s; c.height = m.height * s;
  const ctx = c.getContext('2d');
  ctx.imageSmoothingEnabled = false;
  m.layers.forEach(l => {
    if (l.visible === false) return;
    ctx.globalAlpha = l.opacity ?? 1;
    l.data.forEach((gid, i) => { if (gid & GID_MASK) drawTile(ctx, gid, i % m.width * s, Math.floor(i / m.width) * s, s); });
  });
  ctx.globalAlpha = 1;
  if (state.grid) {
    ctx.strokeStyle = 'rgba(128,128,128,.35)'; ctx.lineWidth = 1; ctx.beginPath();
    for (let x = 0; x <= m.width; x++) { ctx.moveTo(x * s + .5, 0); ctx.lineTo(x * s + .5, c.height); }
    for (let y = 0; y <= m.height; y++) { ctx.moveTo(0, y * s + .5); ctx.lineTo(c.width, y * s + .5); }
    ctx.stroke();
  }
}

function renderPalette() {
  const c = $('palette'), ts = state.project && layerTileset();
  if (!ts || !ts.img) { c.width = c.height = 0; return; }
  const z = 2, rows = Math.ceil(ts.tilecount / ts.columns);
  c.width = ts.columns * T * z; c.height = rows * T * z;
  const ctx = c.getContext('2d');
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(ts.img, 0, 0, ts.img.width * z, ts.img.height * z);
  (ts.non_donor_tileids || []).forEach(local => {
    const x = local % ts.columns * T * z, y = Math.floor(local / ts.columns) * T * z;
    ctx.fillStyle = '#777'; ctx.fillRect(x, y, T * z, T * z);
    ctx.strokeStyle = '#fff'; ctx.beginPath(); ctx.moveTo(x, y); ctx.lineTo(x + T * z, y + T * z); ctx.stroke();
  });
  $('tsName').textContent = ts.name;
  if (state.sel && state.sel.ts === ts) {
    const { x0, y0, x1, y1 } = state.sel;
    ctx.strokeStyle = '#ff3b2f'; ctx.lineWidth = 2;
    ctx.strokeRect(Math.min(x0, x1) * T * z + 1, Math.min(y0, y1) * T * z + 1,
      (Math.abs(x1 - x0) + 1) * T * z - 2, (Math.abs(y1 - y0) + 1) * T * z - 2);
  }
}

function brushGid(i) {
  let gid = state.brush.data[i];
  if (!(gid & GID_MASK)) return 0;
  if (state.flipH) gid ^= FLIP_H;
  if (state.flipV) gid ^= FLIP_V;
  return gid >>> 0;
}
/* The brush as it will land: a flipped block also mirrors its cell order. */
function brushCells() {
  const { w, h } = state.brush, out = [];
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const sx = state.flipH ? w - 1 - x : x, sy = state.flipV ? h - 1 - y : y;
    out.push({ x, y, gid: brushGid(sy * w + sx) });
  }
  return out;
}

function renderBrush() {
  const c = $('brush'), { w, h } = state.brush, s = Math.max(4, Math.floor(64 / Math.max(w, h, 1)));
  c.width = w * s; c.height = h * s;
  const ctx = c.getContext('2d');
  ctx.imageSmoothingEnabled = false;
  if (!state.project) return;
  brushCells().forEach(b => { if (b.gid) drawTile(ctx, b.gid, b.x * s, b.y * s, s); });
}

function renderLayers() {
  const ul = $('layers');
  ul.innerHTML = '';
  if (!state.project) return;
  map().layers.forEach((l, i) => {
    const li = document.createElement('li'), cb = document.createElement('input'), name = document.createElement('span');
    li.className = i === state.layer ? 'active' : '';
    cb.type = 'checkbox'; cb.checked = l.visible !== false; cb.title = 'Visible';
    cb.onclick = e => { e.stopPropagation(); l.visible = cb.checked; markDirty(); render(); };
    name.textContent = l.name;
    li.append(cb, name);
    li.onclick = () => { state.layer = i; state.sel = null; renderLayers(); renderPalette(); };
    ul.append(li);
  });
}

function markDirty(v = true) {
  state.dirty = v;
  $('save').disabled = !state.project || !v;
  $('export').disabled = !state.project || !state.project.id || v;
  $('undo').disabled = !state.undo.length; $('redo').disabled = !state.redo.length;
}

/* A stroke collects {layer, index, before, after} cells; undo/redo replays them. */
function setCell(stroke, i, gid) {
  const l = layer();
  if (l.data[i] === gid) return;
  if (!stroke.cells.has(i)) stroke.cells.set(i, l.data[i]);
  l.data[i] = gid;
}
function commit(stroke) {
  if (!stroke.cells.size) return;
  const cells = [...stroke.cells].map(([i, before]) => [i, before, map().layers[stroke.layer].data[i]]);
  state.undo.push({ layer: stroke.layer, cells, tool: stroke.tool });
  if (state.undo.length > 500) state.undo.shift();
  state.redo = [];
  markDirty();
}
function replay(from, to, index) {
  const op = from.pop();
  if (!op) return;
  op.cells.forEach(c => { map().layers[op.layer].data[c[0]] = c[index]; });
  to.push(op);
  markDirty(); render();
}

function cellAt(e) {
  const r = $('map').getBoundingClientRect(), s = T * state.zoom;
  const x = Math.floor((e.clientX - r.left) / s), y = Math.floor((e.clientY - r.top) / s);
  return x >= 0 && y >= 0 && x < map().width && y < map().height ? { x, y } : null;
}

function apply(stroke, cell) {
  const m = map(), l = layer();
  if (state.tool === 'pick') {
    const gid = l.data[cell.y * m.width + cell.x];
    if (gid && !isRomDonor(gid)) return status('Celda sin tile cargado: no es un donante de la ROM.', true);
    state.brush = { w: 1, h: 1, data: [(gid & GID_MASK) >>> 0] };
    state.flipH = !!(gid & FLIP_H); state.flipV = !!(gid & FLIP_V);
    syncFlips(); renderBrush();
    return;
  }
  if (state.tool === 'erase') return setCell(stroke, cell.y * m.width + cell.x, 0);
  if (state.tool === 'fill') {
    const start = cell.y * m.width + cell.x, target = l.data[start], gid = brushGid(0);
    if (target === gid) return;
    const todo = [start], seen = new Set([start]);
    while (todo.length) {
      const i = todo.pop(), x = i % m.width, y = Math.floor(i / m.width);
      setCell(stroke, i, gid);
      [[x - 1, y], [x + 1, y], [x, y - 1], [x, y + 1]].forEach(([nx, ny]) => {
        const n = ny * m.width + nx;
        if (nx >= 0 && ny >= 0 && nx < m.width && ny < m.height && !seen.has(n) && l.data[n] === target) { seen.add(n); todo.push(n); }
      });
    }
    return;
  }
  brushCells().forEach(b => {
    const x = cell.x + b.x, y = cell.y + b.y;
    if (x < m.width && y < m.height && b.gid) setCell(stroke, y * m.width + x, b.gid);
  });
}

function syncFlips() { $('flipH').classList.toggle('on', state.flipH); $('flipV').classList.toggle('on', state.flipV); }
function setTool(t) {
  state.tool = t;
  document.querySelectorAll('[data-tool]').forEach(b => b.classList.toggle('on', b.dataset.tool === t));
}

async function open(project) {
  $('publish').hidden=!new URLSearchParams(location.search).get('art');
  $('publish').disabled=false;
  state.project = project; state.layer = 0; state.undo = []; state.redo = []; state.sel = null;
  await Promise.all(project.map.tilesets.map(async ts => { ts.img = await loadImage(ts.image); }));
  $('nativeNote').textContent = project.map.tilesets.some(ts => ts.non_donor_tileids?.length)
    ? 'La ROM referencia el tile 1023 sin cargarlo. Se muestra transparente como VRAM inicial limpia; la VRAM de una partida en curso puede diferir. Las celdas conservan su identidad y no se ofrecen como donantes.' : '';
  const first = project.map.layers[0].data.find(g => isRomDonor(g)) || 0;
  state.brush = { w: 1, h: 1, data: [(first & GID_MASK) >>> 0] };
  markDirty(false); renderLayers(); render(); renderPalette(); renderBrush();
  $('scene').value = project.scene;
}

function stripImages(project) {
  const copy = { ...project, map: { ...project.map, tilesets: project.map.tilesets.map(({ img, ...ts }) => ts) } };
  return copy;
}

async function refreshProjects(selected) {
  const { projects } = await api('/api/editor/projects'), sel = $('project');
  sel.innerHTML = '<option value="">—</option>';
  const art=new URLSearchParams(location.search).get('art');
  const links=art?await api('/api/editor/art-projects?parent_id='+encodeURIComponent(art)):null;
  projects.filter(p=>!links||links.ids.includes(p.id)).forEach(p => {
    const o = document.createElement('option');
    o.value = p.id; o.textContent = `${p.scene} · r${p.revision} · ${(p.updated_at || '').slice(0, 16).replace('T', ' ')}`;
    sel.append(o);
  });
  sel.value = selected || '';
}

function confirmLeave() { return !state.dirty || confirm('Hay cambios sin guardar. ¿Descartarlos?'); }

async function init() {
  const list = await api('/api/editor/scenes');
  if (!list.available) { status(list.reason || 'Sin cartucho local', true); return; }
  state.scenes = list.scenes;
  const sel = $('scene');
  list.scenes.forEach(s => {
    const o = document.createElement('option');
    o.value = s.id; o.textContent = s.name; o.disabled = !s.supported;
    sel.append(o);
  });
  await refreshProjects();
  const art=new URLSearchParams(location.search).get('art');
  if(art){
    document.querySelector('.back').href='/?image='+encodeURIComponent(art)+'&view=history';
    $('publish').hidden=false;$('publish').disabled=true;
    status('Elige la escena de la ROM para este arte y pulsa Nuevo proyecto.');
    // Keep only projects linked to this exact artwork revision.
    const links=await api('/api/editor/art-projects?parent_id='+encodeURIComponent(art));
    const ids=new Set(links.ids);
    for(const option of [...$('project').options])if(option.value&&!ids.has(option.value))option.remove();
    if(links.ids.length){$('project').value=links.ids.at(-1);$('project').dispatchEvent(new Event('change'));}
  }else status(`${list.scenes.length} escenas del cartucho local`);
}

$('new').onclick = async () => {
  if (!confirmLeave()) return;
  try {
    status('Creando…');
    const art=new URLSearchParams(location.search).get('art');
    const { project } = await api(art?'/api/editor/art':'/api/editor/project/new', { scene: $('scene').value, parent_id:art });
    await open(project); await refreshProjects(project.id);
    status(`Proyecto nuevo de la escena ${project.scene}`);
  } catch (e) { status(e.message, true); }
};
$('project').onchange = async e => {
  if (!e.target.value) return;
  if (!confirmLeave()) { e.target.value = state.project ? state.project.id : ''; return; }
  try { const { project } = await api('/api/editor/project?id=' + e.target.value); await open(project); status(`Abierto r${project.revision}`); }
  catch (err) { status(err.message, true); }
};
$('save').onclick = async () => {
  try {
    const body = stripImages(state.project);
    body.commands = state.undo.slice(-200).map(op => ({ layer: op.layer, tool: op.tool, cells: op.cells }));
    const { project } = await api('/api/editor/project', { project: body, expected_revision: state.project.revision });
    const images = new Map(state.project.map.tilesets.map(t => [t.source_sha256, t.img]));
    project.map.tilesets.forEach(t => { t.img = images.get(t.source_sha256); });
    state.project = project;
    markDirty(false); await refreshProjects(project.id);
    status(`Guardado r${project.revision}`);
  } catch (e) { status(e.message, true); }
};
$('publish').onclick = async () => {
  const button=$('publish');button.disabled=true;
  try {
    if(state.dirty) { await $('save').onclick(); if(state.dirty) return; }
    const child=await api('/api/editor/publish',{id:state.project.id});
    location.href='/?image='+encodeURIComponent(child.id)+'&view=history';
  } catch(e) { status(e.message,true); } finally {button.disabled=false;}
};
$('export').onclick = () => { location.href = '/api/editor/export?format=zip&id=' + state.project.id; };
$('undo').onclick = () => replay(state.undo, state.redo, 1);
$('redo').onclick = () => replay(state.redo, state.undo, 2);
$('flipH').onclick = () => { state.flipH = !state.flipH; syncFlips(); renderBrush(); };
$('flipV').onclick = () => { state.flipV = !state.flipV; syncFlips(); renderBrush(); };
$('zoom').oninput = e => { state.zoom = +e.target.value; render(); };
$('grid').onchange = e => { state.grid = e.target.checked; render(); };
document.querySelectorAll('[data-tool]').forEach(b => { b.onclick = () => setTool(b.dataset.tool); });

const mapCanvas = $('map');
mapCanvas.onpointerdown = e => {
  if (!state.project || e.button !== 0) return;
  const cell = cellAt(e);
  if (!cell) return;
  mapCanvas.setPointerCapture(e.pointerId);
  state.painting = { layer: state.layer, cells: new Map(), tool: state.tool, last: null };
  apply(state.painting, cell); state.painting.last = cell.x + ',' + cell.y; render();
};
mapCanvas.onpointermove = e => {
  if (!state.project) return;
  const cell = cellAt(e);
  $('cursor').textContent = cell ? `Tile ${cell.x}, ${cell.y} · ${layer().name}` : '';
  if (!state.painting || !cell || state.tool === 'fill') return;
  const key = cell.x + ',' + cell.y;
  if (key === state.painting.last) return;
  state.painting.last = key;
  apply(state.painting, cell); render();
};
mapCanvas.onpointerup = mapCanvas.onpointercancel = () => {
  if (state.painting) commit(state.painting);
  state.painting = null;
};

/* Palette: click picks one tile, a drag picks a block. */
const pal = $('palette');
function palCell(e) {
  const r = pal.getBoundingClientRect(), ts = layerTileset();
  const x = Math.floor((e.clientX - r.left) / (T * 2)), y = Math.floor((e.clientY - r.top) / (T * 2));
  return { x: Math.max(0, Math.min(ts.columns - 1, x)), y: Math.max(0, y), ts };
}
function selectBrush() {
  const { x0, y0, x1, y1, ts } = state.sel, data = [];
  const ax = Math.min(x0, x1), ay = Math.min(y0, y1), w = Math.abs(x1 - x0) + 1, h = Math.abs(y1 - y0) + 1;
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const local = (ay + y) * ts.columns + ax + x;
    data.push(local < ts.tilecount ? ts.firstgid + local : 0);
  }
  if (data.some(gid => gid && !isRomDonor(gid))) {
    status('La selección incluye una celda sin tile cargado; elige sólo tiles de la ROM.', true);
    return;
  }
  state.brush = { w, h, data };
  if (state.tool !== 'fill') setTool('pencil');
  renderBrush(); renderPalette();
}
pal.onpointerdown = e => {
  if (!state.project) return;
  pal.setPointerCapture(e.pointerId);
  const c = palCell(e);
  state.sel = { x0: c.x, y0: c.y, x1: c.x, y1: c.y, ts: c.ts, dragging: true };
  selectBrush();
};
pal.onpointermove = e => {
  if (!state.sel || !state.sel.dragging) return;
  const c = palCell(e);
  if (c.x === state.sel.x1 && c.y === state.sel.y1) return;
  state.sel.x1 = c.x; state.sel.y1 = c.y; selectBrush();
};
pal.onpointerup = () => { if (state.sel) state.sel.dragging = false; };

document.addEventListener('keydown', e => {
  if (e.target.matches('input, select')) return;
  const k = e.key.toLowerCase();
  if ((e.ctrlKey || e.metaKey) && k === 'z') { e.preventDefault(); (e.shiftKey ? $('redo') : $('undo')).click(); }
  else if ((e.ctrlKey || e.metaKey) && k === 'y') { e.preventDefault(); $('redo').click(); }
  else if ((e.ctrlKey || e.metaKey) && k === 's') { e.preventDefault(); if (!$('save').disabled) $('save').click(); }
  else if (k === 'b') setTool('pencil');
  else if (k === 'i') setTool('pick');
  else if (k === 'e') setTool('erase');
  else if (k === 'g') setTool('fill');
  else if (k === 'x') $('flipH').click();
  else if (k === 'y') $('flipV').click();
});
window.addEventListener('beforeunload', e => { if (state.dirty) { e.preventDefault(); e.returnValue = ''; } });

init().catch(e => status(e.message, true));
