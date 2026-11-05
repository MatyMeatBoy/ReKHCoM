"""Check imported requirement readability without copyrighted source textures."""
import unittest
import numpy as np
import synthesis_requirement_hd as s
import khhd_sprites as k

class ReComOutline(unittest.TestCase):
 def test_single_total_size_and_drop_shadow(self):
  ops=s.recom_requirement((-8,-5,16,16),3,0).split(';')
  blits=[list(map(int,o[5:].split(',')))for o in ops if o.startswith('blit=')]
  shadow,face=blits
  self.assertEqual(face[7]-face[5],96) #12GBApx at8x, preserving its italic ratio
  self.assertEqual(shadow[4]-face[4],6)
  self.assertEqual(shadow[5]-face[5],6)

 def test_white_edge_strength_and_glass_fill(self):
  tex=np.zeros((192,512,4),dtype=np.uint8)
  x0,y0,x1,y1=s.RECOM_CARD;tex[y0:y1,x0:x1]=(255,255,255,255)
  x0,y0,x1,y1=s.RECOM_OUTLINE
  tex[y0:y1,x0:x0+2]=(255,255,255,255);tex[y0:y1,x1-2:x1]=(255,255,255,255)
  tex[y0:y0+2,x0:x1]=(255,255,255,255);tex[y1-2:y1,x0:x1]=(255,255,255,255)
  for color in range(5):
   with self.subTest(color=color):
    b=k.apply(tex,s.recom_body((-16,-20,32,39),color),(-16,-20,32,39))
    self.assertGreaterEqual(int(b[150,22,3]),250)
    self.assertTrue(np.all(b[150,22,:3]>=230))
    self.assertEqual(int(b[150,128,3]),89) #unchanged35%glass alpha
    expected=[int(s.RECOM_GLASS[color][i:i+2],16)for i in(0,2,4)]
    self.assertEqual(b[150,128,:3].tolist(),expected)
    self.assertEqual(b[0,0,3],0)

 def test_every_rule_value_keeps_oam_and_card_clearance(self):
  pieces=list(s.specs());self.assertEqual(len(pieces),405)
  for _,name,box,rule,value in pieces:
   if rule is None:continue
   with self.subTest(name=name):
    for op in s.recom_requirement(box,rule,value).split(';'):
     if op.startswith('blit='):
      v=list(map(int,op[5:].split(',')));x0,y0,x1,y1=v[4:]
      self.assertTrue(0<=x0<x1<=box[2]*s.K)
      self.assertTrue(0<=y0<y1<=box[3]*s.K)
      self.assertGreaterEqual(box[0]+x0/s.K,-13)
      self.assertLessEqual(box[0]+x1/s.K,13)
     if op.startswith('poly='):
      v=list(map(int,op.split(',')[1:]));self.assertTrue(all(0<=x<=box[2]*s.K for x in v[::2]));self.assertTrue(all(0<=y<=box[3]*s.K for y in v[1::2]))

 def test_tsv_refresh_preserves_neighbor_order(self):
  import tempfile
  from pathlib import Path
  from unittest.mock import patch
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);target=root/'port/data/hd_sprites.tsv';target.parent.mkdir(parents=True)
   target.write_text('before\n'+s.TSV_BEGIN+'\nold recipe\n'+s.TSV_END+'\nafter\n')
   with patch.object(s,'ROOT',root):s.write_tsv()
   rows=target.read_text().splitlines()
   self.assertEqual(rows[0],'before');self.assertEqual(rows[-1],'after')
   self.assertEqual(rows[1:-1],s.recipe_rows())

if __name__=='__main__':unittest.main()
