import json
import tempfile
import unittest
from pathlib import Path

from tile_editor import TileEditor, digest


class NativeBlankTileTest(unittest.TestCase):
    def atlas(self, directory, tile, raw):
        editor = TileEditor(directory, directory)
        editor.source = {'rom_sha256': 'test-cartridge'}
        background = {'palette_words': list(range(16)), 'palette': [[i, i, i] for i in range(16)]}
        return editor._atlas({'name': 'bg2', 'entries': [tile], 'tiles': raw}, background)

    def test_unloaded_last_slot_is_explicit_initial_vram_and_keeps_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            tileset, variants, atlas = self.atlas(directory, 1023, bytes([0x11]) * 32)
            self.assertEqual(variants, {(1023, 0): 0})
            self.assertEqual(tileset['non_donor_tileids'], [0])
            self.assertEqual(atlas.crop((0, 0, 8, 8)).getchannel('A').getextrema(), (0, 0))
            indices = json.loads((Path(directory) / 'assets' / tileset['source_sha256'] / 'indices.json').read_text())
            self.assertEqual(indices['tiles_sha256'], digest(bytes([0x11]) * 32))
            self.assertEqual(indices['tiles'][0]['native_tile'], 1023)
            self.assertEqual(indices['tiles'][0]['storage'], 'initial-cleared-vram')
            self.assertEqual(indices['tiles'][0]['pixels'], [0] * 64)

    def test_resident_last_slot_uses_its_real_rom_pixels(self):
        with tempfile.TemporaryDirectory() as directory:
            _, _, atlas = self.atlas(directory, 1023, bytes(1023 * 32) + bytes([0x11]) * 32)
            self.assertEqual(atlas.getpixel((0, 0)), (1, 1, 1, 255))

    def test_other_missing_slots_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, 'native tile bank'):
                self.atlas(directory, 1022, bytes(32))

    def test_partial_last_slot_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, 'native tile bank'):
                self.atlas(directory, 1023, bytes(1023 * 32 + 1))


if __name__ == '__main__':
    unittest.main()
