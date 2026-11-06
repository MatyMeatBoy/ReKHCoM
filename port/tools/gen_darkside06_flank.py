"""Reproduce the narrow pending punch-flank shadow from a readonly local ROM.

Only new palette indices are emitted. No approved pose or ROM is written.
This generates this proposal's own header, not imports or game assets.
"""
from pathlib import Path
from functools import lru_cache
import argparse,hashlib,json,re,struct

ROOT=Path(__file__).resolve().parents[2]
APPROVED_SHA='fce1799e73d7d25ba2196e62367672f6bc0af295a32bc3669acc06bc95e3a64e'
CONTROL=((122,1),(128,3),(133,2),(136,0),(140,0),(141,2),(147,6),(154,8),(160,7),(168,4),(176,1),(180,0))

def width(y):
    for (a,v),(b,w) in zip(CONTROL,CONTROL[1:]):
        if a<=y<=b:
            t=(y-a)/(b-a);t=t*t*(3-2*t)
            return v+(w-v)*t
    return 0

def noise(x,y):
    n=((x*0x1f123bb5)^(y*0x5f356495))&0xffffffff
    n^=n>>16;n=n*0x7feb352d&0xffffffff;n^=n>>15
    return n&255

def build_flank(native,approved):
    out={}
    for y in range(122,180):
        w=width(y)
        for d in range(8):
            if d>=w:continue
            # A solid connected interior, with only one fractional edge pixel.
            coverage=min(255,max(0,int((w-d)*255)))
            if noise(256+d,y)>=coverage:continue
            c=native.get((255,y),0)
            if not c:continue
            # Keep the side behind the arm in shadow, without a lit lower lobe.
            value=max(1,int(min(c,4)-(d/max(w,1))*2+.5))
            if (256+d,y) not in approved:out[256+d,y]=value
    return out

def generate(rom_path,symbol_path):
    raw=rom_path.read_bytes();symbols=symbol_path.read_bytes()
    sy={n:int(a,16)for n,a in re.findall(r'\.set _(\w+), (0x[0-9a-fA-F]+)',symbols.decode())}
    addresses=sorted(set(sy.values()))
    def data(name):
        a=sy[name];end=addresses[addresses.index(a)+1]
        if not 0<=a-0x8000000<end-0x8000000<=len(raw):raise ValueError('ROM symbol outside supplied cartridge')
        return raw[a-0x8000000:end-0x8000000]
    source=(ROOT/'src/bos/bos_dsd_main.c').read_text()
    maps=[s.split(',')[2].strip()for s in re.findall(r'\{([^}]*)\}',re.search(r'gBosDsdFrameMaps\[47\]\[4\] = \{(.*?)\};',source,re.S)[1])]
    tiles=[x.strip()for x in re.search(r'gBosDsdFrameTiles\[47\] = \{(.*?)\};',source,re.S)[1].split(',')if x.strip()]
    @lru_cache(None)
    def frame(slot):
        m,t=data(maps[slot]),data(tiles[slot]);out={}
        if len(m)<2048:raise ValueError('Incomplete native frame map')
        for y in range(256):
            for x in range(256):
                e=struct.unpack_from('<H',m,(y//8*32+x//8)*2)[0]
                xx=7-x%8 if e&1024 else x%8;yy=7-y%8 if e&2048 else y%8
                off=(e&1023)*32+yy*4+xx//2
                if off>=len(t):raise ValueError('Native frame references unloaded tile')
                n=(t[off]>>(4*(xx%2)))&15
                if n:out[x,y]=n
        return out
    approved_text=(ROOT/'port/include/port/darkside_body_punch.inc').read_text()
    if hashlib.sha256(approved_text.encode()).hexdigest()!=APPROVED_SHA:raise ValueError('Approved punch changed; review before generating')
    approved={}
    for y,row in enumerate(re.findall(r'\{\s*(0x[^}]+)\}',approved_text)):
        for j,b in enumerate(re.findall(r'0x([0-9a-fA-F]+)',row)):
            byte=int(b,16)
            for x,n in ((232+j*2,byte&15),(233+j*2,byte>>4)):
                if n:approved[x,y+42]=n
    native=frame(13);added=build_flank(native,approved);slots=[]
    for slot in (13,*range(21,29)):
        dy=0 if slot==13 else min(slot-20,6)*8;current=frame(slot)
        if any(current.get((255,y+dy))!=native.get((255,y)) for y in range(122,186)):raise ValueError('Native seam changed')
        merged={**{(x,y+dy):v for(x,y),v in added.items()},**approved,**current}
        assert all(merged[p]==v for p,v in current.items())
        assert all(merged[p]==v for p,v in approved.items()if p not in current)
        slots.append(dict(slot=slot,offset=dy,native_preserved=True,approved_preserved=True))
    assert len(added)==211 and all(x>=256 and y<180 and 1<=v<=4 for(x,y),v in added.items())
    for y in range(122,180):
        xs=sorted(x for x,yy in added if yy==y)
        if xs:assert xs==list(range(256,max(xs)+1))
    lines=['/* Pending narrow punch-flank shadow: added pixels only, live palette indices1..4.',
           ' * Reproduce with gen_darkside06_flank.py using the local readonly ROM. No original pixels. */',
           '#define DARKSIDE_FLANK_X 256','#define DARKSIDE_FLANK_TOP 122',
           '#define DARKSIDE_FLANK_WIDTH 8','#define DARKSIDE_FLANK_ROWS 58',
           'static const unsigned char kDarksideFlank[58][4] = {']
    for y in range(122,180):lines.append('    { '+', '.join('0x%02x'%(added.get((x,y),0)|(added.get((x+1,y),0)<<4))for x in range(256,264,2))+' },')
    lines.append('};')
    return '\n'.join(lines)+'\n',dict(rom_sha256=hashlib.sha256(raw).hexdigest(),symbols_sha256=hashlib.sha256(symbols).hexdigest(),approved_sha256=APPROVED_SHA,added_pixels=len(added),approved_arm_pixels=len(approved),max_width=8,lower_taper_unchanged=True,rows_contiguous=True,slots=slots)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('rom','symbols','output'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--report',type=Path);a=p.parse_args()
    allowed=(ROOT/'port/include/port/darkside_body_punch_flank.inc').resolve()
    if a.output.resolve()!=allowed or (a.report and a.report.resolve() in (a.rom.resolve(),a.symbols.resolve(),allowed)):p.error('Output must be this proposal header and report must be separate')
    text,proof=generate(a.rom,a.symbols);a.output.write_text(text,encoding='utf8',newline='\n')
    if a.report:a.report.write_text(json.dumps(proof,indent=2))
    print(json.dumps(proof))

if __name__=='__main__':main()
