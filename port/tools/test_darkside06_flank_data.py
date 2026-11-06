"""Pure indexed-data regressions; no compiler/game or ROM required."""
from pathlib import Path
import re,unittest

class DarksideFlankData(unittest.TestCase):
    def setUp(self):
        self.root=Path(__file__).resolve().parents[2]
        self.text=(self.root/'port/include/port/darkside_body_punch_flank.inc').read_text()
        self.rows=[[int(x,16)for x in re.findall(r'0x([0-9a-f]+)',row)]for row in re.findall(r'\{([^{}]+)\}',self.text)]
        self.values=[[v for b in row for v in (b&15,b>>4)]for row in self.rows]

    def test_shadow_has_no_lower_lobe_or_bright_pixels(self):
        self.assertEqual(len(self.values),58) # y122..179; native taper180..185 untouched
        self.assertTrue(all(len(row)==8 for row in self.values))
        self.assertEqual(sum(v!=0 for row in self.values for v in row),211)
        self.assertLessEqual(max(v for row in self.values for v in row),4)

    def test_added_rows_are_attached_without_isolated_pixels(self):
        for row in self.values:
            xs=[i for i,v in enumerate(row)if v]
            if xs:self.assertEqual(xs,list(range(max(xs)+1)))

    def test_approved_arm_never_overwritten(self):
        approved=(self.root/'port/include/port/darkside_body_punch.inc').read_text()
        for y,row in enumerate(re.findall(r'\{\s*(0x[^}]+)\}',approved)):
            for j,b in enumerate(re.findall(r'0x([0-9a-fA-F]+)',row)):
                byte=int(b,16)
                for x,v in ((232+j*2,byte&15),(233+j*2,byte>>4)):
                    if v and 256<=x<264 and 122<=y+42<180:self.assertEqual(self.values[y+42-122][x-256],0)

if __name__=='__main__':unittest.main()
