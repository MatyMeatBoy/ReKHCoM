import tempfile, unittest
from pathlib import Path
from unittest.mock import Mock
from tile_editor import TileEditor

class ArtTilesTest(unittest.TestCase):
    def test_link_is_separate_from_editable_project_and_survives_restart(self):
        with tempfile.TemporaryDirectory() as d:
            editor=TileEditor(Path(d),Path(d))
            editor.new_project=Mock(return_value=dict(id='a'*32,scene='054'))
            project=editor.art_project('054','b'*32)
            editor.new_project.assert_called_once_with('054')
            project['parent_id']='c'*32
            editor=TileEditor(Path(d),Path(d))
            self.assertEqual(editor.art_parent(project['id']),'b'*32)
            with self.assertRaises(ValueError): editor.art_project('054','../escape')
            with self.assertRaises(ValueError): editor.art_parent('../escape')
