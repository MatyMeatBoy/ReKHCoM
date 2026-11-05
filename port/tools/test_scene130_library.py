"""The exact accepted library recipe participates in normal wide rendering."""
import unittest
from pathlib import Path
from preliminar_v1_recipes import READY, RECIPES


class LibraryRecipeTest(unittest.TestCase):
    def test_accepted_library_is_identified_and_enabled_without_preview_flags(self):
        self.assertIn(130, READY)
        self.assertEqual(RECIPES[130][:2], (64, 64))
        ppu = (Path(__file__).resolve().parents[1] / 'src/port_ppu.c').read_text()
        approved = ppu.split('kPreliminarApproved[] = {', 1)[1].split('};', 1)[0]
        self.assertIn(130, [int(v.strip()) for v in approved.split(',') if v.strip()])


if __name__ == '__main__':
    unittest.main()
