"""Directed real UI regression: skip/reenter an HD scene without retaining a black poster.

Uses private synthetic media and data; no player import/save/window/audio.
"""
import argparse, os, shutil, subprocess
from pathlib import Path
from PIL import Image

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','rom','work'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--no-shots',action='store_true',help='validate poster sidecar when software SDL readback is unavailable')
    p.add_argument('--repeats',type=int,default=8)
    a=p.parse_args();a.work.mkdir(parents=True,exist_ok=True)
    for name in ('khcom.exe','SDL3.dll'):shutil.copy2(a.exe if name=='khcom.exe' else a.exe.parent/name,a.work/name)
    assets=a.exe.parent/'assets'
    if not assets.is_dir(): assets=Path(__file__).resolve().parents[1]/'assets'
    shutil.copytree(assets,a.work/'assets',dirs_exist_ok=True)
    movie=a.work/'hd/scenes/zz_event_123.mp4';movie.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run(['ffmpeg','-v','error','-y','-f','lavfi','-i','color=black:s=640x360:r=30:d=0.5',
        '-f','lavfi','-i','color=red:s=640x360:r=30:d=3.5','-filter_complex','[0:v][1:v]concat=n=2:v=1:a=0[v]',
        '-map','[v]','-c:v','libx264','-g','300','-keyint_min','300','-sc_threshold','0','-pix_fmt','yuv420p',str(movie)],
        check=True,creationflags=subprocess.CREATE_NO_WINDOW)
    # Seed the bad but well-formed PNG cached by old MF code. The movie stays intact.
    Image.new('RGB',(640,360)).save(movie.with_suffix('.poster-v1-t1-640x360.png'))
    (a.work/'scenes_seen.dat').write_bytes(b'\xff'*128)
    (a.work/'kh_com.ini').write_text('config_version=11\ngraphics_mode=0\ngraphics_mode_applied=0\n'
        'hd_title=1\nhd_text=1\nhd_music=0\nfullscreen=0\nview_mode=4\nzoom=8\nvsync=0\n')
    close=70+50*a.repeats
    keys=','.join([f'{70+50*i}:120' for i in range(a.repeats)]+[f'{close}:-1'])
    shots=[65+50*i for i in range(a.repeats+1)]
    env={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))}
    env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_HIDDEN='1',KHCOM_FAST='1',SDL_RENDER_DRIVER='software',
        KHCOM_BACKGROUND_TEST='1',KHCOM_PPU_THREADS='1',KHCOM_WINDOW='1920x1080',KHCOM_MODE='1:mode_title',
        KHCOM_OPTIONS_AT='120',KHCOM_SCENES_SHOT='2',KHCOM_GALLERY_EVENT='56',KHCOM_GALLERY_KEYS=keys,
        KHCOM_GALLERY_SHOTS=','.join(map(str,shots)),KHCOM_GALLERY_QUIT_AFTER=str(close),KHCOM_QUIT='8000',
        KHCOM_HDMOVIE_PAUSETEST='0',KHCOM_HDMOVIE_PAUSE_ACTION='skip')
    if a.no_shots: env.pop('KHCOM_GALLERY_SHOTS')
    with (a.work/'run.log').open('w') as log:
        subprocess.run([str((a.work/'khcom.exe').resolve()),'--rom',str(a.rom.resolve()),'--language','en'],cwd=a.work,env=env,
            stdout=log,stderr=subprocess.STDOUT,timeout=180,check=True,creationflags=subprocess.CREATE_NO_WINDOW)
    text=(a.work/'run.log').read_text(errors='replace')
    assert text.count('HD zz_event_123 -> 1')==a.repeats, text[-4000:]
    assert 'bios:' not in text and 'CRASH' not in text,text[-4000:]
    for tick in ([] if a.no_shots else shots[1:]):
        im=Image.open(a.work/f'gallery_{tick:04d}.bmp').convert('RGB')
        crop=im.crop((int(im.width*.63),int(im.height*.25),int(im.width*.94),int(im.height*.48)))
        red=sum(r>100 and g<50 and b<50 for r,g,b in crop.get_flattened_data())
        assert red>1000,(tick,red,'HD poster black after return')
    poster=Image.open(movie.with_suffix('.poster-v1-t1-640x360.png')).convert('RGB')
    assert sum(r>100 and g<50 and b<50 for r,g,b in poster.get_flattened_data())>200000,'old black cache retained'
    detail='poster pixels checked in sidecar (SDL readback unavailable)' if a.no_shots else 'every returned poster checked on screen'
    print(f'PASS: {a.repeats} actual HD playback/skip/return cycles; {detail}; old black PNG rebuilt')

if __name__=='__main__':main()
