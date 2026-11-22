#!/usr/bin/env python3
"""ROM-free full-frame comparison with an independent GBA pixel oracle.

Synthetic register/VRAM/OAM fixtures render through the production PPU.
Expected pixels use the hardware equations, not production rendering helpers.
Reference: https://mgba-emu.github.io/gbatek/ (GBA LCD video controller).
"""
import argparse
from array import array
import datetime
import json
import os
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
CASES = ['forced', 'backdrop']
CASES += [f'text_{bpp}_size{size}{mos}' for bpp in ('4bpp', '8bpp')
          for size in range(4) for mos in ('', '_mosaic')]
CASES += ['text_4bpp_hblank']
CASES += [f'affine_{bg}_{wrap}{rot}{mos}' for bg in ('bg2', 'bg3')
          for wrap in ('wrap', 'nowrap') for rot in ('', '_rotate') for mos in ('', '_mosaic')]
CASES += ['affine_bg2_size1_wrap_rotate_mosaic']
CASES += [f'bitmap{mode}_{page}{rot}{mos}' for mode in (3,4,5)
          for page in (('page0',) if mode==3 else ('page0','page1'))
          for rot in ('', '_rotate') for mos in ('', '_mosaic')]
CASES += [f'obj_{bpp}_{mapping}{variant}' for bpp in ('4bpp','8bpp')
          for mapping in ('1d','2d') for variant in ('','_flip','_affine','_mosaic')]
CASES += ['compose_bg_tie','compose_obj_tie','compose_obj_behind','compose_obj_oam_tie',
          'compose_alpha','compose_alpha_clamp','compose_alpha_blocked','compose_alpha_backdrop',
          'compose_bright','compose_dark','compose_obj_bright','compose_obj_dark',
          'compose_window_priority','compose_window_wrapped','compose_semi_window']
CASES += [f'semantic_hd_{layer}{cover}' for layer in ('world','hud','screen')
          for cover in ('','_cover_bg','_cover_obj','_window')]
CASES += ['semantic_hd_compact','semantic_bgtext','semantic_bgtext_stale','semantic_bgtext_clear',
          'semantic_bgtext_cover_obj','semantic_bgtext_cover_bg','semantic_bgtext_screen',
          'semantic_bgtext_explicit']
CASES += ['semantic_hd_hud_message']
CASES += ['fast_'+name for name in CASES if not name.startswith('semantic')]
CASES += ['fast_'+name for name in CASES if name.startswith('semantic') and not any(layer in name for layer in ('hud','screen'))]

def color(i):
    return ((i*3)&31) | (((i*5)&31)<<5) | (((i*7)&31)<<10)

def rgb(c):
    result = 0xff000000
    for shift, out in ((0,16),(5,8),(10,0)):
        v=(c>>shift)&31
        result |= ((v<<3)|(v>>2)) << out
    return result

