"""Real temporary-file transactions only; no game, source assets or dialog execution."""
from pathlib import Path
import importlib.util
import tempfile
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('native_import',ROOT/'port/linux/launcher/rekhcom_launcher.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
MOVIE=b'\0\0\0\x08moov\0\0\0\x08mdat'
def put(path,data):path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
class TransactionTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name)
  self.game=self.root/'game';self.game.mkdir();self.source=self.root/'source';put(self.source/'movies/ROP.mp4',MOVIE)
  put(self.game/'hd/music/old.ogg',b'OggS original');put(self.game/'kh_com.sav',b'player-save')
 def unchanged(self):
  self.assertEqual((self.game/'hd/music/old.ogg').read_bytes(),b'OggS original');self.assertEqual((self.game/'kh_com.sav').read_bytes(),b'player-save')
 def test_success_keeps_old_backup_and_merges(self):
  backup=m.transaction(self.game,self.source);self.unchanged();self.assertEqual((self.game/'hd/movies/ROP.mp4').read_bytes(),MOVIE);self.assertEqual((backup/'music/old.ogg').read_bytes(),b'OggS original')
 def test_cancel_corrupt_overlap_and_protected_preserve(self):
  with self.assertRaises(m.ImportErrorSafe):m.transaction(self.game,self.source,cancel=lambda:True)
  self.unchanged();put(self.source/'movies/ROP.mp4',b'corrupt')
  with self.assertRaises(m.ImportErrorSafe):m.transaction(self.game,self.source)
  self.unchanged()
  with self.assertRaises(m.ImportErrorSafe):m.transaction(self.game,self.game/'hd')
  put(self.source/'user.sav',b'forbidden')
  with self.assertRaises(m.ImportErrorSafe):m.transaction(self.game,self.source)
  self.unchanged()
 def test_interrupt_after_real_first_rename_rolls_back(self):
  replace=m.os.replace
  def interrupt(source,dest):
   result=replace(source,dest)
   if Path(source)==self.game/'hd':raise KeyboardInterrupt()
   return result
  with patch.object(m.os,'replace',side_effect=interrupt),self.assertRaises(KeyboardInterrupt):m.transaction(self.game,self.source)
  self.unchanged();self.assertFalse(list(self.game.glob('.hd-import-*')))
 def test_symlink_rejected(self):
  link=self.source/'movies/link.mp4'
  try:link.symlink_to(self.source/'movies/ROP.mp4')
  except OSError:self.skipTest('symlink permission unavailable')
  with self.assertRaises(m.ImportErrorSafe):m.transaction(self.game,self.source)
  self.unchanged()
 def test_real_eight_token_variant_and_partial_rejection(self):
  base=self.source/'media/en/g-fixture';put(base/'movies/ROP.mp4',MOVIE)
  put(base/'media.meta',b'REKHCOM_MEDIA_V1 en user aabb 0 1 0 1\nmovies/ROP.mp4 16\n')
  put(self.source/'media/en/active.txt',b'REKHCOM_MEDIA_ACTIVE_V1\nmovies g-fixture\nscenes -\n')
  m.transaction(self.game,self.source);self.unchanged()
  (base/'movies/ROP.mp4').unlink()
  with self.assertRaises((m.ImportErrorSafe,OSError)):m.transaction(self.game,self.source)
  self.unchanged()
 def test_failed_reference_producer_preserves_active(self):
  def fail(source,stage):raise RuntimeError('dependency/codec failure')
  with self.assertRaises(RuntimeError):m.transaction(self.game,self.source,fail)
  self.unchanged()
if __name__=='__main__':unittest.main()
