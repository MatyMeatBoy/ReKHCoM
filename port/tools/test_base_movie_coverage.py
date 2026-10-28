"""Semantic movie3 coverage, actual CLI copy and full-profile de-duplication.

Only generated MP4-box fixtures; no decoder, proprietary assets or game runtime.
"""
from pathlib import Path
import json,os,struct,subprocess,sys,tempfile,unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))
import khhd_import
import khhd_posters

class BaseMovieCoverage(unittest.TestCase):
 def test_actual_legacy_full_collector_is_unique(self):
  source=(ROOT/'port/decompui/hd_import.inc').read_text(encoding='utf-8')
  bases=source[source.index('static const char* const kBaseMovies[]'):].split(';',1)[0]+';'
  start=source.index('    std::vector<fs::path> list;')
  block=source[start:source.index('\n    if (music)',start)]
  c=r'''
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <cassert>
namespace fs=std::filesystem;
static struct {void Plan(int,int){}}sJob;
static int Units(const fs::path&){return 1;}
static int MoviePart(const fs::path&){return 0;}
static fs::path MovieFolder(const fs::path&p){return p;}
BASES
static std::vector<fs::path> Collect(fs::path source,bool movies,bool scenes){std::error_code ec;
BLOCK
return list;}
int main(int argc,char**argv){
 assert(argc==2);fs::path root=argv[1];fs::create_directories(root);
 for(const char*name:{"ROP","RSE","RRE","zz_event_123","zz_event_151","zz_event_621","zz_event_631","zz_event_001","noise"})std::ofstream(root/(std::string(name)+".mp4"))<<"owned collector fixture";
 auto basic=Collect(root,true,false);assert(basic.size()==7);
 auto scenes=Collect(root,false,true);assert(scenes.size()==5);
 auto all=Collect(root,true,true);assert(all.size()==8);
 std::sort(all.begin(),all.end());assert(std::adjacent_find(all.begin(),all.end())==all.end());
}
'''.replace('BASES',bases).replace('BLOCK',block)
  compiler=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang++.exe'
  with tempfile.TemporaryDirectory(prefix='legacy movie collector ') as td:
   p=Path(td);(p/'test.cpp').write_text(c,encoding='utf-8')
   subprocess.run([str(compiler),'-std=c++17','-O2','-static',str(p/'test.cpp'),'-o',str(p/'test.exe')],check=True,capture_output=True)
   subprocess.run([str(p/'test.exe'),str(p/'fixtures')],check=True,timeout=10)

 def test_basic_button_copy_and_translations(self):
  keys=['Just the basics: movies, music and HUD (~0.6 GB)','Just the basics: movies, music and HUD, ~0.6 GB']
  for rel,key in zip(('hd_import.inc','rom_folder.inc'),keys):
   self.assertIn(key,(ROOT/'port/decompui'/rel).read_text(encoding='utf-8'))
  pairs=json.loads((ROOT/'port/decompui/i18n/strings.json').read_text(encoding='utf-8'))
  table=(ROOT/'port/decompui/i18n_table.inc').read_text(encoding='utf-8')
  for key in keys:
   self.assertIn('películas',next(row['es'] for row in pairs if row['en']==key))
   self.assertEqual(table.count('"'+key+'"'),1)
   for lang in ('fr','de','it','ja'):
    localized=json.loads((ROOT/f'port/decompui/i18n/{lang}.json').read_text(encoding='utf-8'))
    self.assertTrue(localized[key]);self.assertNotIn(key.replace('movies','intros'),localized)

 def test_game_movie3_in_both_basic_catalogs(self):
  game=(ROOT/'port/src/port_hdmovie.c').read_text(encoding='utf-8')
  self.assertRegex(game,r'case 3: \*startAt = 170; return "zz_event_123"')
  self.assertIn('zz_event_123',khhd_import.BASE_MOVIES)
  cpp=(ROOT/'port/decompui/hd_import.inc').read_text(encoding='utf-8')
  bases=cpp.split('kBaseMovies[] = {',1)[1].split('};',1)[0]
  self.assertIn('"zz_event_123"',bases)
  self.assertEqual(khhd_posters.poster_seconds(Path('zz_event_123.mp4')),(1,171))

 def test_cli_basic_and_full_keep_scene_bytes_and_deduplicate(self):
  # Independent expected set: deriving this from BASE_MOVIES would hide omissions.
  names=('ROP','RSE','RRE','zz_event_123','zz_event_151','zz_event_621','zz_event_631')
  box=lambda tag,payload=b'':struct.pack('>I4s',len(payload)+8,tag)+payload
  plain=box(b'ftyp',b'isom'+bytes(4)+b'isom')+box(b'moov')+box(b'mdat',bytes(range(256))*2)
  with tempfile.TemporaryDirectory(prefix='base movie coverage ') as td:
   root=Path(td);source=root/'source';movies=source/'STEAM/dt/KHCReSource/BIN/movie';movies.mkdir(parents=True)
   image=source/'Image/dt';image.mkdir(parents=True);(image/'Recom.hed').write_bytes(b'');(image/'Recom.pkg').write_bytes(b'')
   for index,name in enumerate(names):
    encoded=bytes(v^khhd_import.MOVIE_KEY[i%16] if i<256 else v for i,v in enumerate(plain)) if index%2 else plain
    (movies/(name+'.mp4')).write_bytes(encoded)
   original={p.name:p.read_bytes() for p in movies.iterdir()}
   for label,extra in [('basic',[]),('full',['--scenes'])]:
    data=root/label
    p=subprocess.run([sys.executable,str(ROOT/'port/tools/khhd_import.py'),str(source),str(data),'--no-music',*extra],env=dict(os.environ,PATH=''),capture_output=True,text=True,check=True,timeout=10)
    self.assertEqual(p.stdout.splitlines().count('movie zz_event_123'),1)
    self.assertEqual((data/'hd/scenes/zz_event_123.mp4').read_bytes(),plain)
    self.assertFalse((data/'hd/movies/zz_event_123.mp4').exists())
    self.assertEqual(len(list((data/'hd').rglob('*.mp4'))),7)
   self.assertEqual(original,{p.name:p.read_bytes() for p in movies.iterdir()})

if __name__=='__main__':unittest.main()
