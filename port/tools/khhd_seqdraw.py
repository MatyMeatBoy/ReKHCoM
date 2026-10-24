"""Python twin of port_hdlayout.c's drawing (rotation included) to debug a sequence frame by frame."""
import math, sys
sys.path.insert(0, str(__import__('pathlib').Path(__file__).parent))
import khhd_layout as L
from PIL import Image, ImageDraw

def lerp(t, a, b): return a * (1 - t) + b * t

def draw(canvas, tex, s, ag, frame, ox, oy, scale, uv=1.0, only=None, label=False):
    d = ImageDraw.Draw(canvas)
    clip = None
    for idx, a in enumerate(ag['anims']):
        if frame < a['f0'] or frame > a['f1']: continue
        if only is not None and idx not in only: continue
        delta = (frame - a['f0']) / (a['f1'] - a['f0']) if a['f1'] > a['f0'] else 0.0
        fl = a['flags']
        t = delta if fl & 1 else (math.sin(delta * math.pi - math.pi / 2) + 1) / 2
        x = a['tx0'] if fl & 0x1000 else lerp(t, a['tx0'], a['tx1'])
        y = a['ty0'] if fl & 0x1000 else lerp(t, a['ty0'], a['ty1'])
        if not fl & 0x40:
            sc = lerp(t, a['s0'], a['s1']); sx = sc * lerp(t, a['sx0'], a['sx1']); sy = sc * lerp(t, a['sy0'], a['sy1'])
        else: sx = sy = 1
        rz = 0 if fl & 0x20 else lerp(t, a['rz0'], a['rz1'])
        px = 0 if fl & 0x4000 else lerp(t, a['px0'], a['px1']); py = 0 if fl & 0x4000 else lerp(t, a['py0'], a['py1'])
        col = [1.0] * 4
        if not fl & 0x400:
            c0 = [((a['c0'] >> s_) & 255) / 128 for s_ in (0, 8, 16, 24)]
            c1 = [((a['c1'] >> s_) & 255) / 128 for s_ in (0, 8, 16, 24)]
            col = c0 if fl & 0x80 else [lerp(t, c0[k], c1[k]) for k in range(4)]
        for part in s['groups'][a['group']]:
            sp = s['sprites'][part['sprite']]
            mask = sp['u1'] == sp['u0'] and sp['v1'] == sp['v0']
            pts = []
            for cx, cy in ((part['l'], part['t']), (part['r'], part['t']), (part['r'], part['b']), (part['l'], part['b'])):
                X, Y = (cx + px) * sx, (cy + py) * sy
                c, sn = math.cos(-rz), math.sin(-rz)
                X, Y = X * c - Y * sn, X * sn + Y * c
                pts.append((ox + (X + x) * scale, oy + (Y + y) * scale))
            if mask:
                clip = (min(p[0] for p in pts), min(p[1] for p in pts), max(p[0] for p in pts), max(p[1] for p in pts))
                continue
            # draw the texture piece as an affine quad (PIL transform from dest quad)
            u0, v0, u1, v1 = [v * uv for v in (sp['u0'], sp['v0'], sp['u1'], sp['v1'])]
            piece = tex.crop((int(min(u0, u1)), int(min(v0, v1)), max(int(max(u0, u1)), int(min(u0, u1)) + 1),
                              max(int(max(v0, v1)), int(min(v0, v1)) + 1)))  # a zero-wide source: one texel column
            if u1 < u0: piece = piece.transpose(Image.FLIP_LEFT_RIGHT)
            if v1 < v0: piece = piece.transpose(Image.FLIP_TOP_BOTTOM)
            W, H = canvas.size
            # quad transform: map dest quad -> source rect
            q = pts
            data = (0, 0, 0, piece.height, piece.width, piece.height, piece.width, 0)
            # PIL QUAD maps a source quad to the whole output; do it inversely by drawing via mesh on a temp
            minx, miny = min(p[0] for p in q), min(p[1] for p in q)
            maxx, maxy = max(p[0] for p in q), max(p[1] for p in q)
            bw, bh = int(maxx - minx) + 2, int(maxy - miny) + 2
            if bw <= 0 or bh <= 0 or bw > 4000 or bh > 4000: continue
            # inverse affine from three corners
            (x0, y0), (x1, y1), _, (x3, y3) = q
            ax, ay, bx, by = x1 - x0, y1 - y0, x3 - x0, y3 - y0
            det = ax * by - ay * bx
            if abs(det) < 1e-6: continue
            ia, ib, ic, id_ = by / det, -bx / det, -ay / det, ax / det
            pw, ph = piece.width, piece.height
            # output pixel (X,Y) in temp -> canvas (X+minx, Y+miny) -> (s,t) in [0,1]
            a0 = ia * pw; a1 = ib * pw; b0 = ic * ph; b1 = id_ * ph
            cx0, cy0 = minx - x0, miny - y0
            coeffs = (a0, a1, a0 * cx0 + a1 * cy0, b0, b1, b0 * cx0 + b1 * cy0)
            tmp = piece.transform((bw, bh), Image.AFFINE, coeffs, Image.BILINEAR)
            sc_ = [sum(((sp[k] >> s_) & 255) / 128 for k in ('cl', 'ct', 'cr', 'cb')) / 4 for s_ in (0, 8, 16, 24)]
            mul = [min(2.0, sc_[k] * col[k]) for k in range(4)]
            ch = tmp.split()
            tmp = Image.merge('RGBA', [ch[k].point(lambda v, m=mul[k]: min(255, int(v * m))) for k in range(4)])
            if clip:
                m = Image.new('L', tmp.size, 0)
                ImageDraw.Draw(m).rectangle((clip[0] - int(minx), clip[1] - int(miny), clip[2] - int(minx) - 1, clip[3] - int(miny) - 1), fill=255)
                r, g, b, al = tmp.split(); al = Image.composite(al, m, m); tmp = Image.merge('RGBA', (r, g, b, al))
            canvas.alpha_composite(tmp, (int(minx), int(miny)))
            if label:
                d.text(((q[0][0] + q[2][0]) / 2, (q[0][1] + q[2][1]) / 2), f"{idx}:g{a['group']}", fill=(255, 255, 0, 255))
