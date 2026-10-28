"""Names for Options > Scenes (port/src/port_scene_gallery_menu.inc), in the six menu languages, built from the event
ids of port/src/port_hdscenes.inc ("EVENT_046_11F_TWILIGHT_TOWN_E1_2" -> "11F · Twilight Town · Event 1 (2)"). The
cartridge's own event names are Japanese debug text, so these are the port's. Writes port/src/port_scene_names.inc.
The floors' generic scenes (entrance, end of floor, scene) also name who appears in them, read from their scripts
(src/evt/events/NNN_*.inc). Story order: the floors in order, the worlds' scenes (chosen in any order between 2F and
10F) after 10F and before 11F, Riku's after Sora's (#163: Monstro and 100 Acre Wood were listed after the ending).

    python port/tools/gen_scene_names.py
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / 'include/evt/event_ids.h'
OUT = ROOT / 'port/src/port_scene_names.inc'
LANGS = 6  # EN FR DE IT ES JA (port_ui.c's order)

WORDS = {
    'ENTRANCE': ('Entrance', 'Entrée', 'Eingang', 'Ingresso', 'Entrada', 'いりぐち'),
    'GOAL': ('End of floor', "Fin d'étage", 'Etagenende', 'Fine del piano', 'Final del piso', 'さいご'),
    'DEMO': ('Scene', 'Scène', 'Szene', 'Scena', 'Escena', 'シーン'),
    'LAST': ('Finale', 'Finale', 'Finale', 'Finale', 'Desenlace', 'クライマックス'),
    'OPNING': ('Opening', 'Ouverture', 'Auftakt', 'Apertura', 'Apertura', 'はじまり'),
    'ENDING': ('Ending', 'Fin', 'Ende', 'Finale', 'Final', 'エンディング'),
    'DARK': ('Darkness', 'Ténèbres', 'Dunkelheit', 'Oscurità', 'Oscuridad', 'やみ'),
    'LIGHT': ('Light', 'Lumière', 'Licht', 'Luce', 'Luz', 'ひかり'),
    'E': ('Event', 'Événement', 'Ereignis', 'Evento', 'Evento', 'イベント'),
}
PLACES = {
    'TWILIGHT_TOWN': ('Twilight Town', 'Cité du Crépuscule', 'Stadt des Zwielichts', 'Crepuscopoli', 'Villa Crepúsculo',
                      'トワイライトタウン'),
    'DESTINY_ISLAND': ('Destiny Islands', 'Îles du Destin', 'Insel des Schicksals', 'Isole del Destino',
                       'Islas del Destino', 'デスティニーアイランド'),
    'CASTLE_OBLIVION': ('Castle Oblivion', 'Manoir Oblivion', 'Schloss des Entfallens', "Castello dell'Oblio",
                        'Castillo del Olvido', 'わすれじのしろ'),
    'CASLTE_OBLIVION': None,  # the cartridge's typo, same place
    'CAPSULE_ROOM': ('Capsule room', 'Salle des capsules', 'Kapselraum', 'Sala delle capsule', 'Sala de cápsulas',
                     'カプセルのへや'),
}
PLACES['CASLTE_OBLIVION'] = PLACES['CASTLE_OBLIVION']
PLACES.update({
    'TRAVERSE_TOWN': ('Traverse Town', 'Ville de Traverse', 'Traverse', 'Città di Mezzo', 'Ciudad de Paso', 'トラヴァースタウン'),
    'MONSTORO': ('Monstro', 'Monstro', 'Monstro', 'La Balena', 'Monstruo', 'モンストロ'),
    'HALLOWEEN_TOWN': ('Halloween Town', 'Ville Halloween', 'Halloween Town', 'Città di Halloween', 'Ciudad de Halloween', 'ハロウィンタウン'),
    'WONDERLAND': ('Wonderland', 'Pays des Merveilles', 'Wunderland', 'Paese delle Meraviglie', 'País de las Maravillas', 'ワンダーランド'),
    'ATLANTICA': ('Atlantica', 'Atlantica', 'Atlantica', 'Atlantica', 'Atlántica', 'アトランティカ'),
    'AGRABAH': ('Agrabah', 'Agrabah', 'Agrabah', 'Agrabah', 'Agrabah', 'アグラバー'),
    'NEVERLAND': ('Neverland', 'Pays Imaginaire', 'Nimmerland', 'Isola che non c’è', 'País de Nunca Jamás', 'ネバーランド'),
    'COLISEUM': ('Olympus Coliseum', 'Colisée de l’Olympe', 'Olymp', 'Monte Olimpo', 'Coliseo del Olimpo', 'オリンポスコロシアム'),
    'HOLLOWBASTION': ('Hollow Bastion', 'Forteresse Oubliée', 'Hollow Bastion', 'Fortezza Oscura', 'Bastión Hueco', 'ホロウバスティオン'),
    '100ACREWOOD': ('100 Acre Wood', 'Forêt des Rêves Bleus', 'Hundertmorgenwald', 'Bosco dei 100 Acri', 'Bosque de los Cien Acres', '１００エーカーのもり'),
})
WORDS.update({
    'START': ('Beginning', 'Début', 'Anfang', 'Inizio', 'Comienzo', 'はじまり'),
    'END': ('Ending', 'Fin', 'Ende', 'Finale', 'Final', 'おわり'),
    'BOSS': ('Boss encounter', 'Rencontre du boss', 'Bossbegegnung', 'Incontro col boss', 'Encuentro con el jefe', 'ボス'),
    'BATTLE': ('Battle', 'Combat', 'Kampf', 'Battaglia', 'Combate', 'たたかい'),
})


EVENTS = ROOT / 'src/evt/events'
# who appears in a scene, in the six languages; the party, Jiminy, objects and effects are left out
CHARACTERS = {
    'AXEL': ('Axel',) * 5 + ('アクセル',),
    'LARXENE': ('Larxene',) * 5 + ('ラクシーヌ',),
    'VEXEN': ('Vexen',) * 5 + ('ヴィクセン',),
    'MARLUXIA': ('Marluxia',) * 5 + ('マールーシャ',),
    'LEXAEUS': ('Lexaeus',) * 5 + ('レクセウス',),
    'ZEXION': ('Zexion',) * 5 + ('ゼクシオン',),
    'NAMINE': ('Naminé',) * 5 + ('ナミネ',),
    'RIKU_REPLICA': ('Riku Replica', 'Réplique de Riku', 'Riku-Replik', 'Replica di Riku', 'Réplica de Riku', 'リク＝レプリカ'),
    'RIKU': ('Riku',) * 5 + ('リク',),
    'ANSEM': ('Ansem',) * 5 + ('アンセム',),
    'DIZ': ('DiZ',) * 5 + ('ディズ',),
    'MICKEY': ('Mickey', 'Mickey', 'Micky', 'Topolino', 'Mickey', 'ミッキー'),
    'KAIRI': ('Kairi',) * 5 + ('カイリ',),
    'ROBED_FIGURE': ('Hooded figure', 'Silhouette encapuchonnée', 'Kapuzengestalt', 'Figura incappucciata',
                     'Figura encapuchada', 'フードの男'),
}
GENERIC = ('ENTRANCE', 'GOAL', 'DEMO')


def cast(ident):
    """The scene's named characters (script order of first appearance), without the story's lead."""
    number = int(ident.split('_')[1])
    files = sorted(EVENTS.glob(f'{number:03d}_*.inc'))
    if not files:
        return []
    found = []
    for token in re.findall(r'CHARA_([A-Z0-9_]+)', files[0].read_text(encoding='utf-8', errors='replace')):
        if token in CHARACTERS and token not in found and not (token == 'RIKU' and '_RIKU_' in ident):
            found.append(token)
    return found[:3]


def story_order(ident):
    """Floors in order, then the worlds' scenes between 10F and 11F, Riku's after Sora's; cinematics fit between."""
    number = int(ident.split('_')[1])
    if 74 <= number <= 148:  # Monstro .. 100 Acre Wood: worlds of the middle floors, never after the ending
        return 42 * 100 + 1 + (number - 74)  # 4201..4275, between 10F (4200) and 11F (4300)
    return number * 100