def mix(a,b,eva=8,evb=8):
    return sum(min(31,(((a>>s)&31)*eva+((b>>s)&31)*evb)//16)<<s for s in (0,5,10))

def composition_pixel(name,x,y):
    backdrop=color(17)
    mask=0x3f
    if 'window' in name:
        if 'wrapped' in name:
            win0=(x>=220 or x<20) and (y>=150 or y<10)
        else: win0=30<=x<90 and 10<=y<70
        win1=60<=x<120 and 40<=y<100
        winobj=70<=x<86 and 50<=y<66
        mask=1 if win0 else 2 if win1 else 0x10 if winobj else 0x13
        if 'semi' in name: mask=0x13
    layers=[]
    if mask&1: layers.append((1,1,31,'bg0'))
    if mask&2 and 'backdrop' not in name: layers.append((1 if 'bg_tie' in name else 2,2,31<<5,'bg1'))
    if ('obj' in name or 'semi' in name or 'window' in name) and mask&16 and 40<=x<56 and 30<=y<46:
        layers.append((3 if 'obj_behind' in name else 1,0,31<<10,'obj'))
    layers.sort(key=lambda p:p[:2])
    layers.append((4,9,backdrop,'backdrop'))
    top=layers[0]; second=layers[1] if len(layers)>1 else layers[0]
    c=top[2]
    if 'semi' in name and top[3]=='obj' and second[3]=='bg0': c=mix(c,second[2])
    elif 'alpha' in name and top[3]=='bg0':
        target='backdrop' if 'backdrop' in name or 'blocked' in name else 'bg1'
        if second[3]==target: c=mix(c,second[2],31 if 'clamp' in name else 8,31 if 'clamp' in name else 8)
    elif 'bright' in name:
        c=sum((((c>>s)&31)+(31-((c>>s)&31))*9//16)<<s for s in (0,5,10))
    elif 'dark' in name:
        c=sum((((c>>s)&31)-((c>>s)&31)*9//16)<<s for s in (0,5,10))
    return c

def expected(name):
    buf=array('I',[rgb(color(17))])*(160*240)
    if name=='forced': return array('I',[0xffffffff])*(160*240)
    if name=='backdrop': return buf
    for y in range(160):
        for x in range(240):
            c=None
            if name.startswith('compose'):
                c=composition_pixel(name,x,y)
            elif name.startswith('text'):
                size=int(name.split('size')[1][0]) if 'size' in name else 0
                gx=x-x%4 if 'mosaic' in name else x
                gy=y-y%3 if 'mosaic' in name else y
                px=(gx+(3*y if 'hblank' in name else 249))%(512 if size&1 else 256)
                py=(gy+251)%(512 if size&2 else 256)
                tx,ty=px//8,py//8
                fx=7-px%8 if tx&1 else px%8
                fy=7-py%8 if ty&1 else py%8
                ci=1+((tx+5*ty)%16+fx+3*fy)%(255 if '8bpp' in name else 15)
                if '4bpp' in name: ci+=((tx+ty)%4)*16
                c=color(ci)
            elif name.startswith('affine'):
                gx=x-x%4 if 'mosaic' in name else x
                gy=y-y%3 if 'mosaic' in name else y
                if 'rotate' in name:
                    px=(-7*256+192*gx+96*gy)//256
                    py=(11*256-96*gx+192*gy)//256
                else: px,py=-7+gx,11+gy
                side=256 if 'size1' in name else 128
                if 'nowrap' not in name: px,py=px%side,py%side
                if 0<=px<side and 0<=py<side:
                    tile=(px//8+2*(py//8))%16
                    c=color(1+(tile+3*(px%8)+5*(py%8))%255)
            elif name.startswith('bitmap'):
                mode=int(name[6]); page=int('page1' in name)
                gx=x-x%4 if 'mosaic' in name else x
                gy=y-y%3 if 'mosaic' in name else y
                if 'rotate' in name:
                    px=(7*256+192*gx+64*gy)//256
                    py=(13*256-64*gx+192*gy)//256
                else: px,py=gx,gy
                width,height=(160,128) if mode==5 else (240,160)
                if 0<=px<width and 0<=py<height:
                    ci=(px+3*py+page*13)%256
                    if mode!=4 or ci: c=color(ci)
            elif name.startswith('obj'):
                affine='affine' in name
                width=32 if affine else 16
                if 3<=x<3+width and 5<=y<5+width:
                    fx,fy=x-3,y-5
                    if 'mosaic' in name:
                        fx=x-x%4-3
                        fy=(y-5)-(y-5)%3
                    if affine:
                        tx=(192*(fx-16)+64*(fy-16))//256+8
                        ty=(-64*(fx-16)+192*(fy-16))//256+8
                    elif 'flip' in name: tx,ty=15-fx,15-fy
                    else: tx,ty=fx,fy
                    if 0<=tx<16 and 0<=ty<16:
                        c=color(1+(tx+3*ty)%(255 if '8bpp' in name else 15))
            if c is not None: buf[y*240+x]=rgb(c)
    return buf

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cases',nargs='+',default=CASES,choices=CASES)
    args=ap.parse_args()
    out=ROOT/'build/ppu-conformance'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S_%f')
    out.mkdir(parents=True)
    toolchain_root = Path(os.environ.get('KHCOM_TOOLCHAIN', ROOT.parent / 'toolchain'))
    toolchain = sorted(toolchain_root.glob('llvm-mingw-*'))[-1]
    exe=out/'ppu_conformance.exe'
    includes=[ROOT/'include']+[p for p in (ROOT/'include').iterdir() if p.is_dir() and p.name!='gba']
    cmd=[str(toolchain/'bin/i686-w64-mingw32-clang.exe'),'-O2','-g','-std=gnu11',
         '-funsigned-char','-fno-strict-aliasing','-fwrapv','-mno-ms-bitfields',
         '-DPLATFORM_PC','-DVERSION_EU','-I'+str(ROOT/'port/include')]
    for p in includes: cmd+=['-iquote',str(p)]
    cmd += [str(ROOT/'port/tools/ppu_conformance.c'),'-o',str(exe)]
    subprocess.run(cmd,check=True)
    subprocess.run([str(exe), 'rgb555-blend'], check=True)
    results=[]
    sheet=Image.new('RGB',(6*240,((len(args.cases)+5)//6)*180),'#222222')
    draw=ImageDraw.Draw(sheet)
    for i,name in enumerate(args.cases):
        raw=out/(name+'.bin')
        proc=subprocess.run([str(exe),name,str(raw)],check=True,timeout=20,capture_output=True,text=True)
        scenario=name.removeprefix('fast_')
        if scenario.startswith('semantic'):
            actual=json.loads(proc.stdout)
            want=[0,0,0]
            if not any(s in name for s in ('cover_','window','stale','clear')):
                want[2 if 'screen' in name else 1 if 'hud' in name else 0]=1
            count=int(actual['counts']!=want)
            if 'explicit' in name:
                count += int(actual['cap'] != 5 or actual['base'] != 6)
            results.append({'case':name,'pixels_compared':0,'mismatches':count,
                            'glyph_counts':actual['counts'],'expected_counts':want,
                            'cap':actual.get('cap'),'baseline':actual.get('base')})
            sx,sy=(i%6)*240,(i//6)*180
            draw.text((sx+2,sy+2),name,fill='red' if count else 'white')
            draw.text((sx+2,sy+40),str(actual['counts'])+' expected '+str(want),fill='white')
            print(f'{name}: {count} semantic mismatch',flush=True)
            continue
        actual=array('I'); actual.frombytes(raw.read_bytes())
        assert len(actual)==38400
        want=expected(scenario)
        mismatch=[p for p,(a,b) in enumerate(zip(actual,want)) if a!=b]
        count=len(mismatch)
        result={'case':name,'pixels_compared':38400,'mismatches':count}
        if count:
            yy,xx=divmod(mismatch[0],240)
            result.update(first=[xx,yy],actual=hex(actual[mismatch[0]]),expected=hex(want[mismatch[0]]))
        results.append(result)
        pic=Image.frombytes('RGBA',(240,160),actual.tobytes(),'raw','BGRA').convert('RGB')
        pic.save(out/(name+'.png'))
        if count:
            diff=Image.new('RGB',(240,160))
            for p in mismatch: diff.putpixel((p%240,p//240),(255,0,255))
            diff.save(out/(name+'_diff.png'))
        sx,sy=(i%6)*240,(i//6)*180
        sheet.paste(pic,(sx,sy+20))
        draw.text((sx+2,sy+2),name,fill='red' if count else 'white')
        print(f'{name}: {count} mismatch',flush=True)
    sheet.save(out/'contact.png')
    report={'reference':'GBATEK GBA LCD video controller; independent mathematical oracle',
            'renderer':'port/src/port_ppu.c included directly, ROM-free memory fixtures',
            'cases':results,'passed':sum(r['mismatches']==0 for r in results),'total':len(results)}
    (out/'report.json').write_text(json.dumps(report,indent=2))
    print(out,flush=True)
    return int(report['passed']!=report['total'])

if __name__=='__main__': raise SystemExit(main())
