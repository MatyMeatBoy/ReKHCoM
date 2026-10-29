"""Inventory every indexed story scene without mutating Atlas or loading a ROM.

Shared artwork is grouped, but events/camera paths remain distinct. Catalog
links are evidence to review, never approval or runtime coverage.
"""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re


def uncomment(text):
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)


def regional_source(text, region):
    """Select the repository's regional conditionals, without expanding includes."""
    macros, stack, active, output = {'VERSION_'+region.upper():1, 'PLATFORM_PC':1}, [], True, []
    def condition(expr):
        expr=re.sub(r'\bdefined\s*\(\s*(\w+)\s*\)',lambda m:str(int(m[1] in macros)),expr)
        expr=re.sub(r'\bdefined\s+(\w+)',lambda m:str(int(m[1] in macros)),expr)
        expr=expr.replace('&&',' and ').replace('||',' or ')
        expr=re.sub(r'!(?!=)',' not ',expr)
        def word(m):
            name=m[0]
            if name in ('and','or','not'):return name
            value=macros.get(name,0)
            if value is None:raise ValueError('unresolved conditional macro: '+name)
            return str(value)
        expr=re.sub(r'\b[A-Za-z_]\w*\b',word,expr)
        def run(node):
            if isinstance(node,ast.Expression):return run(node.body)
            if isinstance(node,ast.Constant) and isinstance(node.value,(int,bool)):return bool(node.value)
            if isinstance(node,ast.UnaryOp) and isinstance(node.op,ast.Not):return not run(node.operand)
            if isinstance(node,ast.BoolOp):
                values=[run(n) for n in node.values]
                return all(values) if isinstance(node.op,ast.And) else any(values)
            raise ValueError('unsupported regional conditional: '+expr)
        return run(ast.parse(expr.strip(),mode='eval'))
    for line in uncomment(text).splitlines():
        match=re.match(r'\s*#\s*(\w+)\s*(.*)',line)
        if not match:
            if active:output.append(line)
            continue
        command,arg=match.groups()
        if command in ('if','ifdef','ifndef'):
            met=condition(arg) if command=='if' else (arg.strip() in macros)==(command=='ifdef')
            stack.append([active,met]);active=active and met
        elif command=='elif':
            if not stack:raise ValueError('unmatched #elif')
            parent,taken=stack[-1];met=not taken and condition(arg)
            stack[-1][1]=taken or met;active=parent and met
        elif command=='else':
            if not stack:raise ValueError('unmatched #else')
            parent,taken=stack[-1];stack[-1][1]=True;active=parent and not taken
        elif command=='endif':
            if not stack:raise ValueError('unmatched #endif')
            active=stack.pop()[0]
        elif active and command=='define':
            parts=arg.split(None,1)
            try:value=int(parts[1],0) if len(parts)>1 else 1
            except ValueError:value=None
            macros[parts[0]]=value
        elif active and command=='undef':macros.pop(arg.strip(),None)
    if stack:raise ValueError('unterminated regional conditional')
    return '\n'.join(output)


def fields(text):
    """Split C initializer fields, retaining nested callbacks/macros intact."""
    result, start, depth, quote, escaped = [], 0, 0, None, False
    for i, ch in enumerate(text):
        if quote:
            if escaped: escaped = False
            elif ch == '\\': escaped = True
            elif ch == quote: quote = None
        elif ch in "\"'": quote = ch
        elif ch in '({[': depth += 1
        elif ch in ')}]': depth -= 1
        elif ch == ',' and depth == 0:
            if text[start:i].strip(): result.append(text[start:i].strip())
            start = i + 1
    if text[start:].strip(): result.append(text[start:].strip())
    return result


def initializer(text, name):
    match = re.search(r'\b' + re.escape(name) + r'(?:\s*\[[^]]*\])?\s*=\s*\{', text)
    if not match: raise ValueError('missing initializer: ' + name)
    start, depth = match.end(), 1
    for i in range(start, len(text)):
        if text[i] == '{': depth += 1
        elif text[i] == '}':
            depth -= 1
            if not depth: return text[start:i]
    raise ValueError('unterminated initializer: ' + name)


