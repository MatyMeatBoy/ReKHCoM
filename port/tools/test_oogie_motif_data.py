"""ROM-free mask/source agreement; never invokes a compiler or game."""
from pathlib import Path
import unittest
import oogie_motifs as motifs

class OogieMotifData(unittest.TestCase):
    def test_generated_header_is_current(self):
        header=Path(__file__).resolve().parents[1]/'include/port/port_oogie_motifs.h'
        self.assertEqual(header.read_text(),motifs.generate())

    def test_missing_cheek_and_ghost_point_have_ink(self):
        self.assertEqual(motifs.art(240,457),146)
        self.assertEqual(motifs.art(519,449),146)

    def test_masks_exclude_the_native_room_and_use_live_palette_offsets(self):
        for y in range(408,488):
            for x in range(224,528):
                value=motifs.art(x,y)
                self.assertIn(value,(0,146,157,158))
                if 256<=x<496 and y<472:self.assertEqual(value,0)

if __name__=='__main__':unittest.main()
