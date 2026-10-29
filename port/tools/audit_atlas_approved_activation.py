#!/usr/bin/env python3
"""Inventory every authoritative Atlas build_version without modifying the desk.

Input is the JSON response from Atlas /api/state. A source/default gate match
is deliberately not runtime evidence. With --desk, verify each accepted PNG
hash and record encoded and decoded sizes; no image content is exported.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def source_contract(asset, source, ready, default_scenes):
    title = asset['title'].lower()
    match = re.search(r'(?:escena\s*|coliseo\s*)(\d{3})', title)
    if asset['id'] == '1de7c0eee9be421884c5121e27ead2a9':
        match = re.search(r'(004)', '004')
    if match and 'guard armor' not in title:
        scene = int(match.group(1))
        if scene in [57, 61, 74]:
            return {'kind': 'native_scene', 'scene': scene,
                    'source': [f'port/include/port/port_scene{scene:03}_tiles.h'],
                    'default_source_gate': 'dedicated_profile', 'import': 'not_required_native_tiles'}
        return {'kind': 'native_scene', 'scene': scene,
                'source': ['port/include/port/port_preliminar_v1_tiles.h',
                           'port/include/port/port_preliminar_v1_data.inc'],
                'recipe_ready': scene in ready,
                'default_source_gate': 'listed' if scene in default_scenes else 'MISSING',
                'import': 'not_required_native_tiles'}
    if 'oogie' in title or 'trickmaster' in title:
        name, gate = ('oogie', 'oogieHall') if 'oogie' in title else ('trickmaster', 'tmBoard')
        expression = re.search(r'int '+gate+r'\s*=\s*(.*?);', source, re.S).group(1)
        return {'kind': 'native_arena', 'source': [f'port/src/port_{name}.c'],
                'default_source_gate': 'enabled' if 'PortArenasApproved()' in expression else 'MISSING',
                'gate_expression': ' '.join(expression.split()), 'import': 'not_required_native_tiles'}
    if 'darkside' in title:
        return {'kind': 'native_boss', 'source': ['port/src/port_darkside.c',
                *[f'port/include/port/darkside_body_{name}.inc' for name in
                  ['idle', 'crouch', 'raise', 'claw', 'lean', 'punch', 'reach']]],
                'default_source_gate': 'STATIC_enabled_PortDarksideBeachOn_unset_or_nonzero',
                'import': 'not_required_native_tiles'}
    if any(word in title for word in ['malefica', 'maléfica', 'marluxia', 'rsula', 'jafar', 'guard armor']):
        name = next(name for word, name in [('malefica', 'maleficent'), ('maléfica', 'maleficent'),
                    ('marluxia', 'marluxia'), ('rsula', 'ursula'), ('jafar', 'jafar'),
                    ('guard armor', 'guard_armor')] if word in title)
        gate = {'maleficent': 'STATIC_enabled_PortArenasApproved',
                'ursula': 'STATIC_enabled_PortArenasApproved',
                'marluxia': 'STATIC_enabled_PortArenasApproved_when_field_matches',
                'jafar': 'STATIC_enabled_PortJafarWide_unset_or_nonzero',
                'guard_armor': 'STATIC_enabled_when_map_matches_and_no_explicit_NO_GUARD_PATIO'}[name]
        return {'kind': 'native_boss', 'source': [f'port/src/port_{name}.c', 'port/src/port_ppu.c'],
                'default_source_gate': gate, 'import': 'not_required_native_tiles'}
    if 'agrabah' in title:
        return {'kind': 'native_scene', 'source': ['port/include/port/port_agrabah_tiles.h'],
                'default_source_gate': 'STATIC_enabled_PortArtOn_unset_or_nonzero',
                'import': 'not_required_native_tiles'}
    return {'kind': 'menu_hud_or_other', 'source': [
            'port/src/port_main.c', 'port/src/port_ppu.c', 'port/src/port_hdsprites.c',
            'port/include/port/hd_sprites.inc', 'port/data/hd_sprites.tsv',
            'port/src/port_config.c', 'port/src/port_ui.c'],
            'default_source_gate': 'menu_modes_registered_HD_groups_require_hdCards_or_hdTitle',
            'import': 'needs_current_asset_and_binding_check'}


def inventory(state, desk=None):
    assets = {asset['id']: asset for asset in state['assets']}
    source = (ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8')
    defaults = set(map(int, re.findall(r'\d+', re.search(
        r'kPreliminarApproved\[\] = \{(.*?)\}', source, re.S).group(1))))
    recipes = (ROOT/'port/tools/preliminar_v1_recipes.py').read_text(encoding='utf-8')
    ready = set(map(int, re.findall(r'\d+', re.search(r'READY=\{(.*?)\}', recipes).group(1))))
    documents = [(str(path.relative_to(ROOT)), path.read_text(encoding='utf-8', errors='replace'))
                 for path in (ROOT/'port/progress').glob('*.md')]
    rows = []
    blob_cache = {}
    def source_file(name):
        if name not in blob_cache:
            path = ROOT/name
            blob_cache[name] = {'path': name, 'exists': path.exists(), 'git_blob': subprocess.check_output(['git', '-C', str(ROOT), 'hash-object', name], text=True).strip() if path.exists() else None}
        return blob_cache[name]
    for approval in state['approvals']:
        accepted = approval.get('approved_version')
        if not accepted:
            continue
        asset = assets[accepted]
        row = {key: asset.get(key) for key in ['title', 'id', 'status', 'sha256', 'width', 'height']}
        row.update(family_id=approval['family_id'], approved_version=accepted,
                   current_feedback_status=asset['status'],
                   build_version=approval.get('build_version'),
                   approval_matches_build=accepted == approval.get('build_version'),
                   pending_descendants=[v for v in approval.get('pending_ids', []) if v != accepted],
                   rgba_bytes=asset['width']*asset['height']*4,
                   runtime='NOT_VERIFIED_BY_THIS_INVENTORY',
                   current_source=source_contract(asset, source, ready, defaults))
        row['historical_reference_documents'] = [name for name, text in documents if accepted in text]
        row['source_equivalence_to_approved_image'] = 'NOT_PROVEN_BY_TITLE_OR_DOCUMENT_REFERENCE'
        row['current_source']['files'] = [source_file(name) for name in row['current_source']['source']]
        if asset['status'] not in ['approved', 'honors']:
            row['warning'] = 'accepted base remains valid; later feedback is pending follow-up, not revocation'
        if desk is not None:
            path = desk/'images'/f'{accepted}.png'
            if path.exists():
                row.update(png_bytes=path.stat().st_size,
                           png_hash_matches=hashlib.sha256(path.read_bytes()).hexdigest() == asset['sha256'])
            else:
                row['warning'] = 'accepted image missing from supplied desk'
        rows.append(row)
    return {'schema': 'rekhcom.atlas-accepted-activation/1',
            'source_commit': subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip(),
            'source_worktree_dirty': subprocess.run(['git', '-C', str(ROOT), 'diff', '--quiet']).returncode != 0,
            'families_total': len(state['approvals']), 'accepted_total': len(rows), 'accepted': rows,
            'source_gate_gaps': [r['id'] for r in rows if r['current_source']['default_source_gate'] == 'MISSING'],
            'scope': 'Complete accepted-version inventory; source gate inspection is not runtime certification.'}


def validate_complete(state, report):
    """Fail closed if an accepted family/version was dropped or substituted."""
    expected = {a['family_id']: a['approved_version'] for a in state['approvals']
                if a.get('approved_version')}
    actual = {a['family_id']: a['approved_version'] for a in report['accepted']}
    if expected != actual or len(actual) != len(report['accepted']):
        raise ValueError('Atlas accepted-version inventory is incomplete, duplicated or substituted')
    if report['accepted_total'] != len(expected):
        raise ValueError('Atlas accepted-version count differs from authoritative selection')


def attach_resource_receipt(report, receipt):
    """Report importer/registry coverage without claiming exact-image parity."""
    report['current_resource_audit'] = receipt['summary']
    for row in report['accepted']:
        if row['current_source']['kind'] == 'menu_hud_or_other':
            row['import_chain'] = {
                'recipe_inventory': receipt['summary'].get('inventory_status', 'NOT_SUPPLIED'),
                'compiled_registry': receipt['summary'].get('compiled_registry_check', 'NOT_SUPPLIED'),
                'resource_bytes': receipt['summary']['encoded_bytes'],
                'potential_decoded_rgba_bytes': receipt['summary']['rgba_bytes'],
                'exact_family_resource_mapping': 'PENDING_targeted_consumer_and_frame_review',
                'runtime': 'NOT_CERTIFIED_BY_GLOBAL_INVENTORY'}
        else:
            row['import_chain'] = {'recipe_inventory': 'native_ROM_palette_tiles_no_Atlas_PNG_import',
                                   'runtime': 'NOT_CERTIFIED_BY_GLOBAL_INVENTORY'}


def attach_runtime_receipt(report, receipt):
    accepted = {row['approved_version']: row for row in report['accepted']}
    for version, proof in receipt.items():
        if version not in accepted:
            raise ValueError('Runtime receipt cannot promote a version outside authoritative approvals')
        accepted[version]['runtime'] = proof
        accepted[version]['source_equivalence_to_approved_image'] = proof.get('source_equivalence', 'NOT_SUPPLIED')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--state', type=Path, required=True)
    ap.add_argument('--desk', type=Path)
    ap.add_argument('--resource-audit', type=Path)
    ap.add_argument('--runtime-audit', type=Path)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    state = json.loads(args.state.read_text(encoding='utf-8-sig'))
    report = inventory(state, args.desk)
    validate_complete(state, report)
    if args.resource_audit:
        attach_resource_receipt(report, json.loads(args.resource_audit.read_text(encoding='utf-8')))
    if args.runtime_audit:
        attach_runtime_receipt(report, json.loads(args.runtime_audit.read_text(encoding='utf-8')))
    args.out.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k: report[k] for k in ['families_total', 'accepted_total', 'source_gate_gaps']}))


if __name__ == '__main__':
    main()
