"""Execute the production geometry C AST without compiling or invoking SDL.
Requires pycparser (test dependency). Actual compiler/runtime checks are separate.
"""
from pathlib import Path
import itertools,re,unittest
from pycparser import c_ast,c_parser

ROOT=Path(__file__).resolve().parents[2]

class Returned(Exception):
    def __init__(self,value):self.value=value

class Header:
    def __init__(self):
        text=(ROOT/'port/include/port/gba_border_layout.h').read_text()
        text=re.sub(r'/\*.*?\*/','',text,flags=re.S)
        text='\n'.join(line for line in text.splitlines() if not line.lstrip().startswith('#'))
        tree=c_parser.CParser().parse(text)
        self.functions={n.decl.name:n for n in tree.ext if isinstance(n,c_ast.FuncDef)}
    def call(self,name,*args):
        f=self.functions[name]
        env={n.name:v for n,v in zip(f.decl.type.args.params,args)}
        try:self.run(f.body,env)
        except Returned as r:return r.value
    def expr(self,n,e):
        if n is None:return None
        if isinstance(n,c_ast.ID):return e[n.name]
        if isinstance(n,c_ast.Constant):
            value=re.sub(r'[fFuUlL]+$','',n.value)
            return float(value) if n.type in ('float','double') else int(value,0)
        if isinstance(n,c_ast.StructRef):return self.expr(n.name,e)[n.field.name]
        if isinstance(n,c_ast.InitList):return dict(zip(('x','y','w','h'),[self.expr(x,e) for x in n.exprs]))
        if isinstance(n,c_ast.Cast):
            v=self.expr(n.expr,e);typ=n.to_type.type.type.names[0]
            return int(v) if typ=='int' else float(v)
        if isinstance(n,c_ast.UnaryOp):
            v=self.expr(n.expr,e)
            return {'!':lambda:int(not v),'-':lambda:-v,'+':lambda:+v,'&':lambda:v}[n.op]()
        if isinstance(n,c_ast.TernaryOp):return self.expr(n.iftrue if self.expr(n.cond,e) else n.iffalse,e)
        if isinstance(n,c_ast.FuncCall):return self.call(n.name.name,*[self.expr(x,e) for x in n.args.exprs])
        if isinstance(n,c_ast.BinaryOp):
            a=self.expr(n.left,e)
            if n.op=='&&':return int(bool(a) and bool(self.expr(n.right,e)))
            if n.op=='||':return int(bool(a) or bool(self.expr(n.right,e)))
            b=self.expr(n.right,e)
            return {'+':lambda:a+b,'-':lambda:a-b,'*':lambda:a*b,
                    '/':lambda:int(a/b) if isinstance(a,int) and isinstance(b,int) else a/b,
                    '%':lambda:a%b,'<':lambda:int(a<b),'>':lambda:int(a>b),
                    '<=':lambda:int(a<=b),'>=':lambda:int(a>=b),'==':lambda:int(a==b),'!=':lambda:int(a!=b)}[n.op]()
        raise AssertionError(type(n))
    def run(self,n,e):
        if n is None:return
        if isinstance(n,c_ast.Compound):
            for item in n.block_items or []:self.run(item,e)
        elif isinstance(n,c_ast.Decl):e[n.name]=self.expr(n.init,e)
        elif isinstance(n,c_ast.Return):raise Returned(self.expr(n.expr,e))
        elif isinstance(n,c_ast.If):self.run(n.iftrue if self.expr(n.cond,e) else n.iffalse,e)
        elif isinstance(n,c_ast.Assignment):
            v=self.expr(n.rvalue,e)
            if isinstance(n.lvalue,c_ast.StructRef):self.expr(n.lvalue.name,e)[n.lvalue.field.name]=v
            else:e[n.lvalue.name]=v
        else:raise AssertionError(type(n))

class BorderGeometry(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.c=Header()
    def test_off_identity_and_invalid(self):
        for w,h in ((1920,1080),(1080,1920),(5120,1440),(240,160),(1,1)):
            self.assertEqual(self.c.call('PortGbaBorderViewport',w,h,0),dict(x=0,y=0,w=w,h=h))
        for w,h in ((0,0),(-1,900),(900,0)):
            self.assertEqual(self.c.call('PortGbaBorderViewport',w,h,1),dict(x=0,y=0,w=0,h=0))
    def test_ratios_center_tiny_and_large(self):
        sizes=((1920,1080),(640,480),(3440,1440),(5120,1440),(1080,1920),(1620,1080),(1,1),(2,3),(17,31),(2147483647,2147483647))
        for w,h in sizes:
            r=self.c.call('PortGbaBorderViewport',w,h,1)
            self.assertTrue(0<r['w']<=w and 0<r['h']<=h)
            self.assertLessEqual(abs(w-r['w']-2*r['x']),1)
            self.assertLessEqual(abs(h-r['h']-2*r['y']),1)
            self.assertLess(abs(r['w']/w-r['h']/h),max(1/w,1/h)+1e-9)
    def test_legacy_exact(self):
        for scale in (1,2,5,10):
            ap=dict(x=72.*scale,y=28.*scale,w=240.*scale,h=160.*scale)
            self.assertTrue(self.c.call('PortGbaBorderLegacy',384*scale,216*scale,ap))
            self.assertFalse(self.c.call('PortGbaBorderLegacy',385*scale,216*scale,ap))
    def test_uniform_native_decoration_fits_available_bands(self):
        for w,h in ((1920,1080),(640,480),(3440,1440),(5120,1440),(1080,1920),(1620,1080),(240,160),(17,31)):
            ap={k:float(v) for k,v in self.c.call('PortGbaBorderViewport',w,h,1).items()}
            s=self.c.call('PortGbaBorderDecorationScale',w,h,ap)
            self.assertGreaterEqual(s,0)
            self.assertLessEqual(72*s,ap['x']+.001)
            self.assertLessEqual(72*s,w-ap['x']-ap['w']+.001)
            self.assertLessEqual(28*s,ap['y']+.001)
            self.assertLessEqual(28*s,h-ap['y']-ap['h']+.001)
            self.assertLessEqual(240*s,ap['w']+.001)
            self.assertLessEqual(160*s,ap['h']+.001)
    def test_border_option_independent(self):
        ui=(ROOT/'port/src/port_ui.c').read_text(encoding='utf8')
        self.assertTrue('case ITEM_GBA_BORDER: return 1;' in ui,'Border must be visible independently of view')
        # Selecting the existing setting must not assign aspect/zoom or HD flags.
        body=re.search(r'case ITEM_GBA_BORDER:\s*gPortConfig.gbaBorder =.*?break;',ui,re.S)[0]
        self.assertNotRegex(body,r'gPortConfig\.(?:aspect|zoom|hd\w+)\s*=')

if __name__=='__main__':unittest.main()
