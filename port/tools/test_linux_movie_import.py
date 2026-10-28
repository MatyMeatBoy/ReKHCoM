"""Generated MP4 box fixtures through the real CLI producer and native transaction.

No game content, decoder, ffmpeg, network, VM or game execution is needed.
"""
from pathlib import Path
import importlib.util,os,struct,subprocess,sys,tempfile,unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))
import khhd_import
spec=importlib.util.spec_from_file_location('launcher_movies',ROOT/'port/linux/launcher/rekhcom_launcher.py')
launcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(launcher)

def box(kind,body=b''):return struct.pack('>I4s',len(body)+8,kind)+body
PLAIN=box(b'ftyp',b'isom'+bytes(4)+b'isom')+box(b'moov')+box(b'mdat',bytes(300))
def encode(data):return bytes(b^khhd_import.MOVIE_KEY[i%16] if i<256 else b for i,b in enumerate(data))

class MovieImport(unittest.TestCase):
 def test_real_cli_plain_xor_and_mixed_sources(self):
  for label in ('plain','xor','mixed'):
   with self.subTest(label=label),tempfile.TemporaryDirectory(prefix='movie import spaces ') as t:
    root=Path(t);source=root/'owned source';movies=source/'STEAM/dt/KHCReSource/BIN/movie';movies.mkdir(parents=True)
    image=source/'Image/dt';image.mkdir(parents=True)
    (image/'Recom.hed').write_bytes(b'');(image/'Recom.pkg').write_bytes(b'')
    for i,name in enumerate(khhd_import.BASE_MOVIES):
     (movies/(name+'.mp4')).write_bytes(encode(PLAIN) if label=='xor' or(label=='mixed' and i%2) else PLAIN)
    game=root/'game';(game/'hd/music').mkdir(parents=True)
    old=game/'hd/music/old.ogg';old.write_bytes(b'owned old HD');save=game/'kh_com.sav';save.write_bytes(b'owned save')
    env=dict(os.environ,PATH='') # Optional poster executable absent; no codec dependency.
    def producer(selected,stage):
     subprocess.run([sys.executable,str(ROOT/'port/tools/khhd_import.py'),str(selected),str(stage),'--no-music'],
                    env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True,timeout=10)
    backup=launcher.transaction(game,source,producer)
    for name in khhd_import.BASE_MOVIES:
     folder='scenes' if name.startswith('zz_event_') else 'movies'
     self.assertEqual((game/'hd'/folder/(name+'.mp4')).read_bytes(),PLAIN)
    self.assertEqual(old.read_bytes(),b'owned old HD');self.assertEqual(save.read_bytes(),b'owned save')
    self.assertEqual((backup/'music/old.ogg').read_bytes(),b'owned old HD')

 def test_box_limit_boundary(self):
  with tempfile.TemporaryDirectory() as t:
   path=Path(t)/'movie.mp4'
   for count,accepted in ((4095,True),(4096,True),(4097,False)):
    with self.subTest(count=count):
     path.write_bytes(box(b'moov')+box(b'mdat')+box(b'free')*(count-2))
     self.assertEqual(launcher.mp4(path),accepted)

 def test_rejected_outputs_preserve_all_previous_files(self):
  invalid=[PLAIN[:-1],box(b'ftyp',b'isom')+struct.pack('>I4s',0xffffffff,b'mdat'),
           box(b'ftyp',b'isom')+struct.pack('>I4sQ',1,b'mdat',1<<40),
           box(b'ftyp',b'isom')+box(b'moov'),b'wrong movie format']
  for encoded in (False,True):
   for i,data in enumerate(invalid):
    with self.subTest(encoded=encoded,case=i),tempfile.TemporaryDirectory() as t:
     root=Path(t);source=root/'source';source.mkdir();src=source/'ROP.mp4';src.write_bytes(encode(data) if encoded else data)
     game=root/'game';(game/'hd/movies').mkdir(parents=True);(game/'hd/music').mkdir()
     (game/'hd/movies/ROP.mp4').write_bytes(PLAIN);(game/'hd/music/old.ogg').write_bytes(b'owned previous')
     (game/'kh_com.sav').write_bytes(b'owned save')
     before={p.relative_to(game):p.read_bytes() for p in game.rglob('*') if p.is_file()}
     def producer(selected,stage):
      out=stage/'hd/movies/ROP.mp4';khhd_import.copy_movie(src,out)
     with self.assertRaises(launcher.ImportErrorSafe):launcher.transaction(game,source,producer)
     self.assertEqual({p.relative_to(game):p.read_bytes() for p in game.rglob('*') if p.is_file()},before)
     self.assertFalse(list(game.glob('.hd-import-*')));self.assertFalse((game/'.rekhcom-hd-import.lock').exists())

if __name__=='__main__':unittest.main()
