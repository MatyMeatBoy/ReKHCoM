"""Actual config load/save for audio mode, independent of GBA Undub and text.

Uses the existing ROM-free input/config fixture and real SDL headers/libraries.
"""
import importlib.util
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('card_config_fixture', TOOLS / 'test_card_scroll.py')
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)
anchor = '    PortConfigLoad(); /* no settings file */'
assert anchor in fixture.HARNESS
fixture.HARNESS = fixture.HARNESS.replace(anchor, anchor + r'''
    assert(gPortConfig.hdMediaLanguage == 0);
    for (int mode = 0; mode < 3; ++mode) {
        gPortConfig.hdMediaLanguage = mode;
        gPortConfig.undub = 1;
        gPortConfig.hdSubtitles = 1;
        PortConfigSave();
        gPortConfig.hdMediaLanguage = -1;
        PortConfigLoad();
        assert(gPortConfig.hdMediaLanguage == mode);
        assert(gPortConfig.undub == 1 && gPortConfig.hdSubtitles == 1);
    }
    gPortConfig.hdMediaLanguage = 0; PortConfigSave();
''', 1)

if __name__ == '__main__':
    fixture.main()
    print('PASS actual INI: Auto/EN/JP roundtrip, legacy default, independent GBA Undub/subtitles')
