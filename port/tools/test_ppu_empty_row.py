"""Interpret the real empty-row C branch; no compiler or game is invoked."""
from pathlib import Path
import itertools,re,unittest
from pycparser import c_parser,c_ast as a

ROOT=Path(__file__).resolve().parents[2]

def balanced(s,start,opening='{',closing='}'):
    p=s.index(opening,start);q=p+1;depth=1
    while depth:depth+=(s[q]==opening)-(s[q]==closing);q+=1
    return q

def condition(source,needle):
    start=source.index(needle);p=source.index('(',start);q=balanced(source,start,'(',')')
    return c_parser.CParser().parse('void f(){if '+source[p:q]+' {}}').ext[0].body.block_items[0].cond

class Model:
    def __init__(self,source,header):
        h=re.sub(r'/\*.*?\*/','',header,flags=re.S)
        h='\n'.join(l for l in h.splitlines()if not l.startswith('#'))
        self.helper=c_parser.CParser().parse('typedef unsigned char uint8_t;'+h).ext[-1].body.block_items[0].expr
        start=source.index('if (PortPpu4bppRowEmpty(pixels))');end=balanced(source,start)
        end=balanced(source,source.index('else',end))
        self.body=c_parser.CParser().parse('void f(){'+source[start:end]+'}').ext[0].body
        self.decodes=0;self.empty=0

    def value(self,n,e):
        if isinstance(n,a.Constant):return int(n.value.rstrip('uUlL'),0)
        if isinstance(n,a.ID):return e[n.name]
        if isinstance(n,a.ArrayRef):return self.value(n.name,e)[self.value(n.subscript,e)]
        if isinstance(n,a.StructRef):return self.value(n.name,e)[n.field.name]
        if isinstance(n,a.TernaryOp):return self.value(n.iftrue if self.value(n.cond,e)else n.iffalse,e)
        if isinstance(n,a.FuncCall):
            assert n.name.name=='PortPpu4bppRowEmpty'
            result=self.value(self.helper,dict(pixels=e['pixels']));self.empty+=bool(result);return result
        if isinstance(n,a.UnaryOp):
            if n.op=='!':return not self.value(n.expr,e)
            if n.op=='-':return -self.value(n.expr,e)
            if n.op=='p++':v=self.value(n.expr,e);e[n.expr.name]=v+1;return v
        if isinstance(n,a.ExprList):
            for expr in n.exprs:v=self.value(expr,e)if not isinstance(expr,a.Assignment)else self.run(expr,e)
            return v
        if isinstance(n,a.BinaryOp):
            x=self.value(n.left,e)
            if n.op=='||':return x or self.value(n.right,e)
            if n.op=='&&':return x and self.value(n.right,e)
            y=self.value(n.right,e)
            return {'|':lambda:x|y,'&':lambda:x&y,'>>':lambda:x>>y,'+':lambda:x+y,
                    '-':lambda:x-y,'*':lambda:x*y,'/':lambda:int(x/y),'<':lambda:x<y,
                    '>':lambda:x>y,'>=':lambda:x>=y,'<=':lambda:x<=y,'==':lambda:x==y}[n.op]()
        raise AssertionError(type(n).__name__)

    def run(self,n,e):
        if isinstance(n,a.Compound):
            for s in n.block_items or []:self.run(s,e)
        elif isinstance(n,a.If):self.run(n.iftrue if self.value(n.cond,e)else n.iffalse,e)
        elif n is None:return
        elif isinstance(n,a.For):
            self.run(n.init,e)
            while self.value(n.cond,e):self.run(n.stmt,e);self.value(n.next,e)
        elif isinstance(n,a.Decl):
            e[n.name]=self.value(n.init,e)
            if n.name=='c':self.decodes+=1
        elif isinstance(n,a.Assignment):
            v=self.value(n.rvalue,e)
            if isinstance(n.lvalue,a.ID):
                k=n.lvalue.name;e[k]=v if n.op=='='else e[k]+v
            else:
                assert n.op=='=';self.value(n.lvalue.name,e)[self.value(n.lvalue.subscript,e)]=v
        else:self.value(n,e)

    def draw(self,pixels,first,step,count,palette=None):
        self.decodes=self.empty=0
        e=dict(pixels=pixels,xx=first,step=step,run=count,j=-1,x=3,
               colors=palette or [0xf000+i*503 for i in range(16)],TRANSPARENT=0x8000,out=[0x1234]*(count+6))
        self.run(self.body,e);return e['out'],self.decodes,self.empty

class EmptyRowTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source=(ROOT/'port/src/port_ppu.c').read_text()
        assert 'if (PortPpu4bppRowEmpty(pixels))' in cls.source,'missing empty4bpp-row fastpath'
        cls.header=(ROOT/'port/include/port/port_ppu_tile_row.h').read_text()
        cls.model=Model(cls.source,cls.header)

    def test_actual_ast_patterns_flips_and_all_clipped_runs(self):
        patterns={bytes(4),bytes([255]*4)}
        for pos,value in itertools.product(range(4),range(256)):
            row=bytearray(4);row[pos]=value;patterns.add(bytes(row))
        for pixels in patterns:
            for flip,start in itertools.product((False,True),range(8)):
                first=7-start if flip else start;step=-1 if flip else 1
                for count in range(1,9-start):
                    got,decodes,empty=self.model.draw(pixels,first,step,count)
                    expected=[0x1234]*3
                    for j in range(count):
                        xx=first+j*step;c=pixels[xx>>1]>>((xx&1)*4)&15
                        expected.append((0xf000+c*503)&0x7fff if c else 0x8000)
                    expected+=[0x1234]*3
                    self.assertEqual(got,expected)
                    self.assertEqual(decodes,0 if not any(pixels)else count)
                    self.assertEqual(empty,int(not any(pixels)))

    def test_empty_keeps_transparency_even_if_palette_zero_is_opaque(self):
        self.assertEqual(self.model.draw(bytes(4),0,1,8,[65535]*16)[0][3:11],[0x8000]*8)

    def test_existing_world_font_and_bounds_guards(self):
        world=condition(self.source,'if ((outside || gx < 0 || gx >= GBA_SCREEN_W) && mosH == 1')
        font=condition(self.source,'if (bpp8 || !sHdText || base + off >= 0x10000 ||')
        bounds=condition(self.source,'if (base + off + (bpp8 ? 8 : 4) > 0x10000)')
        for outside,gx,mosh,screen,docked,valid,world_valid in itertools.product((0,1),(-1,0,239,240),(1,2),(0,1),(0,1),(0,1),(0,1)):
            e=dict(outside=outside,gx=gx,GBA_SCREEN_W=240,mosH=mosh,sScreenBg=[screen],bg=0,
                   sBgDockedLatched=[docked],sBgLatch=[dict(valid=valid)],sBgWorldLatched=[dict(valid=world_valid)])
            self.assertEqual(bool(self.model.value(world,e)),bool((outside or gx<0 or gx>=240)and mosh==1 and not screen and not docked and(valid or world_valid)))
        for bpp8,hd,base,off,tag in itertools.product((0,1),(0,1),(0,16384,32768,49152),(0,16380,16381,65532,65533,65536),(0,32,33)):
            tags=[dict(code=tag)]*4097;e=dict(bpp8=bpp8,sHdText=hd,base=base,off=off,sBgTextTiles=tags)
            eligible=bool(self.model.value(font,e));over=bool(self.model.value(bounds,e))
            self.assertEqual(eligible,bool(bpp8 or not hd or base+off>=65536 or tag<=32))
            self.assertEqual(over,base+off+(8 if bpp8 else 4)>65536)
        branch=self.source.index('if (PortPpu4bppRowEmpty(pixels))')
        for needle in ('memset(sInWorld + x, 1, run);','if (bpp8)'):
            location=self.source.rfind(needle,0,branch)
            self.assertGreaterEqual(location,0);self.assertLess(location,branch)

if __name__=='__main__':unittest.main()