def camera_segments(rows, event_end):
    """Mirror view_1's u16 deadlines and progression to the next camera target."""
    result, start = [], 0
    for row in rows:
        if start >= event_end:break
        if len(row) != 8: raise ValueError('camera row must have eight fields')
        try: deadline, duration = int(row[0], 0) & 0xFFFF, int(row[6], 0)
        except ValueError as e: raise ValueError('unresolved camera deadline/duration') from e
        end = 'CAMERA_KEYFRAME_FLAG_END' in row[5]
        until = event_end if end else min(deadline, event_end)
        if until < start: raise ValueError('non-monotone camera deadline')
        if start < event_end:
            settled = min(start + duration, max(start, until - 1))
            samples = sorted({f for f in [max(0,start-1), start, start+1,
                start+duration//2, settled, until-1] if 0 <= f < event_end})
            result.append(dict(start=start, until=until, settled_at=settled,
                               deadline=deadline, end_sentinel=end, target=row[1:5],
                               flags=row[5], callback=row[7], capture_frames=samples))
        if end: break
        start = deadline
    return result


def catalog_links(event, assets):
    result, direct, adjacency = [], {}, {}
    ids={a.get('id') for a in assets}
    for item in assets:
        identifier,parent=item.get('id'),item.get('parent_id')
        if parent in ids:
            adjacency.setdefault(identifier,set()).add(parent)
            adjacency.setdefault(parent,set()).add(identifier)
        key = item.get('revision_info', {}).get('catalog_key', '')
        title = item.get('title', '')
        keyed = re.fullmatch(r'scene-(\d+)', key)
        labelled = re.search(r'\b(?:escena|scene)\s*[-_:]?\s*(\d{1,4})\b', title, re.I)
        reasons = []
        if keyed and int(keyed[1]) == event: reasons.append('catalog_key')
        if labelled and int(labelled[1]) == event: reasons.append('scene_label')
        if reasons:direct[identifier]=reasons
    linked, pending=set(direct),list(direct)
    while pending:
        for identifier in adjacency.get(pending.pop(),()):
            if identifier not in linked:linked.add(identifier);pending.append(identifier)
    for item in assets:
        if item.get('id') in linked:
            result.append(dict(id=item.get('id'), title=item.get('title',''), historical_status=item.get('status'),
                               catalog_key=item.get('revision_info',{}).get('catalog_key',''),
                               matched_by=direct.get(item.get('id'),['family_lineage'])))
    return result


def render_contexts(source, region='eu'):
    source=Path(source)
    result=dict(field_worlds=[],battle_entries=[],battle_stages=[],runtime_coverage='UNVERIFIED',missing_sources=[])
    world=source/'include/menu/world_types.h'
    battle=source/'src/debug/mode_chkbtl.c'
    if world.exists():
        text=regional_source(world.read_text(encoding='utf-8'),region)
        enum=re.search(r'enum\s+WorldId\s*\{(.*?)\};',text,re.S)
        if not enum:raise ValueError('WorldId enumeration missing')
        value=-1
        for item in fields(enum[1]):
            parts=item.split('=',1);value=int(parts[1],0) if len(parts)>1 else value+1
            symbol=parts[0].strip()
            if symbol!='WORLD_NONE':result['field_worlds'].append(dict(world=value,symbol=symbol))
    else:result['missing_sources'].append('include/menu/world_types.h')
    if battle.exists():
        text=regional_source(battle.read_text(encoding='utf-8'),region)
        for i,item in enumerate(fields(initializer(text,'sChkBtlEntries'))):
            row=fields(item.strip()[1:-1])
            if len(row)<3:raise ValueError('battle entry lacks world/kind/battle')
            result['battle_entries'].append(dict(index=i,world=row[0],kind=row[1],battle=row[2],fields=row))
        result['battle_stages']=[fields(row.strip()[1:-1])[0] for row in fields(initializer(text,'sChkBtlWorlds'))]
    else:result['missing_sources'].append('src/debug/mode_chkbtl.c')
    return result


def inventory_region(source, assets, region):
    source = Path(source)
    header = regional_source((source/'include/evt/event_ids.h').read_text(encoding='utf-8'),region)
    enum=re.search(r'enum\s+EventId\s*\{(.*?)\};',header,re.S)
    if not enum:raise ValueError('EventId enumeration missing')
    names,current={},-1
    for item in fields(enum[1]):
        parts=item.split('=',1);current=int(parts[1],0) if len(parts)>1 else current+1
        match=re.fullmatch(r'EVENT_(\d{3,})_(\w+)',parts[0].strip())
        if match:names[int(match[1])]=(match[2],current)
    index = regional_source((source/'src/evt/event_index_data.c').read_text(encoding='utf-8'),region)
    pointers = fields(initializer(index, 'gEventBackgroundDefs'))
    scripts = {int(m[1]):p for p in (source/'src/evt/events').glob('*.inc')
               if (m:=re.match(r'(\d+)_',p.name))}
    events, groups, issues = [], {}, []
    for number, (name,runtime) in sorted(names.items()):
        row = dict(event=number, runtime_event=runtime, region=region,name=name, background=None, camera_variants=[],
                   atlas_links=catalog_links(number, assets), runtime_coverage='UNVERIFIED')
        def issue(reason): issues.append(dict(event=number,region=region,reason=reason))
        if runtime >= len(pointers): issue('background index missing')
        else:
            token = pointers[runtime]
            if token == 'NULL': row['background_kind'] = 'no_story_background'
            elif re.fullmatch(r'&gEventBg\w+', token): row['background'] = token[1:]
            else: issue('unresolved background pointer: ' + token)
        script = scripts.get(number)
        if not script: issue('script missing')
        else:
            text = regional_source(script.read_text(encoding='utf-8'),region)
            row['script'] = script.relative_to(source).as_posix()
            row['script_sha256'] = hashlib.sha256(script.read_bytes()).hexdigest()
            row['has_belle'] = 'PORTRAIT_BELLE' in text
            try:
                sequence = fields(initializer(text, 'gEvent'+f'{number:03}'))
                row['duration'] = int(sequence[6], 0)
                cameras = re.findall(r'\b(sEvent\d+Camera)\s*\[', text)
                if not cameras: issue('camera initializer missing')
                for camera in dict.fromkeys(cameras):
                    raw = initializer(text, camera)
                    entries = [fields(entry.strip()[1:-1]) for entry in fields(raw) if entry.strip().startswith('{')]
                    row['camera_variants'].append(dict(name=camera, rows=entries,
                        segments=camera_segments(entries,row['duration'])))
                row['background_effect_arrays'] = []
                for effect in dict.fromkeys(re.findall(r'\b(sEvent\d+BgEffects)\s*\[',text)):
                    row['background_effect_arrays'].append(dict(name=effect, rows=fields(initializer(text,effect))))
            except (ValueError, IndexError) as e: issue(str(e))
        events.append(row)
        if row['background']:
            group = groups.setdefault(row['background'],dict(events=[], linked_assets=[],runtime_coverage='UNVERIFIED'))
            group['events'].append(number)
            known = {a['id'] for a in group['linked_assets']}
            group['linked_assets'].extend(a for a in row['atlas_links'] if a['id'] not in known)
    return dict(schema='rekhcom.scenario-inventory/1', events=events, backgrounds=groups,
                background_table_count=len(pointers), issues=issues,
                scope='Source/catalog discovery; no runtime, acceptance or import completeness inferred')


def inventory(source, assets, regions=('eu','us','jp')):
    events,groups,issues,counts,tables={},{},[],{},{}
    for region in regions:
        report=inventory_region(source,assets,region)
        counts[region]=len(report['events']);tables[region]=report['background_table_count']
        issues.extend(report['issues'])
        for row in report['events']:
            combined=events.setdefault(row['event'],{**row,'regional_variants':[]})
            combined['regional_variants'].append(row)
            bg=row['background']
            if bg:
                group=groups.setdefault(bg,dict(events=[],linked_assets=[],runtime_coverage='UNVERIFIED'))
                if row['event'] not in group['events']:group['events'].append(row['event'])
                for asset in row['atlas_links']:
                    if not any(a['id']==asset['id'] for a in group['linked_assets']):group['linked_assets'].append(asset)
    return dict(schema='rekhcom.scenario-inventory/1',events=list(events.values()),backgrounds=groups,
                region_counts=counts,regional_background_table_count=tables,issues=issues,
                render_contexts={region:render_contexts(source,region) for region in regions},
                scope='Source/catalog discovery across regional conditionals; base fields use the first available region and regional_variants retains all. No runtime/acceptance/import completeness inferred')


def markdown(report):
    lines = ['# Scenario coverage inventory', '', report['scope'], '',
             f"{len(report['events'])} events; {len(report['backgrounds'])} shared backgrounds; {len(report['issues'])} unresolved source items.", '',
             '| Background | Events | Deterministic catalog links | Runtime coverage |',
             '| --- | --- | --- | --- |']
    for name, row in report['backgrounds'].items():
        lines.append('| '+name+' | '+', '.join(map(str,row['events']))+' | '+
                     str(len(row['linked_assets']))+' | UNVERIFIED |')
    lines += ['', 'Catalog links preserve historical records; all camera/zoom/entry/exit variants still need actual review.',
              'No deterministic link means manual matching is needed, not proof that artwork does not exist.',
              'Null-background events remain listed and must be crossed with battle/other rendering paths.']
    lines += ['', '## Regional source variants', '']
    for region,count in report['region_counts'].items():
        context=report['render_contexts'][region]
        lines.append(f"- {region}: {count} events; {len(context['field_worlds'])} field-world definitions; "
                     f"{len(context['battle_entries'])} debug battle entries / {len(context['battle_stages'])} stage selectors. Runtime UNVERIFIED.")
    return '\n'.join(lines)+'\n'


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[3])
    p.add_argument('--atlas-state',type=Path,required=True,help='Read-only JSON export containing assets; desk is never opened')
    p.add_argument('--out',type=Path,required=True,help='Fresh output directory')
    p.add_argument('--region',action='append',choices=['eu','us','jp'],help='Default: all three regional source variants')
    a=p.parse_args()
    if a.out.exists():p.error('output already exists; preserve previous evidence')
    state=json.loads(a.atlas_state.read_text(encoding='utf-8-sig'))
    if not isinstance(state,dict) or not isinstance(state.get('assets'),list):p.error('Atlas export must contain an assets array')
    report=inventory(a.source,state['assets'],a.region or ('eu','us','jp'))
    report['source_inputs_sha256']={name:hashlib.sha256((a.source/name).read_bytes()).hexdigest()
        for name in ['include/evt/event_ids.h','src/evt/event_index_data.c',
                     'include/menu/world_types.h','src/debug/mode_chkbtl.c'] if (a.source/name).exists()}
    report['generator_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    if 'approvals' in state:report['atlas_approval_registry']=state['approvals']
    else:
        # Reuse the desk's exact policy with an in-memory adapter. Do not
        # instantiate Store: its constructor creates folders and opens a DB.
        from types import SimpleNamespace
        if __package__:from .server import Store
        else:from server import Store
        report['atlas_approval_registry']=Store.approvals(SimpleNamespace(all=lambda:state['assets']))
    report['atlas_export_sha256']=hashlib.sha256(a.atlas_state.read_bytes()).hexdigest()
    a.out.mkdir(parents=True)
    (a.out/'inventory.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    (a.out/'inventory.md').write_text(markdown(report),encoding='utf-8')
    print(json.dumps(dict(events=len(report['events']), backgrounds=len(report['backgrounds']),issues=len(report['issues']))))
    return bool(report['issues'])


if __name__=='__main__':raise SystemExit(main())
