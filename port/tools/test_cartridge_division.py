"""Bounded Thumb interpreter for the player's original EU division routines.

No ROM bytes are shipped. Nonzero arithmetic controls validate the instruction
subset before checking zero-divisor results of all four cartridge routines.
"""
import argparse, hashlib, struct

MASK=0xffffffff
def signed(n):return n if n<0x80000000 else n-0x100000000
def extend(n,bits):return n-(1<<bits) if n&(1<<(bits-1)) else n

def execute(rom,entry,a,b):
    r=[0]*16;r[0]=a&MASK;r[1]=b&MASK;r[13]=0x03007f00;r[14]=0xfffffffe
    pc=entry;mem={};N=Z=C=V=False;steps=0
    def nz(value):return bool(value&0x80000000),value==0
    def arithmetic(x,y,sub):
        raw=x-y if sub else x+y;v=raw&MASK
        overflow=bool(((x^y) if sub else ~(x^y))&(x^v)&0x80000000)
        return v,x>=y if sub else raw>MASK,overflow
    while pc!=0xfffffffe:
        assert 0x11c10c<=pc<0x11c3ac,hex(pc)
        steps+=1;assert steps<10000
        h=struct.unpack_from('<H',rom,pc)[0];nextpc=pc+2
        if h&0xf800==0xf000:
            low=struct.unpack_from('<H',rom,pc+2)[0];assert low&0xf800==0xf800
            r[14]=(pc+4)|1;nextpc=pc+4+(extend(h&0x7ff,11)<<12)+((low&0x7ff)<<1)
        elif h&0xf000==0xd000:
            cond=(h>>8)&15
            take={0:Z,1:not Z,2:C,3:not C,4:N,5:not N,6:V,7:not V,8:C and not Z,9:not C or Z,10:N==V,11:N!=V,12:not Z and N==V,13:Z or N!=V}[cond]
            if take:nextpc=pc+4+extend(h&255,8)*2
        elif h&0xf800==0xe000:nextpc=pc+4+extend(h&0x7ff,11)*2
        elif h&0xfe00 in (0xb400,0xbc00):
            pop=bool(h&0x0800);indices=[i for i in range(8) if h&(1<<i)]
            if h&0x100:indices.append(15 if pop else 14)
            if pop:
                for i in indices:
                    value=mem[r[13]];r[13]+=4
                    if i==15:nextpc=value&~1
                    else:r[i]=value
            else:
                r[13]-=len(indices)*4
                for i,index in enumerate(indices):mem[r[13]+4*i]=r[index]
        elif h&0xff00==0x4600:
            dest=(h&7)|((h>>4)&8);value=r[(h>>3)&15]
            if dest==15:nextpc=value&~1
            else:r[dest]=value
        elif h&0xfc00==0x4000:
            op=(h>>6)&15;d=h&7;s=(h>>3)&7;x=r[d];y=r[s]
            if op in (9,10):
                value,C,V=arithmetic(0 if op==9 else x,y,True)
                if op==9:r[d]=value
            elif op==7:
                shift=y&255;amount=shift%32
                value=((x>>amount)|(x<<(32-amount)))&MASK if amount else x
                if shift:C=bool(value&0x80000000)
                r[d]=value
            else:
                value={0:lambda:x&y,1:lambda:x^y,8:lambda:x&y,12:lambda:x|y}[op]()
                if op!=8:r[d]=value
            N,Z=nz(value)
        elif h&0xf800==0x1800:
            d=h&7;s=(h>>3)&7;y=(h>>6)&7
            if not h&0x400:y=r[y]
            r[d],C,V=arithmetic(r[s],y,bool(h&0x200));N,Z=nz(r[d])
        elif h&0xe000==0:
            op=(h>>11)&3;shift=(h>>6)&31;s=(h>>3)&7;d=h&7;x=r[s]
            if op==0:
                if shift:C=bool((x>>(32-shift))&1)
                value=(x<<shift)&MASK
            elif op==1:
                shift=shift or 32;C=bool((x>>(shift-1))&1);value=x>>shift
            else:raise AssertionError(('unsupported shift',hex(h)))
            r[d]=value;N,Z=nz(value)
        elif h&0xe000==0x2000:
            op=(h>>11)&3;d=(h>>8)&7;y=h&255
            if op==0:value=y;r[d]=value
            else:
                value,C,V=arithmetic(r[d],y,op in (1,3))
                if op!=1:r[d]=value
            N,Z=nz(value)
        else:raise AssertionError(('unsupported instruction',hex(pc),hex(h)))
        pc=nextpc
    return r[0],steps

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--rom',required=True);args=ap.parse_args()
    rom=open(args.rom,'rb').read()
    assert rom[0xac:0xb0]==b'B8CP','EU cartridge required'
    functions={'div':0x11c10c,'mod':0x11c1a4,'udiv':0x11c274,'umod':0x11c2ec}
    cases=0
    for name,entry in functions.items():
        for a in [0,1,7,0x7fffffff,0x80000000,0xffffffff]:
            for b in [1,2,3,7,0x7fffffff,0x80000000,0xffffffff,0]:
                sa,sb=(a,b) if name.startswith('u') else (signed(a),signed(b))
                q=0 if not b else (abs(sa)//abs(sb))*(-1 if (sa<0)!=(sb<0) else 1)
                expected=0 if not b else sa-q*sb if 'mod' in name else q
                got,steps=execute(rom,entry,a,b)
                assert got==expected&MASK,(name,a,b,hex(got),hex(expected&MASK),steps)
                cases+=1
    print('PASS',cases,'original Thumb signed/unsigned quotient/remainder controls including zero and overflow; ROM SHA256',hashlib.sha256(rom).hexdigest())

if __name__=='__main__':main()
