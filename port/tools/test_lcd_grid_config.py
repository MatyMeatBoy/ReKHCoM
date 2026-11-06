"""Real native INI persistence for LCD grid, independent of title hint and colours."""
import importlib.util
from pathlib import Path
TOOLS=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('lcd_config_fixture',TOOLS/'test_card_scroll.py')
fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)
anchor='    PortConfigLoad(); /* no settings file */'
assert anchor in fixture.HARNESS
fixture.HARNESS=fixture.HARNESS.replace(anchor,anchor+r'''
    assert(gPortConfig.lcdGrid==0);
    gPortConfig.hideTitleHint=1;gPortConfig.colorCorrection=1;
    for(int intensity=0;intensity<=100;intensity+=25) {
        gPortConfig.lcdGrid=intensity;PortConfigSave();
        gPortConfig.lcdGrid=-1;PortConfigLoad();
        assert(gPortConfig.lcdGrid==intensity);
        assert(gPortConfig.hideTitleHint==1&&gPortConfig.colorCorrection==1);
    }
    f=fopen("kh_com.ini","w");assert(f);fputs("lcd_grid=999\n",f);fclose(f);
    PortConfigLoad();assert(gPortConfig.lcdGrid==100);
    f=fopen("kh_com.ini","w");assert(f);fputs("lcd_grid=-1\n",f);fclose(f);
    PortConfigLoad();assert(gPortConfig.lcdGrid==0);
    gPortConfig.hideTitleHint=0;gPortConfig.colorCorrection=0;
''',1)
if __name__=='__main__':
 fixture.main()
 print('PASS actual LCD INI: default0, intensity roundtrip/clamp, independent title hint/colour correction; prior input suite')
