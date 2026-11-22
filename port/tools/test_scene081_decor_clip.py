"""Check the native-frame guard and clipping predicate without a C compiler."""
import re
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]

class DecorClip(unittest.TestCase):
    def predicate(self):
        source=(ROOT/'port/include/port/port_scene081_clip.h').read_text()
        body=re.search(r'return\s+(.*?);',source,re.S).group(1)
        expression=' '.join(body.replace('&&',' and ').replace('||',' or ').split())
        return lambda gx,gy,wx,wy: bool(eval(expression,{'__builtins__':{}},dict(gx=gx,gy=gy,wx=wx,wy=wy)))

    def test_fragment_and_surrounding_floor_are_distinguished(self):
        clip=self.predicate()
        self.assertTrue(clip(64,208,80,256))
        self.assertTrue(clip(-4,200,0,263))
        for wx,wy in ((80,255),(-1,260),(128,260),(80,512)):
            self.assertFalse(clip(64,208,wx,wy))

    def test_native_frame_always_wins_even_if_camera_reaches_fragment(self):
        clip=self.predicate()
        for gy in range(160):
            for gx in range(240):
                self.assertFalse(clip(gx,gy,80,260))

    def test_gate_is_specific_to_reviewed_decoration_and_scene(self):
        source=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8')
        start=source.index('/* 081: the authored platform leaves')
        gate=source[start:source.index('            if (sMainHdPart[i])',start)] if '            if (sMainHdPart[i])' in source[start:] else source[start:source.index('            if (sMainHdPart',start)]
        self.assertIn('sActorSceneKey == EVENT_081_MONSTORO_E3',gate)
        self.assertIn('sOamHd[i].tiles == gMapGmkMonstro03Tiles',gate)
        self.assertIn('sPreliminarRecipe[1]->v[0] == 81',gate)
        self.assertIn('!sOamTag[i].screen',gate)

if __name__=='__main__':unittest.main()
