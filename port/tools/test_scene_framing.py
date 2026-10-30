"""Compile the actual scene camera policy and INI consumer."""
from gen_scene_framing import generate
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
CODE=r'''
#include <stdio.h>
#include <math.h>
#include "port/port.h"
#include "port/scene_framing.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int PortUiMenuCanvas(void){return 0;}
void PortLog(const char* format,...){(void)format;}
void PortHdMovieInvalidatePaths(void){}
int PortUndubAvailable(void){return 0;}
void PortSetQolButtons(int on){(void)on;}
int main(void){
    int sizes[][2]={{240,160},{640,360},{1280,720},{1280,800},{1920,1080},{2560,1440},{3840,2160},{3440,1440},{5120,1440},{1024,768},{600,1000}};
    CHECK(gPortSceneFraming==PORT_SCENE_ADJUST);
    CHECK(PortSceneFramingScale(1920,1080)==6);CHECK(PortSceneFramingScale(1280,720)==4);
    CHECK(PortSceneFramingNarrowScale(1,1920,1080,6.75f)==6.0f);
    CHECK(PortSceneFramingNarrowScale(0,1920,1080,6.75f)==6.75f);
    for(unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++){
        int s=PortSceneFramingScale(sizes[i][0],sizes[i][1]);
        CHECK(s>=1&&s*240<=sizes[i][0]&&s*160<=sizes[i][1]);
    }
    CHECK(PortSceneFramingUse(PORT_SCENE_ADJUST,1,7));
    CHECK(!PortSceneFramingUse(PORT_SCENE_EXTENDED,1,7));
    CHECK(!PortSceneFramingUse(PORT_SCENE_ADJUST,0,7));
    CHECK(!PortSceneFramingUse(PORT_SCENE_ADJUST,1,121));
    CHECK(PortSceneFramingUse(PORT_SCENE_ADJUST,1,107));
    CHECK(PortSceneFramingUse(PORT_SCENE_ADJUST,1,108));
    CHECK(!PortSceneFramingHonors(-1));CHECK(!PortSceneFramingHonors(0x7fffffff));
    for(int e=0;e<210;e++)if(PortSceneFramingHonors(e))printf("HONORS %d\n",e);
    PortConfigSetDataPath(".");gPortConfig.zoom=8;gPortConfig.hudSize=1;
    gPortSceneFraming=PORT_SCENE_EXTENDED;PortConfigSave();gPortSceneFraming=0;PortConfigLoad();
    CHECK(gPortSceneFraming==PORT_SCENE_EXTENDED&&gPortConfig.zoom==8&&gPortConfig.hudSize==1);
    FILE* f=fopen("kh_com.ini","w");CHECK(f);fprintf(f,"config_version=11\nzoom=8\n");fclose(f);
    PortConfigLoad();CHECK(gPortSceneFraming==PORT_SCENE_ADJUST&&gPortConfig.zoom==8);
    f=fopen("kh_com.ini","w");CHECK(f);fprintf(f,"config_version=11\nscene_framing=999\n");fclose(f);
    PortConfigLoad();CHECK(gPortSceneFraming==PORT_SCENE_EXTENDED);
    f=fopen("kh_com.ini","w");CHECK(f);fprintf(f,"config_version=11\nscene_framing=-3\n");fclose(f);
    PortConfigLoad();CHECK(gPortSceneFraming==PORT_SCENE_ADJUST);
    puts("PASS: scene integer policy, strict honors, default/legacy/persistence and unchanged zoom/HUD");
    return 0;
}
'''
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--cc',required=True);ap.add_argument('--sdl',type=Path,required=True);a=ap.parse_args()
    arch='i686-w64-mingw32' if 'i686' in a.cc else 'x86_64-w64-mingw32';sdl=a.sdl/arch
    with tempfile.TemporaryDirectory(prefix='scene-framing-') as temporary:
        work=Path(temporary);generate(ROOT,work/'scene_framing_grants.h');src,exe=work/'test.c',work/'test.exe';src.write_text(CODE)
        shutil.copy2(sdl/'bin/SDL3.dll',work/'SDL3.dll')
        subprocess.run([a.cc,'-std=gnu11','-I'+str(work),'-I'+str(ROOT/'port/include'),'-I'+str(sdl/'include'),str(src),
                        str(ROOT/'port/src/port_config.c'),'-L'+str(sdl/'lib'),'-lSDL3','-o',str(exe)],check=True)
        r=subprocess.run([str(exe)],cwd=work,capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='');r.check_returncode()
        manifest=json.loads((ROOT/'port/progress/SCENE_FRAMING_HONORS.json').read_text())
        observed={int(line.split()[1]) for line in r.stdout.splitlines() if line.startswith('HONORS ')}
        assert observed=={row['event'] for row in manifest['entries']}
        assert all(row['status']=='honors' and row['asset_id'] and row['art_sha256'] for row in manifest['entries'])
    print('PASS: compiled whitelist matches reviewed honors-version manifest')
if __name__=='__main__':main()
