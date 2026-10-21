"""Serial snapshot fixtures and review gallery for the complete Preliminar V1.
Only one game process runs at a time. All comparisons replay the same state.
"""
import argparse,json,os,subprocess,shutil,html
from pathlib import Path
from PIL import Image,ImageChops,ImageDraw
from preliminar_v1_recipes import RECIPES,READY
from test_events import event_lists
from audit_battle_scenery import cases as battle_cases
ROOT=Path(__file__).resolve().parents[2]
WINDOWS=[(1920,1080),(2560,1080),(3440,1440),(3840,1080)]
BOSSES=['DARKSIDE','OOGIE_BOOGIE','JAFAR','DRAGON_MALEFICENT','URSULA','TRICKMASTER','MARLUXIA_2']
def fixtures():
 events={int(n.split('_')[1]):(l,i) for l,i,n in event_lists() if n.split('_')[1].isdigit()}
 result=[]
 for n in sorted(set(RECIPES)|{4,6,14,57,61,74,107,109,130}):
  l,i=events[n];result.append(dict(name=f'escena-{n:03}',scene=n,start=('KHCOM_EVENT',f'{l}:{i}')))
 # gEventBg053 is reused in Riku's Destiny Islands event. Keep the same
 # recipe, but exercise its different character placement and camera start.
 result.append(dict(name='evento-177-escena-053',scene=53,start=('KHCOM_EVENT','1:28')))
 # gEventBg054 is also reused for Riku's return to Destiny Islands.
 result.append(dict(name='evento-178-escena-054',scene=54,start=('KHCOM_EVENT','1:29')))
 # gEventBg087 is reused in Halloween Town's follow-up E0 event.
 list_id,index=events[88]
 result.append(dict(name='evento-088-escena-087',scene=87,
                    start=('KHCOM_EVENT',f'{list_id}:{index}')))
 # Atlantica's Ursula chamber art is also used by the follow-up event 106.
 list_id,index=events[106]
 result.append(dict(name='evento-106-escena-104',scene=104,
                    start=('KHCOM_EVENT',f'{list_id}:{index}')))
 # gEventBg081 is shared across the Monstro E3 fight's success/failure/retry states.
 for event_id in (82,83,84,85):
  list_id,index=events[event_id]
  result.append(dict(name=f'evento-{event_id:03}-escena-081',scene=81,
                     start=('KHCOM_EVENT',f'{list_id}:{index}')))
 # gEventBg055 spans both Sora's transition and three Riku event states.
 for event_id in (56,179,181,183):
  list_id,index=events[event_id]
  result.append(dict(name=f'evento-{event_id:03}-escena-055',scene=55,
                     start=('KHCOM_EVENT',f'{list_id}:{index}')))
 for c in battle_cases():
  if c['arena'] in BOSSES:result.append(dict(name=c['arena'],start=('KHCOM_BATTLE',f"{c['battle']}:{c['stage']}:0")))
 return result

