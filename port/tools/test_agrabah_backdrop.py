"""Private hidden before/after fixtures for the shared Agrabah 107/108 street.

Uses the same binary on both sides. No ROM or extracted assets are committed.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor
from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build/codex-evidence/agrabah-107'
FRAMES = (100, 300, 550, 800, 1000)

def capture(case):
    region, event, width, height, original = case
    label = f'{region}-{event}-{width}x{height}' + ('-original' if original else '')
    images = []
    for enabled in (False, True):
        d = OUT / label / ('after' if enabled else 'before')
        d.mkdir(parents=True, exist_ok=True)
        for name in ('khcom.exe', 'SDL3.dll'):
            shutil.copy2(ROOT / f'build/agrabah-{region}' / name, d / name)
        (d/'kh_com.ini').write_text(f'config_version=11\nview_mode={0 if original else 4}\nzoom=6\nhd_text=0\nvsync=0\nfullscreen=0\n')
        env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
                   KHCOM_EVENT=f'0:{event-1}', KHCOM_WINDOW=f'{width}x{height}',
                   KHCOM_KEYS='60-63:A,360-363:A,480-483:A,600-603:A,720-723:A',
                   KHCOM_QUIT='1002', KHCOM_SHOTS=','.join(map(str,FRAMES)),
                   KHCOM_OUTPUT_SHOT=','.join(map(str,FRAMES)))
        env.pop('KHCOM_AGRABAH_WIDE',None)
        env['KHCOM_AGRABAH_WIDE']='1' if enabled else '0'  # on by default since 22/10
        code = dict(eu='B8CP',us='B8CE',jp='B8CJ')[region]
        frames=[]
        # Replay precisely the same game state on each side. Text advance and
        # character animation can otherwise diverge between independent boots.
        for f in FRAMES:
            state = OUT / 'states' / f'{region}-{event}' / f'frame{f}.khst'
            shutil.copy2(state,d/'replay.khst')
            replay = dict(env, KHCOM_LOADSTATE='replay.khst', KHCOM_QUIT='14',
                          KHCOM_SHOTS='12', KHCOM_OUTPUT_SHOT='12')
            replay.pop('KHCOM_EVENT',None)
            replay.pop('KHCOM_KEYS',None)
            subprocess.run([str(d/'khcom.exe'),'--language','es','--rom',str(ROOT/f'roms/{code}.gba')],
                           cwd=d,env=replay,timeout=60,check=True,capture_output=True)
            log=(d/'kh_com.log').read_text(errors='replace')
            if ' -> 1' not in log or 'CRASH' in log: raise RuntimeError('state replay failed: '+label)
            with Image.open(d/'shot_00012.bmp') as im:
                frames.append(im.convert('RGB'))
                im.save(d/f'native_{f:05d}.png')
            with Image.open(d/'out_00012.bmp') as im: im.save(d/f'out_{f:05d}.png')
        images.append(frames)
    native_changes=changes=0
    differences=[]
    for f,before,after in zip(FRAMES,*images):
        w,h=before.size
        box=((w-240)//2,(h-160)//2,(w-240)//2+240,(h-160)//2+160)
        native_changes += ImageChops.difference(before.crop(box),after.crop(box)).getbbox() is not None
        changes += ImageChops.difference(before,after).getbbox() is not None
        differences.append(dict(state=f,native_bounds=ImageChops.difference(before.crop(box),after.crop(box)).getbbox()))
    should_change = event in (107,108) and not original
    result=dict(case=label,native_changed_frames=native_changes,changed_frames=changes,frames=differences,
                passed=native_changes==0 and (changes>0 if should_change else changes==0))
    print(json.dumps(result),flush=True)
    return result

def make_states(pair):
    region,event=pair
    d=OUT/'states'/f'{region}-{event}'
    d.mkdir(parents=True,exist_ok=True)
    if ((d/'khcom.exe').exists() and
        (d/'khcom.exe').read_bytes() == (ROOT/f'build/agrabah-{region}/khcom.exe').read_bytes() and
        all((d/f'frame{f}.khst').exists() for f in FRAMES)):
        return
    for name in ('khcom.exe','SDL3.dll'): shutil.copy2(ROOT/f'build/agrabah-{region}'/name,d/name)
    (d/'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=6\nhd_text=0\nvsync=0\n')
    code=dict(eu='B8CP',us='B8CE',jp='B8CJ')[region]
    env=dict(os.environ,SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_FAST='1',
             KHCOM_EVENT=f'0:{event-1}',KHCOM_WINDOW='720x480',KHCOM_QUIT='1004',
             KHCOM_KEYS='60-63:A,360-363:A,480-483:A,600-603:A,720-723:A',
             KHCOM_SAVESTATE=','.join(f'{f}:frame{f}.khst' for f in FRAMES))
    env['KHCOM_AGRABAH_WIDE']='0'  # states from the original picture
    subprocess.run([str(d/'khcom.exe'),'--language','es','--rom',str(ROOT/f'roms/{code}.gba')],
                   cwd=d,env=env,timeout=240,check=True,capture_output=True)
    if not all((d/f'frame{f}.khst').exists() for f in FRAMES): raise RuntimeError('missing states')
    print('States ready: '+str(pair),flush=True)

if __name__=='__main__':
    cases=[('eu',107,w,h,False) for w,h in [(1920,1080),(2560,1080),(3440,1440),(3840,1080)]]
    cases += [('eu',108,1920,1080,False)]
    cases += [(r,107,1920,1080,False) for r in ('us','jp')]
    cases += [(r,107,1920,1080,True) for r in ('eu','us','jp')]
    cases += [('eu',109,1920,1080,False)]
    with ThreadPoolExecutor(max_workers=1) as pool:
        list(pool.map(make_states, sorted(set((c[0],c[1]) for c in cases))))
        results=list(pool.map(capture,cases))
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'report.json').write_text(json.dumps(results,indent=2)+'\n')
    raise SystemExit(0 if all(r['passed'] for r in results) else 1)