def name(ident):
    m = re.match(r'EVENT_(\d+)_?(.*)', ident)
    rest = m.group(2)
    riku = rest.startswith('RIKU_')
    rest = rest[5:] if riku else rest
    floor = re.match(r'(B?\d+F)_?(.*)', rest)
    out = []
    for lang in range(LANGS):
        parts = ['Riku'] if riku else []
        tail = rest
        if floor:
            parts.append(floor.group(1))
            tail = floor.group(2)
        for key, words in PLACES.items():
            if tail.startswith(key):
                parts.append(words[lang])
                tail = tail[len(key):].lstrip('_')
                break
        kind = ''
        tokens = [t for t in tail.split('_') if t]
        if tokens:
            first = tokens[0]
            e = re.match(r'E(\d+)$', first)
            last = re.match(r'LAST(\d+)$', first)
            level = re.match(r'LV(\d+)$', first)
            if e:
                kind = f'{WORDS["E"][lang]} {int(e.group(1)) + 1}'
            elif last:
                kind = WORDS['LAST'][lang] + ' ' + last.group(1)
            elif level:
                kind = WORDS['DEMO'][lang] + ' ' + level.group(1)
            elif re.match(r'PART(\d+)$', first):
                kind = ''
                tokens.append(first[4:])
            else:
                kind = WORDS.get(first, (first.title(),) * LANGS)[lang]
            nums = [t[4:] if t.startswith('PART') else t for t in tokens[1:] if re.match(r'(PART)?\d+$', t)]
            if nums:
                n = ' '.join(nums)
                kind = (kind + (f' ({n})' if kind[-1:].isdigit() else ' ' + n)) if kind else n
        if kind:
            parts.append(kind)
        if tokens and tokens[0] in GENERIC:
            who = cast(ident)
            if who:
                parts.append(', '.join(CHARACTERS[c][lang] for c in who))
        out.append(' · '.join(parts) if parts else f'#{int(m.group(1)):03d}')
    return out


