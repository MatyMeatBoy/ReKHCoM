"""Internal Atlas presentation: shared sprite cuts, or a scene for context.

Artwork is provided by a native-index generator or an artist. This module
never invents an approval, edits a parent, or renders a boss over an arena
when the feedback is about its sprite.
"""
from pathlib import Path
from PIL import Image, ImageDraw


def review_mode(feedback, has_animation=False):
    text = feedback.lower()
    if any(w in text for w in
           ('sprite', 'pose', 'animaci', 'ala', 'brazo', 'difumin', 'lomo', 'cola del jefe')):
        return 'sprite'
    if any(w in text for w in ('arena', 'escena', 'suelo', 'muro', 'fondo', 'pared')):
        return 'scene'
    return 'sprite' if has_animation else 'scene'


def comparison(before, after, output, title, subtitle='', scale=2):
    """Equal canvas, origin, density and background; one distinct cut per review."""
    assert before.size == after.size
    w, h = before.size
    board = Image.new('RGB', (w * scale * 2, h * scale + 64), '#d8d8df')
    draw = ImageDraw.Draw(board)
    draw.text((12, 7), title, fill='#171725')
    draw.text((12, 25), subtitle, fill='#363646')
    for side, (label, im) in enumerate((('ANTES', before), ('DESPUES', after))):
        x = side * w * scale
        draw.text((x + 12, 45), label, fill='#171725')
        rgba = im.convert('RGBA').resize((w * scale, h * scale), Image.Resampling.NEAREST)
        board.paste(rgba, (x, 64), rgba)
    board.save(output)


def group_cut_regions(frames):
    """One proposed completion per shared cut, with all reused poses recorded.

    Hash the original boundary strip, not the assembled pose: moving a healthy
    arm must not create another review of the same missing body region.
    Frames without a cut are coverage only, never new completion proposals.
    """
    groups = {}
    for frame in frames:
        signature = frame.get('seam_hash')
        if not signature:
            continue
        group = groups.setdefault(signature, {
            'id': 'cut-%02d' % (len(groups) + 1),
            'representative': frame['frame'], 'frames': [], 'sources': [],
            'cut_bounds': frame.get('cut_bounds'),
        })
        group['frames'].append(frame['frame'])
        if frame['source'] not in group['sources']:
            group['sources'].append(frame['source'])
    return list(groups.values())
