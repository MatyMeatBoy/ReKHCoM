"""Import delivered preliminary pieces, not every experimental test screenshot.

One canonical image per scene; shared events remain listed as evidence views.
Only explicit known approval records may supply an imported approval status.
"""
import argparse
import hashlib
import json
from pathlib import Path
from server import ROOT, Store, now


def sync(data, ready, arenas):
    store=Store(data)
    catalog=[]
    def register(key,path,title,version,status='pending',evidence='',views=None):
        if not path.is_file():
            catalog.append(dict(key=key,missing=str(path)))
            return
        existing=[a for a in store.all() if a.get('revision_info',{}).get('catalog_key')==key]
        # Preserve the hand-delivered versions and real comments already in the desk.
        if key=='scene-121':existing=[a for a in store.all() if a['title'].startswith('Coliseo 121')]
        if key=='scene-122':existing=[a for a in store.all() if a['title'].startswith('Coliseo 122')]
        raw=path.read_bytes();sha=hashlib.sha256(raw).hexdigest()
        if existing and (key in ['scene-121','scene-122'] or any(a['source_sha256']==sha for a in existing)):
            catalog.append(dict(key=key,id=existing[-1]['id'],preserved=True))
            return
        item=store.add(raw,title,version,'Entrega catalogada desde '+str(path.resolve())+'. '+evidence,
            status=status,parent_id=existing[-1]['id'] if existing else None,
            revision_info=dict(catalog_key=key,source_path=str(path.resolve()),source_sha256=sha,
                evidence_views=views or [],approval_record=evidence if status!='pending' else None))
        catalog.append(dict(key=key,id=item['id'],status=item['status'],source=str(path.resolve())))
    for folder in sorted(ready.glob('escena-*')):
        if not folder.is_dir():continue
        scene=int(folder.name.split('-')[-1])
        image=folder/'3440x1440-300-despues.png'
        if not image.exists():
            choices=sorted(folder.glob('*-despues.png'));image=choices[0] if choices else image
        manifest=folder/'manifest.json'
        version='Preliminar V1 · entrega '+(hashlib.sha256(manifest.read_bytes()).hexdigest()[:10] if manifest.exists() else 'disponible')
        views=[str(p.resolve()) for p in ready.glob('evento-*-escena-'+str(scene).zfill(3)) if p.is_dir()]
        register('scene-'+str(scene),image,'Escena '+str(scene).zfill(3),version,views=views)
    names={'darkside':'Darkside · arena y contexto','jafar':'Jafar · sala de lava','maleficent':'Maléfica · arena y contexto',
           'marluxia':'Marluxia · arena','oogie':'Oogie Boogie · arena','trickmaster':'Trickmaster · arena','ursula':'Úrsula · arena'}
    for key,title in names.items():
        register('arena-'+key,arenas/(key+'.png'),title,'Preliminar V2 · entrega disponible',
                 status='approved' if key=='jafar' else 'pending',
                 evidence='LOGICA_APROBADOS.md: Jafar aprobado 24/10; captura de la entrega V2.' if key=='jafar' else '')
    evidence=ROOT/'build/codex-evidence'
    for scene in [4,57,61,74]:
        register('scene-'+str(scene),evidence/f'accepted-scenes-integrated/eu-{scene:03}-32x9-0/after/out_00300.png',
                 f'Escena {scene:03} · integrada','Versión aprobada · evidencia de integración','approved',
                 'LOGICA_APROBADOS.md: escenas 004/057/061/074 aprobadas; evidencia de pruebas de la receta integrada.')
    for scene in [107,109]:
        register('scene-'+str(scene),evidence/f'agrabah-{scene}/eu-{scene}-3440x1440/after/out_00300.png',
                 f'Agrabah {scene}'+(' / 108' if scene==107 else ''),'Versión con honores · 21/10','honors',
                 f'LOGICA_APROBADOS.md y AGRABAH_{scene}_REVISION.md: receta aprobada con honores el 21/10; captura de la entrega comprobada.')
    register('scene-8',evidence/'guard-patio-preview/32x9/GUARD_ARMOR/out_00380.png','Guard Armor · patio / escena 008',
             'Versión con honores · 20/10','honors','LOGICA_APROBADOS.md: patio de Guard Armor/escena008 aprobado con honores el 20/10.')
    report=dict(at=now(),entries=catalog,assets=len(store.all()),families=len(store.approvals()))
    (Path(data)/'catalogo.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--data',type=Path,default=ROOT/'build/art-review')
    p.add_argument('--ready',type=Path,default=ROOT/'build/preliminar-v1/ready')
    p.add_argument('--arenas',type=Path,required=True,help='Directory containing the delivered V2 after sheets')
    a=p.parse_args();report=sync(a.data,a.ready,a.arenas)
    print(json.dumps(dict(assets=report['assets'],pieces=report['families'],missing=[e for e in report['entries'] if 'missing' in e]),ensure_ascii=False,indent=2))
