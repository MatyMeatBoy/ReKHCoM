"""Reproducible v2.0 trailer: aligned moving comparisons and verified platform cards.

Inputs are private recordings from v20_capture.py, never still gameplay.
Launcher stills are deliberately labelled as an interface view.
"""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,ImageFilter
from v20_capture import T,FF,HD
FINAL=Path(os.environ.get('KHCOM_TRAILER_DELIVERY',T))
CARDS=T/'tools/cards';CARDS.mkdir(exist_ok=True,parents=True)
FONT='C:/Windows/Fonts/pala.ttf';BOLD='C:/Windows/Fonts/palab.ttf'
W,H=1920,1080
VIDEO_NAME=os.environ.get('KHCOM_TRAILER_VIDEO_NAME','ReKHCoM_v2.0_trailer_final.mp4')
def clean_text(value):
    for old,new in [('â†’',' -> '),('Â·',' | '),('\u2192',' -> '),('\u00b7',' | '),('\u2014','-'),('\u2013','-')]:value=value.replace(old,new)
    while '  ' in value:value=value.replace('  ',' ')
    assert value.isascii(),repr(value)
    return value
def heart_expression():
    # Multiply the v4 implicit polynomial by its positive denominator d^6.
    # Same heart and border, fewer divisions/pow calls, no mutable st/ld.
    # Independent 1080p checks at eight P values found zero changed pixels.
    return Path(__file__).with_name('heart_expr_v20.txt').read_text().strip()
def run(args):
    p=subprocess.run([FF,'-hide_banner','-loglevel','error','-y',*args],capture_output=True,text=True,creationflags=0x08000000)
    if p.returncode:raise RuntimeError(p.stderr[-7000:])
def card(name,title,subtitle='',full=False,extra=''):
    title,subtitle,extra=map(clean_text,(title,subtitle,extra))
    im=Image.new('RGBA',(W,H),(0,0,0,0));d=ImageDraw.Draw(im)
    if full:
        for y in range(H):
            a=int(180+40*abs(y-H/2)/(H/2));d.line((0,y,W,y),fill=(4,10,24,a))
        ty=350;ts=100;sy=490;ss=48
    else:
        for y in range(760,H):d.line((0,y,W,y),fill=(3,7,20,int(205*(y-760)/(H-760))))
        ty=875;ts=65;sy=963;ss=32
        d.line((720,852,1200,852),fill=(231,214,162,180),width=2)
    def centered(txt,y,size,color,bold=False):
        f=ImageFont.truetype(BOLD if bold else FONT,size)
        while d.textbbox((0,0),txt,font=f)[2]>1760:
            size-=2;f=ImageFont.truetype(BOLD if bold else FONT,size)
        d.text((W/2,y),txt,font=f,fill=color,anchor='mt',stroke_width=1,stroke_fill=(0,0,0,170))
    centered(title,ty,ts,(255,255,255,255),True)
    for i,line in enumerate(subtitle.split('\n')):centered(line,sy+i*64,ss,(231,214,162,255))
    if full:
        d.line((600,sy-28,1320,sy-28),fill=(221,203,151,200),width=2)
        if extra:centered(extra,875,28,(200,211,228,255))
    p=CARDS/(name+'.png');im.save(p);return p

def encode(name,inputs,graph,video='v',duration=None):
    out=T/'edit'/(name+'.mp4')
    sources=[]
    for inp in inputs:
        p=Path(inp[-1]);sources.append((str(p),p.stat().st_size,p.stat().st_mtime_ns))
    fingerprint=hashlib.sha256(json.dumps([sources,graph,duration]).encode()).hexdigest()
    stamp=out.with_suffix('.fingerprint')
    if out.exists() and out.stat().st_size>10000 and stamp.exists() and stamp.read_text()==fingerprint:return out
    args=[]
    for inp in inputs:args+=inp
    args+=['-filter_complex_threads','2','-filter_complex',graph,'-map',f'[{video}]','-an']
    if duration:args+=['-t',str(duration)]
    tmp=out.with_name(out.stem+'.partial.mp4')
    run(args+['-c:v','libx264','-preset','veryfast','-crf','17','-pix_fmt','yuv420p','-r','60','-threads','2',str(tmp)])
    os.replace(tmp,out)
    stamp.write_text(fingerprint)
    print(name,'rendered',flush=True);return out
