"""Actual KHMenu CFF widths, approved outlines, metrics and generator replay.

    uv run --with fonttools python port/tools/test_font_kh_legible.py
"""
from pathlib import Path
import argparse,shutil,subprocess,sys,tempfile
from fontTools.ttLib import TTFont
from fontTools.pens.recordingPen import RecordingPen

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'port/assets/fonts/KHMenu.otf'

def snapshot(path):
    f=TTFont(path,recalcTimestamp=False)
    top=f['CFF '].cff.topDictIndex[0]
    shapes={};widths={}
    for name in f.getGlyphOrder():
        pen=RecordingPen();cs=top.CharStrings[name];cs.draw(pen)
        shapes[name]=pen.value;widths[name]=cs.width
    return dict(shapes=shapes,widths=widths,metrics=dict(f['hmtx'].metrics),
                cmap=f.getBestCmap(),tables={n:f.getTableData(n) for n in ('cmap','name','OS/2','GPOS','GDEF','post')})

def check_widths(s):
    bad={n:(s['metrics'][n][0],width) for n,width in s['widths'].items()
         if abs(s['metrics'][n][0]-width)>0.01}
    assert not bad,('CFF and hmtx disagree; browser and native layout diverge',bad)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference',type=Path,default=SOURCE)
    args=parser.parse_args()
    original=SOURCE.read_bytes();before=snapshot(args.reference)
    with tempfile.TemporaryDirectory(prefix='khmenu-cff-') as directory:
        target=Path(directory)/'KHMenu.otf';shutil.copyfile(SOURCE,target)
        for run in range(2):
            subprocess.run([sys.executable,str(ROOT/'port/tools/font_kh_legible.py'),str(target)],check=True)
            after=snapshot(target);check_widths(after)
            for field in ('shapes','metrics','cmap','tables'):
                assert after[field]==before[field],('Changed approved font data',run,field)
    check_widths(snapshot(SOURCE))
    assert SOURCE.read_bytes()==original,'Generator test changed the original font'
    print('PASS all CFF widths agree with hmtx; all outlines/metrics/cmap/positioning tables unchanged; replay stable')

if __name__=='__main__':main()
