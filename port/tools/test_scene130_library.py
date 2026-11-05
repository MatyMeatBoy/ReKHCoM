"""Scene 130 remains an opt-in native-coordinate recipe until human review."""
import unittest
from pathlib import Path
from preliminar_v1_recipes import READY, RECIPES


class LibraryRecipeTest(unittest.TestCase):
    def test_library_can_be_identified_without_approving_the_candidate(self):
        self.assertIn(130, READY)
        self.assertEqual(RECIPES[130][:2], (64, 64))
        ppu = (Path(__file__).resolve().parents[1] / 'src/port_ppu.c').read_text()
        approved = ppu.split('kPreliminarApproved[] = {', 1)[1].split('};', 1)[0]
        self.assertNotIn('130', approved)


if __name__ == '__main__':
    unittest.main()