def vin(name,ss=0,d=None):
    return ['-threads','1','-ss',str(ss)]+(['-t',str(d)] if d else [])+['-i',str(T/'clips'/(name+'.mov'))]
def normal(name,src,d,title,sub='',ss=0,full=False):
    c=card(name,title,sub,full)
    # Ultrawide footage is fitted, preserving its full original aspect.
    g=f'[0:v]setpts=PTS-STARTPTS,fps=60,scale=1920:1080:force_original_aspect_ratio=decrease,pad=1920:1080:(ow-iw)/2:(oh-ih)/2,setsar=1,format=yuv420p[b];[1:v]format=rgba,fade=t=in:d=0.5:alpha=1[c];[b][c]overlay=shortest=1:format=auto,format=yuv420p[v]'
    return encode(name,[vin(src,ss,d),['-loop','1','-framerate','60','-i',str(c)]],g,duration=d)
def compare(name,a,b,d,title,sub='',reveal=2.8,sweep=1.5,ss=0):
    c=card(name,title,sub)
    # Both sources start on the SAME source frame. The per-pixel wipe uses
    # the common global T, so B is never restarted at the sweep's offset.
    # xfade's second stream starts at its local zero at the transition's
    # offset: trim that stream to the SAME global source time explicitly.
    g=f'[0:v]setpts=PTS-STARTPTS,fps=60,format=yuv420p[a];[1:v]setpts=PTS-STARTPTS,fps=60,format=yuv420p[b];[a][b]xfade=transition=smoothleft:duration={sweep}:offset={reveal}[wipe];[2:v]format=rgba[c];[wipe][c]overlay=shortest=1:format=auto,format=yuv420p[v]'
    return encode(name,[vin(a,ss,reveal+sweep),vin(b,ss+reveal,d-reveal),['-loop','1','-framerate','60','-i',str(c)]],g,duration=d)
def still(name,path,d,title,sub='',tall=False,full=False):
    c=card(name,title,sub,full)
    if tall:g0=f'scale=1920:-2,crop=1920:1080:0:y=\'min(ih-oh,max(0,(ih-oh)*(t/{d})))\''
    else:g0=f'scale=1980:1114,crop=1920:1080:x=\'30+20*sin(t/{d}*PI)\':y=17'
    g=f'[0:v]{g0},fps=60,setsar=1,format=yuv420p[b];[1:v]format=rgba[c];[b][c]overlay=shortest=1:format=auto,format=yuv420p[v]'
    return encode(name,[['-loop','1','-framerate','60','-i',str(path)],['-loop','1','-framerate','60','-i',str(c)]],g,duration=d)

