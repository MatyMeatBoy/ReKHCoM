"""Exercise real INI load/save for the independent title hint preference.
Reuses the ROM-free native input/config harness; no game or player saves.
"""
import importlib.util
from pathlib import Path
TOOLS=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('title_config_fixture',TOOLS/'test_card_scroll.py')
fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)
anchor='    PortConfigLoad(); /* no settings file */'
assert anchor in fixture.HARNESS
fixture.HARNESS=fixture.HARNESS.replace(anchor,anchor+r'''
    assert(gPortConfig.hideTitleHint==0); /* old files keep the visible default */
    for(int hidden=0;hidden<2;hidden++) {
        gPortConfig.hideTitleHint=hidden;PortConfigSave();
        gPortConfig.hideTitleHint=!hidden;PortConfigLoad();
        assert(gPortConfig.hideTitleHint==hidden);
    }
    f=fopen("kh_com.ini","w");assert(f);fputs("hide_title_hint=99\n",f);fclose(f);
    PortConfigLoad();assert(gPortConfig.hideTitleHint==1);
    f=fopen("kh_com.ini","w");assert(f);fputs("hide_title_hint=-1\n",f);fclose(f);
    PortConfigLoad();assert(gPortConfig.hideTitleHint==0);
    f=fopen("kh_com.ini","w");assert(f);fputs("stock_button=1\n",f);fclose(f);
    PortConfigLoad();assert(gPortConfig.hideTitleHint==0);
''',1)
if __name__=='__main__':
 fixture.main()
 print('PASS actual title-hint INI: default visible, hidden/visible roundtrip, clamping, legacy file; existing input shortcuts intact')
