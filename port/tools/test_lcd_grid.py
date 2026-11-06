"""LCD source wiring and optional actual-C coverage/cache conformance.
No ROM, decoder, assets or game runtime. Compiler invocation requires --cc.
"""
from pathlib import Path
import argparse,hashlib,json,os,re,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
def source_check():
 main=(ROOT/'port/src/port_main.c').read_text(encoding='utf8')
 config=(ROOT/'port/src/port_config.c').read_text(encoding='utf8')
 ui=(ROOT/'port/src/port_ui.c').read_text(encoding='utf8')
 header=(ROOT/'port/include/port/port.h').read_text(encoding='utf8')
 assert re.search(r'int hideTitleHint;[^\n]*\n\s*int lcdGrid;',header),'append after existing setting'
 assert '{ "lcd_grid", &gPortConfig.lcdGrid, 0, 100 }' in config,'persisted intensity'
 assert 'ITEM_LCD_GRID' in ui and 'LCD pixel grid' in ui,'visible independent option'
 assert main.index('DrawLcdGrid(outW, outH,')>main.index('PortUiJournalFooter(sRenderer'),'footer included'
 assert main.index('DrawLcdGrid(outW, outH,')<main.index('/* state / slot notices:'),'notices remain clear'
 assert main.count('lcdSourceW = src.w;')==1,'cropped/narrow source dimensions'
 assert '&sLcdGridXTex, &sLcdGridYTex' in main,'renderer owns both caches'
 # Exercise the actual guest patcher: inserting this row must not break MMPX.
 sys.path.insert(0,str(ROOT/'port/ps5'))
 import build_ps5
 with tempfile.TemporaryDirectory(prefix='lcd guest patch ') as td:
  for dualsense in (False,True):
   patched=build_ps5.patched_source(ROOT/'port/src/port_ui.c',Path(td)/str(dualsense),dualsense=dualsense).read_text(encoding='utf8')
   assert 'ITEM_MMPX,\n    ITEM_SCANLINES,\n    ITEM_LCD_GRID,' in patched
   assert 'LCD pixel grid' in patched
 return main

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--cc');ap.add_argument('--output',type=Path);args=ap.parse_args()
 s=source_check()
 if not args.cc:print('PASS LCD option/source ownership wiring (C conformance not run)');return
 assert args.output,'--output required with --cc'
 args.output.mkdir(parents=True,exist_ok=True);exe=args.output/'lcd_grid.exe'
 cmd=[args.cc,'-std=gnu11','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'port/include'),str(ROOT/'port/tools/lcd_grid_conformance.c'),'-o',str(exe)]
 flags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0)|getattr(subprocess,'CREATE_NO_WINDOW',0)
 c=subprocess.run(cmd,capture_output=True,creationflags=flags);(args.output/'compile.log').write_bytes(c.stdout+c.stderr);assert c.returncode==0,c.stderr.decode(errors='replace')
 p=subprocess.run([str(exe)],capture_output=True,creationflags=flags);(args.output/'stdout.log').write_bytes(p.stdout+p.stderr)
 result={'compile_exit':c.returncode,'exit':p.returncode,'stdout':p.stdout.decode(errors='replace'),'source_sha256':{n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ('port/include/port/lcd_grid.h','port/src/port_lcd_grid.inc','port/src/port_main.c','port/tools/lcd_grid_conformance.c')},'scope':'Actual LCD production helper/cache code with mock SDL; no realGPU/runtime'}
 (args.output/'RESULT.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));assert p.returncode==0
if __name__=='__main__':main()