def timeline():
    shots=[
      dict(name='01_title',kind='compare',a='title_gba',b='title_hd',d=9,title='ReKHCoM  v2.0',sub='The original title · A new HD presentation',chapter='HD title'),
      dict(name='02_guard',kind='compare',a='guard_gba',b='guard_modern',d=10,title='THE SAME WORLD. BEYOND THE EDGE.',sub='GBA at x4 → Extended courtyard · Artwork approved with honors',chapter='Extended arenas'),
      dict(name='03_darkside',kind='compare',a='darkside_gba',b='darkside_modern',d=10,title='DARKSIDE · THE BATTLE, WIDER',sub='Same camera · Same moment · Real movement'),
      dict(name='04_monstro',src='room_monstro',d=6,title='EXPLORE YOUR MEMORIES',sub='Widescreen presentation · x4 scale',chapter='Worlds in motion'),
      dict(name='05_agrabah',kind='compare',a='agrabah109_gba',b='agrabah109_modern',d=6,ss=4,reveal=1.3,sweep=1.2,title='AGRABAH · A WORLD THAT CONTINUES',sub='GBA at x4 → Native extension · Artwork approved with honors'),
      dict(name='06_scene074',kind='compare',a='scene074_gba',b='scene074_modern',d=6,ss=4,reveal=1.3,sweep=1.2,title='DETAIL, ALL THE WAY TO THE EDGE',sub='GBA at x4 → Native extension · Artwork approved with honors'),
      dict(name='07_jafar',src='jafar_current',d=24,title='JAFAR',sub='A battle refined for the widescreen view',chapter='Jafar'),
      dict(name='08_hud',kind='compare',a='hud_gba',b='hud_hd_current',d=10,title='HD HUD',sub='Faces · Cards · Labels · Detail in every battle',chapter='HD HUD'),
      dict(name='09_music',src='hud_hd_current',d=10,title='THE SAME MELODY. A NEW SOUND.',sub='GBA music → Imported HD music · The same musical phrase',chapter='GBA and HD music'),
      dict(name='10_panorama',src='panorama_current',d=10,title='UP TO 32:9',sub='More room for every battle',chapter='Ultrawide view'),
      dict(name='10b_cinematics',src='cinematic_hd',d=12,title='HD CINEMATICS',sub='Imported footage · HD scenes and movies on Windows / Android',chapter='HD scenes and movies'),
      dict(name='11_options',src='options',d=7,title='MAKE IT YOUR OWN',sub='Every setting, in game',chapter='Options'),
      dict(name='12_rebel',src='rebel_jafar_current',d=10,title='REBEL MODE · CHEATS',sub='Experiment with assists and a local achievement list of your own',chapter='Rebel mode and cheats'),
      dict(name='13_editor',kind='still',path='launcher-tall/decompui_6.png',d=13,title='YOUR SAVE. YOUR WAY.',sub='Real editor view · Character · Level · HP · CP · Cards',tall=True,chapter='Save editor'),
      dict(name='14_stable',src='hud_hd_current',d=7,title='AVAILABLE NOW · v2.0',sub='Windows · Linux · Steam Deck',full=True,chapter='Platforms and consoles'),
      dict(name='15_beta',src='room_monstro',d=8,title='BETA PLATFORMS',sub='Android · Anbernic / Linux ARMHF\nPlayStation 4 · PlayStation Vita · PlayStation 5 · Web\nNintendo Switch · Earlier beta: v2.0-pre1',full=True),
      dict(name='16_end',src='title_hd',d=8,title='ReKHCoM  v2.0',sub='AVAILABLE NOW\ngithub.com/MatyMeatBoy/ReKHCoM',full=True,chapter='Download and credits'),
      dict(name='17_legal',src='title_hd',d=5,title='A FAN PROJECT',sub='Your own cartridge / ROM is required. No ROM is included.\nNot affiliated with Square Enix, Disney or Nintendo.',full=True)
    ]
    if os.environ.get('KHCOM_TRAILER_INCLUDE_RELOAD')=='1':
        index=next(i for i,s in enumerate(shots) if s['name']=='08_hud')+1
        shots[index:index]=[
          dict(name='08b_reload_first',kind='compare',a='reload_first_gba',b='reload_first_hd',d=10,title='ORIGINAL TIMING. HD RELOAD.',sub='Dynamic fill | Charge resumes | Reload counter',reveal=2.8,sweep=1),
          dict(name='08c_reload_cycle',kind='compare',a='reload_cycle_gba',b='reload_cycle_hd',d=10,title='FILL. HOLD. COMPLETE. REPEAT.',sub='Real card use | The next reload cycle | GBA -> HD',reveal=2.8,sweep=1),
        ]
    for shot in shots:
        for key in ('title','sub'):
            if key in shot:shot[key]=clean_text(shot[key])
    return shots