def run(exe,d,env,rom,timeout=120):
 subprocess.run([str(exe),'--language','es','--rom',str(rom)],cwd=d,env=env,timeout=timeout,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 log=(d/'kh_com.log').read_text(errors='replace')
 if 'CRASH' in log:raise RuntimeError('game crash: '+str(d))

def capture(c,a,out):
 exe=(ROOT/a.build/'khcom.exe').resolve();rom=ROOT/'roms'/dict(eu='B8CP.gba',us='B8CE.gba',jp='B8CJ.gba')[a.region]
 d=out/c['name'];d.mkdir(parents=True,exist_ok=True)
 for name in ('khcom.exe','SDL3.dll'):shutil.copy2(ROOT/a.build/name,d/name)
 exe=d/'khcom.exe'
 env=dict(os.environ,SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_FAST='1')
 for key in list(env):
  if key.startswith('KHCOM_'):env.pop(key)
 env['KHCOM_FAST']='1'
 env['KHCOM_AGRABAH_WIDE']='1';env['KHCOM_AGRABAH109_WIDE']='1'
 frames=[300] if a.quick else [100,300,1000]
 subject=a.subject if a.subject is not None else c.get('scene') if c.get('scene') in READY else None
 (d/'kh_com.ini').write_text(f'config_version=11\nview_mode={0 if a.original else 4}\nzoom=6\nhd_text=0\nvsync=0\nfullscreen=0\n')
 signature=str(exe.stat().st_mtime_ns)
 if not (d/'build.txt').exists() or (d/'build.txt').read_text()!=signature or not all((d/f'frame{f}.khst').exists() for f in frames):
  boot=dict(env,KHCOM_WINDOW='720x480',KHCOM_QUIT=str(max(frames)+4),KHCOM_KEYS='60-63:A,360-363:A,480-483:A,600-603:A,720-723:A',KHCOM_SAVESTATE=','.join(f'{f}:frame{f}.khst' for f in frames));boot[c['start'][0]]=c['start'][1]
  run(exe,d,boot,rom);(d/'build.txt').write_text(signature)
 results=[]
 for width,height in ([WINDOWS[2]] if a.quick else WINDOWS):
  for f in frames:
   pair=[]
   for enabled in [False,True]:
    replay=dict(env,KHCOM_WINDOW=f'{width}x{height}',KHCOM_LOADSTATE=f'frame{f}.khst',KHCOM_QUIT='14',KHCOM_SHOTS='12',KHCOM_OUTPUT_SHOT='12')
    if enabled or subject is not None:replay['KHCOM_PRELIMINAR_V1']='1'
    if not enabled and subject is not None:replay['KHCOM_PRELIMINAR_SKIP']=str(subject)
    run(exe,d,replay,rom)
    with Image.open(d/'shot_00012.bmp') as im:
     pair.append(im.convert('RGB'))
     if enabled:im.resize((im.width*3,im.height*3),Image.Resampling.NEAREST).save(d/f'{width}x{height}-{f}-limpio.png')
    with Image.open(d/'out_00012.bmp') as im:im.save(d/(f'{width}x{height}-{f}-'+('despues.png' if enabled else 'antes.png')))
   w,h=pair[0].size;box=((w-240)//2,(h-160)//2,(w-240)//2+240,(h-160)//2+160)
   native=ImageChops.difference(pair[0].crop(box),pair[1].crop(box)).getbbox();changed=ImageChops.difference(*pair).getbbox()
   results.append(dict(window=[width,height],frame=f,native_diff=native,changed=changed is not None,passed=native is None))
  print(c['name'],width,'native',sum(not r['passed'] for r in results),flush=True)
 for p in d.glob('*.bmp'):p.unlink()
 expected=c.get('scene')==subject and subject in READY and not a.original
 changed=any(r['changed'] for r in results)
 row=dict(**c,checks=results,passed=all(r['passed'] for r in results) and (changed if expected else not changed),visual_review='pending');(d/'report.json').write_text(json.dumps(row,indent=2));return row

def gallery(out,rows):
 body=['<!doctype html><meta charset="utf-8"><title>Preliminar V1</title><style>body{background:#171a22;color:#eee;font:16px system-ui;margin:24px}img{width:100%;image-rendering:pixelated}details{margin:24px 0}a{color:#9cf}</style><h1>Preliminar V1</h1><p>Implementaciones para comentar. Las capturas antes/después parten de la misma instantánea. La revisión visual del usuario queda pendiente.</p>']
 thumbs=[]
 for r in rows:
  body.append('<details><summary>'+html.escape(r['name'])+'</summary>')
  for w,h in WINDOWS:
   for f in [100,300,1000]:
    for side in ['antes','despues']:
     p=out/r['name']/f'{w}x{h}-{f}-{side}.png'
     if p.exists():body.append(f'<p>{w}×{h} · {f} · {side}</p><a href="{p.relative_to(out).as_posix()}"><img loading="lazy" src="{p.relative_to(out).as_posix()}"></a>')
  body.append('</details>')
  p=out/r['name']/'3440x1440-300-despues.png'
  if p.exists():
   with Image.open(p) as im:
    im=im.convert('RGB');im.thumbnail((860,360),Image.Resampling.NEAREST);thumbs.append((r['name'],im.copy()))
 (out/'index.html').write_text('\n'.join(body),encoding='utf-8')
 for start in range(0,len(thumbs),6):
  sheet=Image.new('RGB',(1720,1170),'#171a22');dr=ImageDraw.Draw(sheet)
  for i,(name,im) in enumerate(thumbs[start:start+6]):x=i%2*860;y=i//2*390;dr.text((x+8,y+5),name,fill='white');sheet.paste(im,(x,y+25))
  sheet.save(out/f'revision-{start//6+1}.png')

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--build',default='build/agrabah109-eu');p.add_argument('--out',default='build/preliminar-v1/eu');p.add_argument('--region',default='eu',choices=['eu','us','jp']);p.add_argument('--only');p.add_argument('--quick',action='store_true');p.add_argument('--original',action='store_true');p.add_argument('--subject',type=int);a=p.parse_args();out=ROOT/a.out;out.mkdir(parents=True,exist_ok=True)
 rows=[]
 for c in fixtures():
  if a.only and c['name'] not in a.only.split(','):continue
  try:rows.append(capture(c,a,out))
  except Exception as e:print(c['name'],repr(e),flush=True);rows.append(dict(**c,passed=False,error=str(e)))
  (out/'report.json').write_text(json.dumps(rows,indent=2))
 gallery(out,rows)
 raise SystemExit(0 if all(r['passed'] for r in rows) else 1)



