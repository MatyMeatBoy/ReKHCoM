"""The real menu test must locate the Item enum and respect explicit aliases."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import test_text_options as options


class OptionsRouteTests(unittest.TestCase):
    def parse(self, source):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'port/src').mkdir(parents=True)
            (root / 'port/src/port_ui.c').write_text(source, encoding='utf-8')
            with patch.object(options, 'ROOT', root):
                return options.item_numbers()

    def test_current_source_has_every_route_item(self):
        numbers = options.item_numbers()
        self.assertTrue(all(name in numbers for name, *_ in options.ROUTE))

    def test_heading_comment_can_change(self):
        numbers = self.parse('enum { LANG_EN, LANG_ES };\nenum {\n /* any heading, or no heading */\n ITEM_VIEW, ITEM_ZOOM\n};')
        self.assertEqual(numbers, {'VIEW': 0, 'ZOOM': 1})

    def test_explicit_rematch_range_preserves_following_indices(self):
        numbers = self.parse('enum {\n    /* the game: old heading */\n ITEM_REMATCH_0,\n ITEM_REMATCH_LAST = ITEM_REMATCH_0 + 13,\n ITEM_CONTROL_A,\n ITEM_CONTROL_B,\n ITEM_COUNT\n};')
        self.assertEqual(numbers['REMATCH_LAST'], 13)
        self.assertEqual(numbers['CONTROL_A'], 14)
        self.assertEqual(numbers['CONTROL_B'], 15)
        self.assertEqual(numbers['COUNT'], 16)


if __name__ == '__main__':
    unittest.main()
