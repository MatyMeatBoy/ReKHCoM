"""Pixel regression for the real full-reload Jafar fixture (frame 1572).

A native fill must not cover the imported counter above it. The fixture's
interior is red, so its white '1' must survive independently of the HD border.
"""
from pathlib import Path
import argparse,os,shutil,subprocess,tempfile
from PIL import Image
p=argparse.ArgumentParser(description=__doc__);p.add_argument('shot',type=Path,nargs='?')
for name in ('exe','rom','sprites','work'):p.add_argument('--'+name,type=Path)
p.add_argument('--hd-hud',type=int,choices=(0,1),default=0,help='HUD HD on (the reported v2.0 case had it on)')
a=p.parse_args()
if a.shot is None:
 if not all((a.exe,a.rom,a.sprites,a.work)):p.error('pass a screenshot, or --exe --rom --sprites --work for the EU runtime fixture')
 a.work.mkdir(parents=True,exist_ok=True);run=Path(tempfile.mkdtemp(prefix='reload-order-',dir=a.work))
 for name in ('khcom.exe','SDL3.dll'):shutil.copy2(a.exe if name=='khcom.exe' else a.exe.parent/name,run/name)
 (run/'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=8\ninteger_zoom=0\nscale_mode=0\nhud_size=1\nexperimental_frame_skip=0\nwindow_scale=4\nhd_cards=1\ngraphics_mode=1\ngraphics_mode_applied=1\ncard_hud_raise=8\nhd_hud='+str(a.hd_hud)+'\nhd_scenes=0\nhd_movies=0\n')
 env={k:v for k,v in os.environ.items() if not k.startswith(('SDL_','KHCOM_'))}
 env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_FAST='1',KHCOM_BATTLE='204:8:0',KHCOM_MONKEY='19:120',KHCOM_KEYS='60-63:A',KHCOM_WINDOW='1920x1080',KHCOM_HD_SPRITES=str(a.sprites.resolve()),KHCOM_OUTPUT_SHOT='1572',KHCOM_QUIT='1575')
 subprocess.run([str((run/'khcom.exe').resolve()),'--rom',str(a.rom.resolve()),'--language','en'],cwd=run,env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=180,check=True)
 a.shot=run/'out_01572.bmp'
im=Image.open(a.shot).convert('RGB');assert im.size==(1920,1080),im.size
roi=im.crop((70,840,165,940));count=sum(min(roi.getpixel((x,y)))>205 for y in range(roi.height) for x in range(roi.width))
assert count>100, f'full reload hides its HD counter: only {count} white interior pixels'
print('PASS: real full reload keeps counter visible above native fill:',count,'white pixels')
