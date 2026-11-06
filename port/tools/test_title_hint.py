"""Execute actual title-hint arming/toggle C AST and check persisted option wiring."""
from pathlib import Path
import re,unittest
from pycparser import c_parser
from test_gba_border_layout import Header
ROOT=Path(__file__).resolve().parents[2]
class TitleHint(unittest.TestCase):
 def setUp(self):
  self.ui=(ROOT/'port/src/port_ui.c').read_text(encoding='utf8')
  self.c=Header()
 def test_hidden_hint_clears_previous_arm(self):
  fn=self.ui[self.ui.index('void PortUiTitleHint(void) {'):].split('\n}',1)[0]+'\n}'
  tree=c_parser.CParser().parse(fn)
  for hidden in (0,1):
   for previous in (0,1,2):
    state={'gPortConfig':{'hideTitleHint':hidden},'sTitleHintFrames':previous}
    self.c.run(tree.ext[0].body,state)
    self.assertEqual(state['sTitleHintFrames'],0 if hidden else 2)
 def test_option_persistence_default_and_toggle(self):
  config=(ROOT/'port/src/port_config.c').read_text(encoding='utf8')
  header=(ROOT/'port/include/port/port.h').read_text(encoding='utf8')
  self.assertRegex(header,r'int goldPremium;[^\n]*\n\s*int hideTitleHint;')
  self.assertIn('{ "hide_title_hint", &gPortConfig.hideTitleHint, 0, 1 }',config)
  toggle=re.search(r'case ITEM_HIDE_TITLE_HINT:\s*(gPortConfig.hideTitleHint =[^;]+;)\s*break;',self.ui)
  self.assertIsNotNone(toggle)
  tree=c_parser.CParser().parse('void toggle(void){'+toggle[1]+'}')
  for value in (0,1):
   state={'gPortConfig':{'hideTitleHint':value}};self.c.run(tree.ext[0].body,state)
   self.assertEqual(state['gPortConfig']['hideTitleHint'],1-value)
 def test_stale_draw_suppressed_and_select_unchanged(self):
  self.assertRegex(self.ui,r'if \(sTitleHintFrames > 0\) \{\s*sTitleHintFrames--;\s*if \(!gPortConfig.hideTitleHint\) DrawTitleHint\(l\);')
  title=(ROOT/'src/title/mode_title.c').read_text(encoding='utf8')
  self.assertRegex(title,r'PortUiTitleHint\(\);\s*if \(GetKeysPressed\(\) & SELECT_BUTTON\)\s*PortUiOpenOptions\(\);')
 def test_option_labels_and_onoff(self):
  self.assertIn('ITEM_DISCORD,\n    ITEM_HIDE_TITLE_HINT,',self.ui)
  enum=self.ui[self.ui.index('enum {\n    /* the main page'):].split('};',1)[0]+'};'
  enum=re.sub(r'/\*.*?\*/','',enum,flags=re.S)
  entries=c_parser.CParser().parse(enum).ext[0].type.values.enumerators
  names={};value=0
  for entry in entries:
   if entry.value is not None:value=self.c.expr(entry.value,names)
   names[entry.name]=value;value+=1
  labels=self.ui.split('sLabels[ITEM_COUNT][LANG_COUNT] = ',1)[1].split('\n};',1)[0]+'\n};'
  labels=re.sub(r'/\*.*?\*/','',labels,flags=re.S)
  rows=c_parser.CParser().parse('const char* labels[999][6] = '+labels).ext[0].init.exprs
  self.assertEqual(len(rows),names['ITEM_COUNT'])
  self.assertTrue(all(len(row.exprs)==6 for row in rows))
  self.assertEqual(rows[names['ITEM_HIDE_TITLE_HINT']].exprs[0].value,'"Hide title hint"')
  self.assertRegex(self.ui,r'case ITEM_HIDE_TITLE_HINT:\s*SDL_strlcpy\(out, sOnOff\[gPortConfig.hideTitleHint != 0\]\[l\], size\);')
if __name__=='__main__':unittest.main()
