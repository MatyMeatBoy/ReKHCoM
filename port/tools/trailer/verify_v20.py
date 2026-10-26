"""Decode the entire final export, inspect streams, and create review contacts.

Requires NumPy/Pillow. Verification never opens a window or plays audio.
"""
import hashlib,json,os,subprocess
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
from v20_capture import T,FF,BUILD,HD,PUBLIC_V20_SHA,PUBLIC_V20_SOURCE
from v20_edit import FINAL,timeline,VIDEO_NAME

def call(args):
    p=subprocess.run(args,capture_output=True,creationflags=0x08000000)
    if p.returncode:raise RuntimeError(p.stderr.decode(errors='replace')[-6000:])
    return p
def main():
    video=FINAL/VIDEO_NAME
    probe=json.loads(call([str(Path(FF).with_name('ffprobe.exe')),'-v','error','-show_streams','-show_format','-of','json',str(video)]).stdout)
    vs=next(s for s in probe['streams'] if s['codec_type']=='video');au=next(s for s in probe['streams'] if s['codec_type']=='audio')
    assert (vs['width'],vs['height'],vs['avg_frame_rate'])==(1920,1080,'60/1')
    assert au['sample_rate']=='48000' and au['channels']==2
    delta=abs(float(vs['duration'])-float(au['duration']));assert delta<0.05,delta
    print('Decoding complete export',flush=True)
    call([FF,'-v','error','-xerror','-i',str(video),'-f','null','-'])
    pcm=np.frombuffer(call([FF,'-v','error','-i',str(video),'-vn','-ac','2','-ar','48000','-f','f32le','-']).stdout,dtype='<f4')
    peak=float(np.max(np.abs(pcm)));rms=float(np.sqrt(np.mean(pcm.astype(np.float64)**2)));assert peak<1.0,peak
    out=FINAL/'previews';out.mkdir(exist_ok=True)
    tl=json.loads((T/'timeline.json').read_text(encoding='utf-8'))
    assert all(s.get(k,'').isascii() for s in tl['segments'] for k in ('title','sub')),'Non-ASCII trailer card'
    font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',22)
    sheets=[]
    for base in range(0,len(tl['segments']),6):
        rows=tl['segments'][base:base+6];sheet=Image.new('RGB',(1920,len(rows)*388),(10,13,22));draw=ImageDraw.Draw(sheet)
        for j,s in enumerate(rows):
            for k,local in enumerate([1,min(3.5,s['d']/2),s['d']-0.8]):
                at=s['start']+local;p=out/f"{s['name']}_{k}.png"
                call([FF,'-v','error','-y','-ss',str(at),'-i',str(video),'-frames:v','1','-vf','scale=640:360',str(p)])
                sheet.paste(Image.open(p),(k*640,j*388));draw.text((k*640+8,j*388+362),f'{at:07.3f}s · {s["name"]}',font=font,fill='white')
        target=out/f'contact_{base//6+1}.jpg';sheet.save(target,quality=92);sheets.append(str(target))
    # An actual frame of the final HD title, with the final v2.0 typography.
    call([FF,'-v','error','-y','-ss','6','-i',str(video),'-frames:v','1',str(FINAL/'ReKHCoM_v2.0_thumbnail_final.png')])
    chapters=[]
    chapter_groups={'01_title':'HD title and extended arenas','04_monstro':'Worlds in motion','07_jafar':'Jafar','08_hud':'HD HUD and music','10_panorama':'Ultrawide and HD cinematics','11_options':'Options and Rebel mode','13_editor':'Save editor','14_stable':'Platforms and consoles','16_end':'Download and credits'}
    chapter_seconds=[]
    for s in tl['segments']:
        if s['name'] in chapter_groups:
            seconds=int(s['start']);chapter_seconds.append(seconds);chapters.append(f'{seconds//60:02}:{seconds%60:02} {chapter_groups[s["name"]]}')
    assert chapter_seconds[0]==0 and all(b-a>=10 for a,b in zip(chapter_seconds,chapter_seconds[1:])),chapter_seconds
    base=FINAL/'YouTube_descripcion_base.txt'
    description=base.read_text(encoding='utf-8') if base.exists() else 'ReKHCoM v2.0 — Feature trailer\nhttps://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.0\n'
    description=description.replace('[Chapters will be inserted from the verified final edit.]','').rstrip()
    description+='\n\nCHAPTERS\n'+'\n'.join(chapters)+'\n'
    if 'Nintendo Switch' not in description:description+='\nNintendo Switch: earlier beta v2.0-pre1, available separately; no Switch package in the stable v2.0 release.\n'
    (FINAL/'YouTube_descripcion.txt').write_text(description,encoding='utf-8')
    jobs=__import__('v20_capture').jobs();used={s.get('src') for s in tl['segments']}|{s.get('a') for s in tl['segments']}|{s.get('b') for s in tl['segments']}
    shots=[]
    for job in jobs:
        if job['name'] not in used:continue
        stage=T/'capture'/job['name'];meta=stage/'capture_job.json'
        if meta.exists():shot=json.loads(meta.read_text(encoding='utf-8'))
        else:
            sha=hashlib.sha256((stage/'khcom.exe').read_bytes()).hexdigest()
            assert sha==PUBLIC_V20_SHA,'Local/corrected captures need per-take provenance'
            shot=dict(**job,executable_sha256=sha,source=PUBLIC_V20_SOURCE,build_description='public v2.0')
        assert not shot.get('preview_only',False),'A preview cannot stand in for a recorded take'
        shots.append(shot)
    manifest=dict(reference_stable_build_sha256=PUBLIC_V20_SHA,reference_stable_source=PUBLIC_V20_SOURCE,capture_platform='Windows x86',imported_hd=str(HD),shots=shots,cinematic=dict(source='hd/movies/ROP.mp4',source_start=7,duration=12,presentation='Imported cinematic excerpt, not a runtime gameplay capture'),editor=dict(source='launcher-tall/decompui_6.png',presentation='Real hidden launcher UI capture, editorial vertical pan, no fabricated interaction'))
    atlas=json.loads((FINAL/'atlas_honors_inventory.json').read_text(encoding='utf-8'))
    approved_ids={'7ef20691e77d4bc0b99730ea3443f4af','3fa0a50b4582424cbef46d540076c838','d08d6c8758ff4ca9906c89a4c4d95888'}
    manifest['honors_references']=[dict(**a,status='honors',build_version=a['version']) for a in atlas if a['id'] in approved_ids]
    assert len(manifest['honors_references'])==3
    manifest['honors_source_files']=['port/src/port_guard_armor.c','port/include/port/port_agrabah109_tiles.h','port/include/port/port_scene074_tiles.h']
    manifest['darkside']='Arena approved; no honors claim. Honors for the separate standing-body piece are not attributed to this arena.'
    manifest['soundtrack_comparison']=json.loads((T/'audio/music_title_match.json').read_text())
    (FINAL/'shot_manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    report=dict(file=str(video),delivery_status=os.environ.get('KHCOM_TRAILER_DELIVERY_STATUS','review'),full_decode_exit=0,titles_ascii=True,video_codec=vs['codec_name'],width=vs['width'],height=vs['height'],fps=vs['avg_frame_rate'],frames=vs.get('nb_frames'),video_duration=vs['duration'],audio_duration=au['duration'],av_duration_delta=delta,audio_codec=au['codec_name'],sample_rate=au['sample_rate'],channels=au['channels'],audio_peak=peak,audio_rms=rms,bytes=video.stat().st_size,sha256=hashlib.sha256(video.read_bytes()).hexdigest(),contacts=sheets,chapters=chapters)
    (FINAL/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    (FINAL/'timeline.json').write_text((T/'timeline.json').read_text(encoding='utf-8'),encoding='utf-8')
    print(json.dumps(report,indent=2),flush=True)

if __name__=='__main__':main()
