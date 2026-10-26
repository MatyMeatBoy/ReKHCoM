"""Inspect both actual presented movie pause dialogs, silently and in private folders.

Requires a built regional exe and a private HD import containing zz_event_151.mp4.
Uses no save, no gallery cache and never modifies the import.
"""
import argparse,os,shutil,subprocess,tempfile
from pathlib import Path
from PIL import Image

def verify(path):
 im=Image.open(path).convert('RGB');w,h=im.size
 # At this ending's black transition the only white area is the pause panel.
 # Its width depends on the localized title (German is wider than English).
 panel=Image.eval(im.convert('L'),lambda p:255 if p>250 else 0).getbbox()
 assert panel,('missing white panel',str(path))
 def pixels(x0,y0,x1,y1):
  crop=im.crop((int(x0),int(y0*h/160),int(x1),int(y1*h/160)))
  return (crop.getpixel((x,y)) for y in range(crop.height) for x in range(crop.width))
 red=sum(r>100 and g<100 and b<100 and r>g*1.5 for r,g,b in pixels(panel[0],52,panel[2],62))
 dark=[sum(max(rgb)<100 for rgb in pixels(panel[0]+24*w/240,y,panel[2]-8*w/240,y+7)) for y in (71,87)]
 assert red>10 and min(dark)>10,(str(path),red,dark,'missing title or option text')
 print(path.name,'title pixels',red,'row pixels',dark)

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('exe','rom','movies','work'):p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--region',choices=('eu','us','jp'),default='eu')
 a=p.parse_args();a.work.mkdir(parents=True,exist_ok=True)
 cases=[('en',1,'640x360'),('es',1,'960x960'),('fr',1,'640x360'),('de',1,'640x360'),('it',1,'640x360'),('en',0,'640x360')] if a.region=='eu' else [('native',1,'640x360'),('native',0,'640x360')]
 for lang,hd,window in cases:
  run=Path(tempfile.mkdtemp(prefix=lang+'-'+str(hd)+'-',dir=a.work))
  for name in ('khcom.exe','SDL3.dll'):shutil.copy2(a.exe if name=='khcom.exe' else a.exe.parent/name,run/name)
  (run/'hd').mkdir()
  if os.name=='nt':subprocess.run(['cmd','/c','mklink','/J',str((run/'hd/scenes').resolve()),str(a.movies.resolve())],check=True,stdout=subprocess.DEVNULL)
  else:(run/'hd/scenes').symlink_to(a.movies.resolve(),target_is_directory=True)
  (run/'kh_com.ini').write_text(f'config_version=11\nhd_movies=1\nhd_scenes=0\nhd_title={hd}\nfullscreen=0\nview_mode=0\nzoom=0\nvsync=0\nhd_subtitles=0\n')
  env={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))}
  env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_MOVIE='4',KHCOM_HDMOVIE_PAUSETEST='223',KHCOM_HDMOVIE_SECONDS='228',KHCOM_FAST='1',KHCOM_QUIT='200',KHCOM_WINDOW=window)
  subprocess.run([str((run/'khcom.exe').resolve()),'--rom',str(a.rom.resolve()),'--language',lang],cwd=run,env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=60,check=True)
  for name in ('hdmovie_pause.bmp','hdmovie_skip_confirm.bmp'):verify(run/name)
 print('PASS:',a.region,len(cases),'isolated sessions; HD/GBA fonts; both dialogs readable')
if __name__=='__main__':main()
