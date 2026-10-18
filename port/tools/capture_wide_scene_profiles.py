"""Compare frozen prior and revised executables at large internal resolutions.

Original ROM and private executables required; never touches installed saves.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import shutil
import subprocess
from PIL import Image, ImageChops, ImageDraw
from test_events import event_lists

ROOT = Path(__file__).resolve().parents[2]


def capture(event, size, args):
    w,h=size
    label=f'{event:03d}-{w}x{h}'
    entry=next((l,i) for l,i,name in event_lists() if name.startswith(f'EVENT_{event:03d}_'))
    images=[]
    for name,exe in [('before',Path(args.baseline)),('after',ROOT/'build/wide-eu/khcom.exe')]:
        d=ROOT/args.out/label/name
        d.mkdir(parents=True,exist_ok=False)
        shutil.copy2(exe,d/'khcom.exe')
        shutil.copy2(ROOT/'build/wide-eu/SDL3.dll',d/'SDL3.dll')
        (d/'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=4\nfullscreen=0\nhd_text=0\nvsync=0\n')
        env=dict(os.environ,SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_FAST='1',
                 KHCOM_WINDOW=f'{w*2}x{h*2}',KHCOM_EVENT=f'{entry[0]}:{entry[1]}',
                 KHCOM_KEYS='60-63:A',KHCOM_SHOTS='300',KHCOM_QUIT='302')
        env.pop('KHCOM_NO_SCENE_PROFILES',None)
        result=subprocess.run([str(d/'khcom.exe'),'--rom',str(ROOT/'roms/B8CP.gba')],cwd=d,env=env,capture_output=True,timeout=180)
        if result.returncode:
            raise RuntimeError(f'{label}: {name} exit {result.returncode}')
        im=Image.open(d/'shot_00300.bmp').convert('RGB')
        im.save(d/'native.png')
        images.append(im)
    before,after=images
    if before.size!=size or after.size!=size:
        raise RuntimeError(f'{label}: unexpected size {before.size}/{after.size}')
    box=((w-240)//2,(h-160)//2,(w+240)//2,(h+160)//2)
    native_equal=ImageChops.difference(before.crop(box),after.crop(box)).getbbox() is None
    bounds=ImageChops.difference(before,after).getbbox()
    pair=Image.new('RGB',(w*2+16,h+32),(24,27,38))
    draw=ImageDraw.Draw(pair)
    for x,title,im in [(0,'ANTES - perfil anterior',before),(w+16,'DESPUES - revision amplia',after)]:
        draw.text((x+4,6),title,fill='white');pair.paste(im,(x,28))
    pair.save(ROOT/args.out/label/'comparison.png')
    report=dict(event=event,view=size,native_gba_unchanged=native_equal,changed_bounds=bounds)
    print(json.dumps(report),flush=True)
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline',required=True)
    p.add_argument('--out',default='build/wide-runtime-review')
    p.add_argument('--events',nargs='+',type=int,default=[4,57,61,74])
    args=p.parse_args()
    out=(ROOT/args.out).resolve()
    if not out.is_relative_to(ROOT/'build'):
        p.error('Output must stay in build/')
    cases=[(e,s) for s in [(960,540),(1440,810)] for e in args.events]
    with ThreadPoolExecutor(max_workers=2) as pool:
        reports=list(pool.map(lambda c:capture(*c,args),cases))
    (out/'report.json').write_text(json.dumps(reports,indent=2))
    return 0 if all(r['native_gba_unchanged'] and (not r['changed_bounds'] if r['event']==4 else bool(r['changed_bounds'])) for r in reports) else 1


if __name__=='__main__':
    raise SystemExit(main())