def main():
    text = SRC.read_text(encoding='utf-8')
    idents = [ident for ident in re.findall(r'\bEVENT_\d+_[A-Z0-9_]+', text) if '_DAMI' not in ident]
    seen = []
    for ident in idents:
        if ident not in seen:
            seen.append(ident)
    lines = ['/* Generated by port/tools/gen_scene_names.py from event_ids.h: EN FR DE IT ES JA. */',
             'static const struct { unsigned short eventId; short storyOrder; const char* name[6]; } kSceneNames[] = {']
    for ident in seen:
        cells = ', '.join('"' + n.replace('"', '\\"') + '"' for n in name(ident))
        order = story_order(ident)
        lines.append(f'    {{ {ident}, {order}, {{ {cells} }} }},')
    movies = [
        (1, -1, ('Sora · Opening cinematic', 'Sora · Cinématique initiale', 'Sora · Introfilm', 'Sora · Filmato iniziale', 'Sora · Cinemática inicial', 'ソラ · オープニング')),
        (2, 2550, ('Sora · 6F · Cinematic', 'Sora · 6F · Cinématique', 'Sora · 6F · Film', 'Sora · 6F · Filmato', 'Sora · 6F · Cinemática', 'ソラ · 6F · ムービー')),
        (3, 5650, ('Sora · 12F · Cinematic', 'Sora · 12F · Cinématique', 'Sora · 12F · Film', 'Sora · 12F · Filmato', 'Sora · 12F · Cinemática', 'ソラ · 12F · ムービー')),
        (4, 7350, ('Sora · Ending cinematic', 'Sora · Cinématique finale', 'Sora · Schlussfilm', 'Sora · Filmato finale', 'Sora · Cinemática final', 'ソラ · エンディング')),
        (5, 19550, ('Riku · Ending cinematic', 'Riku · Cinématique finale', 'Riku · Schlussfilm', 'Riku · Filmato finale', 'Riku · Cinemática final', 'リク · エンディング')),
    ]
    for movie, order, names in movies:
        cells = ', '.join('"' + n.replace('"', '\\"') + '"' for n in names)
        lines.append(f'    {{ PORT_SCENE_MOVIE({movie}), {order}, {{ {cells} }} }},')
    lines.append('};')
    OUT.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    print(OUT, len(seen), 'events +', len(movies), 'cinematics')
    for ident in seen[:6] + seen[-3:]:
        print(' ', ident, '->', name(ident)[0], '|', name(ident)[4])


if __name__ == '__main__':
    main()
