"""Generate portable clean GBA posters with the actual engine, outside the player's data.

One hidden process at a time, shared executable/ROM, no copied decoder/assets/ROM.
Only this explicit preparation tool runs scenes; opening the gallery never does.
"""
import argparse,hashlib,json,os,re,subprocess,tempfile,time
from pathlib import Path
from PIL import Image

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--exe',required=True,type=Path);p.add_argument('--rom',required=True,type=Path)
 p.add_argument('--out',required=True,type=Path);p.add_argument('--work',required=True,type=Path)
 p.add_argument('--events',default='0,10,56,73,149,155,193,195')
 p.add_argument('--movie-frame',type=int,default=None,help='Explicit movie frame; default 90, or 450 for Riku ending')
 p.add_argument('--region',choices=('eu','us','jp'),default='eu')
 a=p.parse_args();a.exe=a.exe.resolve();a.rom=a.rom.resolve();a.out=a.out.resolve();a.work.mkdir(parents=True,exist_ok=True);a.out.mkdir(parents=True,exist_ok=True)
 events=([int(i) for i in re.findall(r'\{ EVENT_(\d+)',(Path(__file__).resolve().parents[2]/'port/src/port_scene_names.inc').read_text(encoding='utf-8'))]+list(range(513,518))) if a.events=='all' else [int(i) for i in a.events.split(',')]
 results=[]
 fingerprints=dict(exe_sha256=hashlib.sha256(a.exe.read_bytes()).hexdigest(),rom_sha256=hashlib.sha256(a.rom.read_bytes()).hexdigest())
 for event in events:
  runtime=event-2 if a.region=='eu' and 129<=event<512 else event
  frame=a.movie_frame if a.movie_frame is not None else (450 if event==517 else 90)
  dest=a.out/f'scene-{event:03d}.png'
  env={k:v for k,v in os.environ.items() if not k.startswith('KHCOM_')}
  env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_WINDOW='360x240',KHCOM_CONFIG='vsync=0;graphics_mode=3;view_mode=0',KHCOM_HIDDEN='1',KHCOM_BACKGROUND_TEST='1',KHCOM_PPU_THREADS='1',KHCOM_QUIT='2500',KHCOM_FAST='1')
  started=time.monotonic()
  with tempfile.TemporaryDirectory(prefix=f'poster-{event}-',dir=a.work) as private:
   log=a.work/f'poster-{event:03d}.log'
   with log.open('w',encoding='utf-8') as stream:
    run=subprocess.run([str(a.exe),'--rom',str(a.rom),'--data-dir',private,'--scene-poster',str(runtime),'--poster-frame',str(frame),'--poster-output',str(dest)],env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=30)
   assert run.returncode==0,(event,run.returncode,log)
   assert dest.exists(),f'Engine failed to emit clean poster {event}: {log}'
   with Image.open(dest) as im:
    assert im.size==(360,240),(event,im.size)
    pixels=im.tobytes();im.save(dest,optimize=True)
   with Image.open(dest) as im:assert im.tobytes()==pixels, 'Lossless poster compression changed pixels'
   text=log.read_text(encoding='utf-8',errors='replace')
   assert 'scene poster: saved' in text and 'bios:' not in text,(event,log)
   results.append(dict(event=event,runtime=runtime,region=a.region,seconds=round(time.monotonic()-started,3),png=str(dest),sha256=hashlib.sha256(dest.read_bytes()).hexdigest(),log=str(log)))
   (a.work/'poster-manifest.json').write_text(json.dumps(dict(**fingerprints,posters=results),indent=2),encoding='utf-8')
   print('PASS poster',event,results[-1]['seconds'],flush=True)
 manifest=dict(exe_sha256=hashlib.sha256(a.exe.read_bytes()).hexdigest(),rom_sha256=hashlib.sha256(a.rom.read_bytes()).hexdigest(),posters=results)
 (a.work/'poster-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
if __name__=='__main__':main()
