"""Silent, isolated captures from the unchanged public v2.0 executable.

No installed configuration or player save is touched. See v20_manifest.json.
"""
import argparse, array, hashlib, json, os, shutil, subprocess, sys, time, wave
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[3]
T=Path(os.environ.get('KHCOM_TRAILER_DIR',ROOT.parent/'arte/Trailer/v20')).resolve()
BUILD=Path(os.environ.get('KHCOM_TRAILER_BUILD','D:/01Claude/ReKHCoM-build/windows-release-now/eu'))
HD=Path(os.environ.get('KHCOM_TRAILER_HD','D:/01Claude/ReKHCoM-build/hd-completo'))
ROM=Path(os.environ.get('KHCOM_TRAILER_ROM',ROOT.parent/'roms/Kingdom Hearts - Chain of Memories (Europe) (En,Fr,De,Es,It).gba'))
FF=shutil.which('ffmpeg') or str(next((Path(os.environ['LOCALAPPDATA'])/'Microsoft/WinGet/Packages').glob('Gyan.FFmpeg_*/ffmpeg-*/bin/ffmpeg.exe')))
for d in ['clips','audio','previews','capture','edit','tools/cards']:(T/d).mkdir(parents=True,exist_ok=True)

def wavdump(raw,out,start,end):
    data=array.array('f');data.frombytes(raw.read_bytes());pcm=array.array('h');pos=0
    for i,line in enumerate(Path(str(raw)+'.rates').read_text().splitlines()):
        n,r=map(int,line.split())
        if start<=i<=end:
            for k in range(800):
                if not n:pcm.extend((0,0));continue
                t=k*(n-1)/799;j=int(t);f=t-j;j2=min(j+1,n-1)
                for c in (0,1):pcm.append(int(max(-1,min(1,data[(pos+j)*2+c]*(1-f)+data[(pos+j2)*2+c]*f))*32767))
        pos+=n
    with wave.open(str(out),'wb') as w:
        w.setnchannels(2);w.setsampwidth(2);w.setframerate(48000);w.writeframes(pcm.tobytes())
    raw.unlink();Path(str(raw)+'.rates').unlink()

def entries():
    sys.path.insert(0,str(ROOT/'port/tools'))
    from test_battles import battle_entries,stage_table
    es=battle_entries();ss=stage_table()
    def boss(n):
        i,w,_=next(e for e in es if e[2]=='BATTLE_'+n)
        s=next((i for i,s in enumerate(ss) if s.replace('BATTLE_STAGE_','WORLD_')==w),0)
        return f'{i}:{s}:0'
    return boss

def jobs():
    boss=entries();out=[]
    for hd in (False,True):
        out.append(dict(name='title_'+('hd' if hd else 'gba'),a=850,b=1390,hd=hd,keys='200-203:A',extra={}))
        for n,battle in [('guard',boss('GUARD_ARMOR')),('coliseum','10:4:0'),('darkside',boss('DARKSIDE'))]:
            off={'guard':{'KHCOM_NO_GUARD_PATIO':'1'},'coliseum':{'KHCOM_ARENAS_V2':'0','KHCOM_ARENAS_APPROVED':'0'},'darkside':{'KHCOM_DARKSIDE_BEACH':'0'}}[n] if not hd else {}
            out.append(dict(name=n+('_modern' if hd else '_gba'),a=180,b=840,hd=False,keys='60-63:A,200-340:LEFT,360-500:RIGHT,520-580:B,600-720:A,730-840:LEFT',extra=dict(KHCOM_BATTLE=battle,**off)))
        for n,event,scene in [('agrabah109','0:108',109),('scene074','0:74',74)]:
            off={} if hd else {'KHCOM_NO_SCENE_PROFILES':'1',f'KHCOM_SCENE{scene:03d}_WIDE':'0','KHCOM_AGRABAH109_WIDE':'0','KHCOM_PRELIMINAR_SKIP':str(scene)}
            out.append(dict(name=n+('_modern' if hd else '_gba'),a=180,b=780,hd=False,keys='240-243:A,420-423:A,600-603:A',extra=dict(KHCOM_EVENT=event,**off)))
    out+= [dict(name='jafar',a=180,b=1740,hd=True,keys='60-63:A',extra={'KHCOM_BATTLE':boss('JAFAR'),'KHCOM_MONKEY':'19:120'}),
           dict(name='hud_hd',a=180,b=900,hd=True,keys='60-63:A',extra={'KHCOM_BATTLE':'10:4:0','KHCOM_MONKEY':'6:120'}),
           dict(name='hud_gba',a=180,b=900,hd=False,keys='60-63:A',extra={'KHCOM_BATTLE':'10:4:0','KHCOM_MONKEY':'6:120'}),
           dict(name='panorama',a=180,b=900,hd=True,size='2560x720',keys='60-63:A',extra={'KHCOM_BATTLE':'10:7:0','KHCOM_MONKEY':'11:120'}),
           dict(name='rebel_action',a=180,b=1020,hd=True,keys='60-63:A',ini='cheat_infinite_hp=1\ncheat_one_hit=1\n',extra={'KHCOM_BATTLE':'10:6:0','KHCOM_MONKEY':'6:120'}),
           dict(name='options',a=510,b=1030,hd=True,keys='',extra={'KHCOM_OPTIONS_AT':'500','KHCOM_VPAD':','.join(f'{f}-{f+3}:DOWN' for f in range(570,1020,40))}),
           dict(name='room_monstro',a=430,b=910,hd=True,keys='420-910:UP,480-486:B,540-546:B,600-606:B',extra={'KHCOM_MODE':'300:room:10301'}),
           dict(name='room_agrabah',a=430,b=910,hd=True,keys='420-910:UP',extra={'KHCOM_MODE':'300:room:10401'}),
           dict(name='room_halloween',a=430,b=910,hd=True,keys='420-910:UP',extra={'KHCOM_MODE':'300:room:10801'})]
    for job in out:
        if job['name'] in ('jafar','hud_hd','panorama','rebel_action'):job['name']+='_current'
    return out

