"""Exercise EXP notices at every zoom, preserving their authored placement.
Requires the local EU ROM; runs in isolated directories and never awards EXP.
"""
import argparse, json, os, re, shutil, subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser()
p.add_argument('--build', type=Path, default=ROOT/'build/rel-eu')
p.add_argument('--rom', type=Path, default=ROOT/'roms/B8CP.gba')
args = p.parse_args()
out = ROOT/'build/v181-review/exp'; out.mkdir(parents=True, exist_ok=True)
cases = [(W,H,z,0,1) for W,H in [(1920,1080),(5120,1440)]
         for z in range(2*(H//160),2*max((W+2559)//2560,(H+1439)//1440)-1,-2)]
cases += [(W,H,8,hd,hud) for W,H in [(1920,1080),(5120,1440)] for hd,hud in [(1,0),(1,1),(1,2)]]
def run(case):
 W,H,z,hd,hud=case; name=f'{W}x{H}-z{z}-hd{hd}-hud{hud}'; d=out/name; d.mkdir(exist_ok=True)
 for f in ['khcom.exe','SDL3.dll']: shutil.copy2(args.build/f,d/f)
 shutil.copytree(ROOT/'port/assets/fonts',d/'assets/fonts',dirs_exist_ok=True)
 (d/'kh_com.ini').write_text(f'config_version=11\nview_mode=4\nzoom={z}\ninteger_zoom=0\nhd_text={hd}\nhud_size={hud}\n')
 env={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))}
 env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_FAST='1',KHCOM_WINDOW=f'{W}x{H}',
  KHCOM_BATTLE='0:1:0',KHCOM_KEYS='60-63:A',KHCOM_MODE='140:textdigits:12345,260:textlevel:1',
  KHCOM_OAMDUMP='160',KHCOM_OUTPUT_SHOT='160,220,280' if z==8 else '',KHCOM_QUIT='282')
 result=subprocess.run([str((d/'khcom.exe').resolve()),'--rom',str(args.rom.resolve()),'--language','es'],cwd=d,env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=180)
 log=(d/'kh_com.log').read_text(errors='replace');assert result.returncode==0 and not any(t in log.lower() for t in ['crash','null read','division by zero']),name
 assert 'harness: textdigits 12345' in log and 'harness: textlevel 1' in log,name
 # 25/10 (user): the notice follows the player's HP gauge to the screen's top-left corner (PortHudBegin), so its
 # pieces keep their authored spacing (label, then digits every 8) from wherever the corner is.
 coords=[(int(x),int(y)) for x,y in re.findall(r'oam\s+\d+ ext\(\s*(-?\d+),\s*(-?\d+)\)',log)]
 exp=sorted(set(x for x,y in coords if y==40))
 assert len(exp)>=6 and all(b-a in (8,16,24,32,40,48) for a,b in zip(exp,exp[1:])),(name,exp)
 if z==8:
  from PIL import Image
  for f in [160,220,280]:
   file=d/f'out_{f:05d}.bmp';assert file.is_file()
   im=Image.open(file);im.resize((1280,round(1280*H/W))).save(d/f'notice_{f}.png');im.close();file.unlink()
 row=dict(case=name,exp_x=exp,passed=True);print(json.dumps(row),flush=True);return row
with ThreadPoolExecutor(max_workers=2) as pool: results=list(pool.map(run,cases))
# With identical gameplay/zoom, the notice must be pixel-identical with HD
# text off/on. Compare all three original label/number states at x4.
from PIL import Image
for W,H in [(1920,1080),(5120,1440)]:
 for f in [160,220,280]:
  a=Image.open(out/f'{W}x{H}-z8-hd0-hud1/notice_{f}.png')
  b=Image.open(out/f'{W}x{H}-z8-hd1-hud1/notice_{f}.png')
  ratio=1280/W;ox=(W-240*4)/2;oy=(H-160*4)/2+40*4
  box=tuple(round(v*ratio) for v in [ox,oy,ox+88*4,oy+16*4])
  assert a.crop(box).tobytes()==b.crop(box).tobytes(),('HD changed EXP pixels',W,f)
(out/'report.json').write_text(json.dumps(dict(configurations=results,native_exp_hd_equivalence=True),indent=2))
print(f'PASS: {len(results)} configurations, 3 notice states each')
