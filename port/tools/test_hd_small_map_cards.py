"""Actual small-card catalog and selective recipe writer; no owned game needed."""
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

import numpy as np
from khhd_sprites import apply, write_recipe_marks
from test_launcher_stale_sprites import fnv
import hd_small_cards as small


class SmallMapCards(unittest.TestCase):
    def test_every_map_family_has_an_import_recipe(self):
        rows={line.split('\t')[0]:line.split('\t')
              for line in small.TSV.read_text(encoding='utf-8').splitlines()
              if line and not line.startswith('#')}
        missing=[name for name in small.map_pairs() if name+'SmallCardHD' not in rows]
        self.assertEqual(missing,[], 'Map families must not disappear from the import inventory')
        self.assertEqual(rows['gCardRoom23Tiles'][8],
                         'FORM/0050/FO0050.RTM:UK_card.imz#19')
        self.assertEqual(rows['gKeyToRewardsSmallCardHD'][8],rows['gCardRoom23Tiles'][8])
        self.assertNotIn('erase=',rows['gKeyToRewardsSmallCardHD'][9])

    def test_python_full_import_marks_absent_recipes_in_desktop_format(self):
        row='missing\t6\tframe\ttiles\t-8\t-20\t16\t32\towned:tex\tscale=8\teu'
        raw='layout\t3\t-\t-\t0\t0\t0\t0\towned:layout\traw'
        with tempfile.TemporaryDirectory(prefix='python-recipe-inventory-') as temp:
            root=Path(temp);source=root/'source.tsv'
            source.write_text('# header\n'+row+'\n'+raw+'\n',encoding='utf-8')
            write_recipe_marks(root,source)
            self.assertEqual((root/'recipes.txt').read_text().splitlines(),
                             ['__recipe_inventory_v2__\t1',f'missing\t{fnv(row)}'])

    def test_map_definitions_join_existing_battle_catalog(self):
        pairs=small.pairs()
        self.assertIn('gTranquilDarkness',pairs)
        self.assertEqual(pairs['gTranquilDarkness'],
                         ('gCardRoom02','gTranquilDarknessSmallCardFrame0',
                          'gTranquilDarknessSmallCardTiles','gTranquilDarknessSmallCardPalette'))
        self.assertEqual(pairs['gKeyToRewards'][0], 'gCardRoom23')
        self.assertIn('gPotion',pairs)
        self.assertIn('gSimba',pairs)
        self.assertIn('gKingdomKey',pairs)
        self.assertEqual(len(small.map_pairs()),26)

    def test_non_square_crop_and_story_key_preserve_native_frame(self):
        texture=np.full((256,256,4),255,dtype=np.uint8)
        for notch in (True,False):
            image=apply(texture,small.crop_ops((14,0,242,256),notch=notch),(-8,-20,16,32))
            self.assertEqual(image.shape,(256,128,4))
            alpha=image[:,:,3]
            # Only the picture window may replace cartridge pixels. Crown,
            # borders and the two original corner cells stay native.
            outside=alpha.copy();outside[128:248,8:120]=0
            self.assertFalse(outside.any())
            self.assertEqual(alpha[140,16],255)
            if notch:
                self.assertEqual(alpha[220,108],0)
            else:
                self.assertTrue((alpha[128:248,8:120]==255).all())

    def test_selective_write_preserves_approved_unselected_rows(self):
        # Execute the production main's write branch; its media decoding and
        # crop search are not needed to exercise the output-file contract.
        source=Path(small.__file__).read_text(encoding='utf-8')
        start=source.index("    if '--write' not in sys.argv:")
        end=source.index("\n\nif __name__",start)
        exec('def fixture_write(rows):\n'+source[start:end],small.__dict__)
        old='gKingdomKeySmallCardHD\t6\tapproved-crop-and-seam'
        selected='gTranquilDarknessSmallCardHD\t6\told'
        new='gTranquilDarknessSmallCardHD\t6\tnew'
        with tempfile.TemporaryDirectory(prefix='small-map-recipes-') as temp:
            root=Path(temp);table=root/'table.tsv';table.write_text('# header\n'+old+'\n'+selected+'\n')
            with patch.object(small,'ROOT',root),patch.object(small,'TSV',table), \
                 patch.object(sys,'argv',['hd_small_cards.py','owned.hed','--write','--only','gTranquilDarkness']):
                small.fixture_write([new])
            lines=table.read_text().splitlines()
            self.assertIn(old,lines)
            self.assertEqual(lines.count(new),1)
            self.assertNotIn(selected,lines)
            self.assertEqual(lines[0],'# header')


if __name__=='__main__':unittest.main()