def capture(j,preview=False):
    name=j['name'];d=T/'capture'/name;d.mkdir(exist_ok=True);a,b=j['a'],j['b'];out=T/'clips'/(name+'.mov');wav=T/'audio'/(name+'.wav')
    if not preview and out.exists() and wav.exists():return name+' cached'
    for n in ['khcom.exe','SDL3.dll']:shutil.copy2(BUILD/n,d/n)
    # Junctions avoid copying imported, private media to the repository.
    for target,source in [('hd',HD),('assets',ROOT/'port/assets')]:
        if not (d/target).exists():
            subprocess.run(['cmd','/c','mklink','/J',str(d/target),str(source)],check=True,creationflags=0x08000000,stdout=subprocess.DEVNULL)
    hd=int(j['hd'])
    (d/'kh_com.ini').write_text(f'config_version=11\ngraphics_mode=1\ngraphics_mode_applied=1\nview_mode=4\nzoom=8\nnarrow_fit=1\nwindow_scale=4\nfullscreen=0\nvsync=0\nexperimental_frame_skip=0\nhd_title={hd}\nhd_cards={hd}\nhd_music=0\nhd_text={hd}\nhd_movies=0\nhd_scenes=0\nmusic_volume=0\nsfx_volume=100\n'+j.get('ini',''))
    env={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))}
    env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_JOYSTICK_HIDAPI='0',SDL_JOYSTICK_RAWINPUT='0',SDL_XINPUT_ENABLED='0',SDL_JOYSTICK_DIRECTINPUT='0',SDL_JOYSTICK_WGI='0',SDL_JOYSTICK_GAMEINPUT='0',SDL_JOYSTICK_GAMEINPUT_RAW='0',KHCOM_HIDDEN='1',KHCOM_FAST='1',KHCOM_WINDOW=j.get('size','1920x1080'),KHCOM_KEYS=j['keys'],KHCOM_QUIT=str(b+3),KHCOM_OUTPUT_SHOT=f'{a},{(a+b)//2},{b}')
    env.update(SDL_RENDER_DRIVER='software',KHCOM_BACKGROUND_TEST='1',KHCOM_PPU_THREADS='2')
    env.update(j['extra'])
    if not preview:env.update(KHCOM_RECORD=f'{a}-{b}:{out}',KHCOM_FFMPEG=FF,KHCOM_WAVDUMP=str(d/'audio.raw'))
    proc=subprocess.Popen([str(d/'khcom.exe'),'--rom',str(ROM),'--language','en'],cwd=d,env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,creationflags=0x08000000)
    deadline=time.monotonic()+1200;ui_stopped=False
    while proc.poll() is None:
        if name=='options' and not preview and (d/'kh_com.log').exists() and 'record: done' in (d/'kh_com.log').read_text(errors='replace'):
            # The paused Options overlay does not service the game's quit
            # frame. Recording has closed its file: stop only our child.
            proc.terminate();proc.wait();ui_stopped=True;break
        if time.monotonic()>deadline:
            proc.kill();proc.wait();raise TimeoutError(name)
        time.sleep(0.25)
    if proc.returncode and not ui_stopped:raise RuntimeError(f'{name}: game exit {proc.returncode}')
    for frame in [a,(a+b)//2,b]:
        p=d/f'out_{frame:05d}.bmp'
        if p.exists():Image.open(p).resize((640,360)).save(T/'previews'/f'{name}_{frame}.png')
    if not preview:
        assert out.stat().st_size>10000
        if ui_stopped and not (d/'audio.raw').stat().st_size:
            with wave.open(str(wav),'wb') as w:
                w.setnchannels(2);w.setsampwidth(2);w.setframerate(48000);w.writeframes(bytes(round((b-a+1)/60*48000)*4))
        else:wavdump(d/'audio.raw',wav,a,b)
    return name+' captured'

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--preview',action='store_true');ap.add_argument('--only',default='');ap.add_argument('--workers',type=int,default=2);args=ap.parse_args()
    js=[j for j in jobs() if not args.only or j['name'] in args.only.split(',')]
    (T/'capture_manifest.json').write_text(json.dumps(dict(build=str(BUILD),sha256=hashlib.sha256((BUILD/'khcom.exe').read_bytes()).hexdigest(),source='59b44df99e05e1f4245516447d5800c88be72cf1',jobs=js),indent=2))
    with ThreadPoolExecutor(args.workers) as pool:
        for result in pool.map(lambda j:capture(j,args.preview),js):print(result,flush=True)

if __name__=='__main__':main()