def segments(only=''):
    for s in timeline():
        if only and s['name'] not in only.split(','):continue
        kw={k:v for k,v in s.items() if k not in ('kind','chapter')}
        if s.get('kind')=='compare':compare(**kw)
        elif s.get('kind')=='still':
            kw['path']=FINAL/kw['path'];still(**kw)
        else:normal(**kw)

def assemble(only_act=None):
    ts=timeline();starts=[];t=0
    # Transitions are short and purposefully keep each modern result visible.
    after={'01_title':('heart',0.8),'02_guard':('flash',0.45),'03_darkside':('heart',0.8),'06_scene074':('flash',0.5),'07_jafar':('heart',0.8),'09_music':('flash',0.5),'10_panorama':('heart',0.7),'10b_cinematics':('flash',0.5),'12_rebel':('flash',0.45),'13_editor':('heart',0.8),'15_beta':('flash',0.6),'16_end':('black',0.8),'08c_reload_cycle':('flash',0.5)}
    transitions={i:after[s['name']] for i,s in enumerate(ts) if s['name'] in after}
    args=[];g=[];v='0:v';t=ts[0]['d'];starts=[0]
    heart=heart_expression()
    def fade_spec(kind,td,off):
        typ={'heart':'custom','flash':'fadewhite','black':'fadeblack','fade':'fade'}[kind]
        spec=f'xfade=transition={typ}:duration={td}:offset={off:.6f}'
        return spec+(f":expr='{heart}'" if kind=='heart' else '')
    groups=[];begin=0
    while begin<len(ts):
        end=min(begin+4,len(ts))
        # Keep every heart inside an act, where native masks are used;
        # the final join then needs only the inexpensive built-in fades.
        while end<len(ts) and transitions.get(end-1,('fade',.28))[0]=='heart' and end>begin+2:end-=1
        if len(ts)-end==1:end=len(ts)
        groups.append(list(range(begin,end)));begin=end
    assert all(transitions.get(ids[0]-1,('fade',.28))[0]!='heart' for ids in groups[1:])
    acts=[];actdur=[]
    # Keep decoder/filter buffers bounded while preserving every transition.
    for gi,ids in enumerate(groups):
        if only_act is not None and gi!=only_act:continue
        aa=[];gg=[];vv='0:v';tt=ts[ids[0]]['d'];p=T/'edit'/f'act_v20_{gi}.mp4'
        for i in ids:aa+=['-threads','1','-i',str(T/'edit'/(ts[i]['name']+'.mp4'))]
        mask_input=len(ids)
        for j,i in enumerate(ids[1:],1):
            kind,td=transitions.get(i-1,('fade',0.28));off=tt-td
            total=off+ts[i]['d']
            if kind=='heart':
                count=round(td*60);prefix=round(off*60);pad=ts[i]['d']+1
                for mask in ('inner','outer'):
                    aa+=['-threads','1','-i',str(T/'heart-masks'/f'heart_{count}_{mask}_444.mkv')]
                aa+=['-f','lavfi','-i',f'color=white:s=1920x1080:r=60:d={total},format=yuv444p,lut=y=245:u=128:v=128']
                gg.append(f'[{vv}]fps=60,format=yuv444p,settb=1/60,setpts=N,tpad=stop_mode=clone:stop_duration={pad},trim=duration={total}[ha{j}]')
                gg.append(f'[{j}:v]fps=60,format=yuv444p,settb=1/60,setpts=N,tpad=start_mode=clone:start_duration={off},trim=duration={total}[hb{j}]')
                for n,label in enumerate(('hi','ho')):
                    # tpad's legal black is Y16/U128/V128. Explicitly zero
                    # EVERY prefix plane; otherwise it would leak B/chroma.
                    gg.append(f'[{mask_input+n}:v]fps=60,format=yuv444p,settb=1/60,setpts=N,tpad=start_mode=add:start_duration={off}:stop_mode=clone:stop_duration={pad},lut=y=0:u=0:v=0:enable=\'lt(n,{prefix})\',trim=duration={total}[{label}{j}]')
                gg.append(f'[ha{j}][{mask_input+2}:v][ho{j}]maskedmerge=planes=7[hw{j}]')
                gg.append(f'[hw{j}][hb{j}][hi{j}]maskedmerge=planes=7,trim=duration={total},format=yuv420p[x{j}]')
                mask_input+=3
            else:
                gg.append(f'[{vv}]settb=1/60,setpts=N[pa{j}];[{j}:v]settb=1/60,setpts=N[pb{j}]')
                gg.append(f'[pa{j}][pb{j}]{fade_spec(kind,td,off)}[x{j}]')
            vv=f'x{j}';tt=total
        valid=False
        stamp=p.with_suffix('.fingerprint')
        identity=hashlib.sha256(json.dumps([(str(T/'edit'/(ts[i]['name']+'.mp4')),(T/'edit'/(ts[i]['name']+'.mp4')).stat().st_mtime_ns) for i in ids]+[gg]).encode()).hexdigest()
        if p.exists() and p.stat().st_size>=10000:
            check=subprocess.run([str(Path(FF).with_name('ffprobe.exe')),'-v','error','-show_entries','format=duration','-of','default=nw=1:nk=1',str(p)],capture_output=True,text=True,creationflags=0x08000000)
            valid=check.returncode==0 and bool(check.stdout.strip()) and abs(float(check.stdout.strip())-tt)<0.06 and stamp.exists() and stamp.read_text()==identity
        if only_act is not None or not valid:
            partial=p.with_name(p.stem+'.partial.mp4')
            run(aa+['-filter_complex_threads','2','-filter_complex',';'.join(gg),'-map',f'[{vv}]','-an','-c:v','libx264','-preset','veryfast','-crf','17','-threads','2',str(partial)])
            os.replace(partial,p)
            stamp.write_text(identity)
            print('Act',gi,'rendered',flush=True)
        acts.append(p);actdur.append(tt)
    if only_act is not None:return
    for i in range(1,len(ts)):
        kind,td=transitions.get(i-1,('fade',0.28));off=t-td;starts.append(off)
        t=off+ts[i]['d']
    for p in acts:args+=['-threads','1','-i',str(p)]
    video_time=actdur[0]
    for gi in range(1,len(acts)):
        kind,td=transitions.get(groups[gi][0]-1,('fade',0.28));off=video_time-td
        g.append(f'[{v}][{gi}:v]{fade_spec(kind,td,off)}[vx{gi}]');v=f'vx{gi}';video_time=off+actdur[gi]
    # Imported soundtrack, mapped and supported by v2.0's own music loader.
    ni=len(acts)
    args+=['-i',str(HD/'music/Title.ogg'),'-stream_loop','-1','-i',str(HD/'music/26_Aladdin_B.ogg'),'-i',str(T/'audio/music_title_gba_matched.wav'),'-i',str(T/'audio/music_title_hd_matched.wav')]
    sfx=[]
    for i,s in enumerate(ts):
        source=s.get('src',s.get('b'))
        if source and (T/'audio'/(source+'.wav')).exists():
            ai=ni+4+len(sfx);sfx.append('sfx'+str(i))
            args+=['-i',str(T/'audio'/(source+'.wav'))]
            ss=s.get('ss',0)
            g.append(f'[{ai}:a]atrim={ss}:{ss+s["d"]},asetpts=PTS-STARTPTS,afade=t=in:d=0.1,afade=t=out:st={s["d"]-0.3}:d=0.3,volume=0.7,adelay={round(starts[i]*1000)}:all=1[sfx{i}]')
    introend=starts[3];endstart=starts[-2]
    # First reveal is also the music reveal: native and HD share the same
    # phrase offset; equal loudness was measured separately before assembly.
    g.append(f'[{ni+2}:a]atrim=0:2.8,asetpts=PTS-STARTPTS,volume=1,afade=t=out:st=2.55:d=0.25[mgba]')
    g.append(f'[{ni}:a]asplit=2[titlea][titlec]')
    hdgain=json.loads((T/'audio/music_title_match.json').read_text())[1]['gain']
    g.append(f'[titlea]atrim=2.8:{introend},asetpts=PTS-STARTPTS,volume={hdgain},afade=t=in:d=0.25,afade=t=out:st={introend-2.8-0.8}:d=0.8,adelay=2800:all=1[mintro]')
    g.append(f'[{ni+1}:a]atrim=0:{endstart-introend},asetpts=PTS-STARTPTS,volume=0.30,afade=t=in:d=0.5,afade=t=out:st={endstart-introend-0.8}:d=0.8,adelay={round(introend*1000)}:all=1[mbattle]')
    musicstart=starts[next(i for i,s in enumerate(ts) if s['name']=='09_music')]
    # The dedicated feature repeats one phrase: 0..4 native then 4..10 HD.
    # Battle bed is ducked through this comparison.
    cinestart=starts[next(i for i,s in enumerate(ts) if s['name']=='10b_cinematics')]
    g[-1]=g[-1].replace('volume=0.30',f"volume='if(between(t,{musicstart-introend},{musicstart-introend+10})+between(t,{cinestart-introend},{cinestart-introend+12}),0,0.30)':eval=frame")
    g.append(f'[{ni+3}:a]atrim=3.9:10,asetpts=PTS-STARTPTS,afade=t=in:d=0.2,adelay={round((musicstart+3.9)*1000)}:all=1[mfeaturehd]')
    # Reuse native input via another input to avoid ambiguous fan-out.
    nativeidx=ni+4+len(sfx)
    args+=['-i',str(T/'audio/music_title_gba_matched.wav')]
    g.append(f'[{nativeidx}:a]atrim=0:4.1,asetpts=PTS-STARTPTS,afade=t=out:st=3.9:d=0.2,adelay={round(musicstart*1000)}:all=1[mfeaturegba]')
    g.append(f'[titlec]atrim=10:{10+t-endstart},asetpts=PTS-STARTPTS,volume=0.65,afade=t=in:d=0.7,afade=t=out:st={t-endstart-2.5}:d=2.5,adelay={round(endstart*1000)}:all=1[mend]')
    labels=['mgba','mintro','mbattle','mfeaturehd','mfeaturegba','mend']+sfx
    g.append(''.join(f'[{x}]' for x in labels)+f'amix=inputs={len(labels)}:normalize=0,atrim=0:{t},loudnorm=I=-16:TP=-1.5:LRA=11,aresample=48000[a]')
    FINAL.mkdir(exist_ok=True,parents=True);out=FINAL/VIDEO_NAME
    (T/'timeline.json').write_text(json.dumps(dict(duration=t,segments=[dict(**s,start=starts[i]) for i,s in enumerate(ts)]),ensure_ascii=False,indent=2),encoding='utf-8')
    (T/'assembly_filter.txt').write_text(';'.join(g),encoding='utf-8')
    partial=out.with_name(out.stem+'.partial.mp4')
    run(args+['-filter_complex_threads','2','-filter_complex',';'.join(g),'-map',f'[{v}]','-map','[a]','-t',str(t),'-c:v','libx264','-preset','fast','-crf','17','-pix_fmt','yuv420p','-r','60','-threads','2','-c:a','aac','-b:a','256k','-movflags','+faststart',str(partial)])
    os.replace(partial,out)
    print('EXPORTED',out,t,flush=True)

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--only',default='');ap.add_argument('--assemble',action='store_true');ap.add_argument('--repair-act',type=int);args=ap.parse_args()
    if args.repair_act is not None:assemble(args.repair_act)
    elif args.assemble:assemble()
    else:segments(args.only)
