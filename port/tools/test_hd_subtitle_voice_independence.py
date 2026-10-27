"""Actual subtitle consumers: native LoadSubs and guest->Windows host adapter.

Only synthetic MP4 headers and owned test strings, in a fresh --work child.
No video/audio/GPU or full game/guest build. No installed files are modified.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
MOVIE='zz_event_012'


def function(source,signature):
    start=source.index(signature+' {');end=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]+'\n'


def put(path,text):
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(text,encoding='utf-8')


def generation(root,voice,movies=False,active=True):
    label='ja' if voice==2 else 'en';name='g-'+label;base=root/'hd/media'/label/name
    member='movies/ROP.mp4' if movies else 'scenes/'+MOVIE+'.mp4'
    path=base/member;path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(struct.pack('>I4sI4s',8,b'moov',17,b'mdat')+b'synthetic')
    put(base/'media.meta',f'REKHCOM_MEDIA_V1 {label} user fixture 0 {int(movies)} {int(not movies)} 1\n{member} {path.stat().st_size}\n')
    if active:put(base.parent/'active.txt',f'REKHCOM_MEDIA_ACTIVE_V1\nmovies {name if movies else "-"}\nscenes {"-" if movies else name}\n')
    return base


def cues(label,shift=0):
    return f'{shift} {29+shift} {label}-owned-one\n{30+shift} {59+shift} {label}-owned-two\n'


def cases(output):
    names=['cross-compatible','cross-shifted','missing-es','missing-reference','same-generation-es',
           'legacy-es','global-compatible','global-shifted','cue-count-differs','ja-reference',
           'uncommitted-only','movie-generation-subs','invalid-range','empty-text']
    result=[]
    for name in names:
        root=output/name;jp=generation(root,2);en=generation(root,1,movies=name=='movie-generation-subs',active=name!='uncommitted-only')
        if name not in ('missing-reference','ja-reference'):put(jp/'subs'/f'{MOVIE}.us.txt',cues('EN'))
        if name=='ja-reference':put(jp/'subs'/f'{MOVIE}.ja.txt',cues('JA'))
        if name not in ('missing-es','global-compatible','global-shifted','legacy-es','same-generation-es'):
            text=cues('ES',1 if name=='cross-shifted' else 0)
            if name=='cue-count-differs':text='0 29 ES-owned-one\n'
            if name=='invalid-range':text='65536 65537 ES-owned-one\n'
            if name=='empty-text':text='0 29\n30 59\n'
            put(en/'subs'/f'{MOVIE}.es.txt',text)
        if name=='same-generation-es':put(jp/'subs'/f'{MOVIE}.es.txt',cues('ES'))
        if name in ('legacy-es','global-compatible','global-shifted'):
            put(root/'hd/subs'/f'{MOVIE}.es.txt',cues('ES',1 if name=='global-shifted' else 0))
        mode=0 if name=='legacy-es' else 2
        if not mode:
            p=root/'hd/scenes'/f'{MOVIE}.mp4';p.parent.mkdir(parents=True);p.write_bytes(b'private legacy fixture')
        expected=2 if name in ('cross-compatible','same-generation-es','legacy-es','global-compatible','ja-reference','movie-generation-subs') else 0 if name=='missing-reference' else 1
        result.append((name,root,mode,expected))
    return result


PRELUDE=r'''
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "port/port_hd_media.h"
#include "port/port_hd_subtitles.h"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"consumer assertion: %s\n",#c);exit(97);}}while(0)
#define SDL_zerop(p) memset(p,0,sizeof(*(p)))
#define SDL_malloc malloc
#define SDL_free free
#define SDL_snprintf snprintf
typedef wchar_t SubChar;
typedef struct {int unused;} SDL_Texture;
static void SDL_DestroyTexture(SDL_Texture*t){CHECK(!t);}
static void SDL_strlcpy(char*out,const char*in,size_t size){snprintf(out,size,"%s",in);}
static struct{int hdMediaLanguage,hdSubtitles,undub;}gPortConfig;
static uint32_t gLanguage=4;
static const char* dataRoot;
static const char* PortConfigPath(const char*name){static char path[4096];snprintf(path,sizeof(path),"%s/%s",dataRoot,name);return path;}
static int PortHdMovieResolvePath(const char*name,char*out,size_t size){return PortHdMediaResolve(dataRoot,gPortConfig.hdMediaLanguage,name,out,size);}
static void PortLog(const char*format,...){(void)format;}
static int sStartAt;
static char capturedMovie[1024],capturedSubs[2][1024];
static int expectedLanguage;
'''


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--work',type=Path,required=True);p.add_argument('--cc',type=Path,required=True)
    a=p.parse_args();a.work.mkdir(parents=True,exist_ok=True)
    output=Path(tempfile.mkdtemp(prefix='subtitle-consumer-',dir=a.work));fixtures=cases(output)
    source=(ROOT/'port/src/port_hdmovie.c').read_text(encoding='utf-8')
    prepare_path=ROOT/'port/windows64/prepare_movie_core.py';spec=importlib.util.spec_from_file_location('movie_prepare',prepare_path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);module.prepare(ROOT,output)
    generated=(output/'windows_movie_subtitles.inc').read_text(encoding='utf-8')
    code=PRELUDE
    # The helper is absent before the fix; still compile the baseline consumer.
    if not (ROOT/'port/include/port/port_hd_subtitles.h').exists():code=code.replace('#include "port/port_hd_subtitles.h"','')
    code+=function(source,'static void MovieSubtitlePath(const char* name, const char* language, char* out, size_t size)')
    code+=function(source,'static void SubsOrder(const char* order[2])')
    begin=source.index('#define SUB_MAX');end=source.index('static void FreeSubs',begin)
    code+=source[begin:end]+function(source,'static void FreeSubs(Subs* s)')
    code+=r'''
static void CheckText(Subs*s){
 CHECK(s->count==(expectedLanguage?2:0));
 if(s->count)CHECK(wcsstr(s->text[0],expectedLanguage==2?L"ES-owned":L"EN-owned")!=NULL);
}
static void HostMovieSubtitlePath(const char*name,int index,char*out,size_t size){(void)name;SDL_strlcpy(out,capturedSubs[index],size);}
'''
    code+=function(generated,'static int LoadSubs(Subs* s, const char* name)').replace('static int LoadSubs','static int HostLoadSubs',1).replace('MovieSubtitlePath(name, i,','HostMovieSubtitlePath(name, i,')
    code+=r'''
static int h_hd_movie(const char*movie,const char*s0,const char*s1,const char*font){
 (void)font;SDL_strlcpy(capturedMovie,movie,sizeof(capturedMovie));SDL_strlcpy(capturedSubs[0],s0?s0:"",sizeof(capturedSubs[0]));SDL_strlcpy(capturedSubs[1],s1?s1:"",sizeof(capturedSubs[1]));
 Subs host;HostLoadSubs(&host,"zz_event_012");CheckText(&host);FreeSubs(&host);return 1;
}
static void MoviePath(const char*name,char*out,size_t size){PortHdMovieResolvePath(name,out,size);}
static void PortBridgeRefreshTextureEpoch(void){}
'''
    guest=source[source.index('#elif defined(__ANDROID__) || defined(PORT_ANDROID64) || defined(PORT_LINUX64_GUEST)'):]
    code+=function(guest,'static int PlayFile(const char* name)')
    code+=r'''
int main(int argc,char**argv){
 CHECK(argc==4);dataRoot=argv[1];gPortConfig.hdMediaLanguage=atoi(argv[2]);gPortConfig.hdSubtitles=1;gPortConfig.undub=1;expectedLanguage=atoi(argv[3]);
 char before[1024],after[1024];CHECK(PortHdMovieResolvePath("zz_event_012",before,sizeof(before)));
 Subs native;LoadSubs(&native,"zz_event_012");CheckText(&native);FreeSubs(&native);
 CHECK(PlayFile("zz_event_012")==1);CHECK(!strcmp(before,capturedMovie));
 CHECK(PortHdMovieResolvePath("zz_event_012",after,sizeof(after))&&!strcmp(before,after));
 CHECK(gPortConfig.undub==1);printf("PASS native and guest->host: language=%d voice=%d unchanged\n",expectedLanguage,gPortConfig.hdMediaLanguage);
 return 0;
}
'''
    unit=output/'consumer.c';exe=output/'consumer.exe';unit.write_text(code,encoding='utf-8')
    subprocess.run([str(a.cc),'-std=gnu11','-DVERSION_EU','-DPORT_LINUX64_GUEST','-I'+str(ROOT/'port/include'),str(unit),'-o',str(exe)],check=True)
    results=[]
    for name,root,mode,expected in fixtures:
        run=subprocess.run([str(exe),str(root),str(mode),str(expected)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=10,creationflags=0x08004000)
        (output/(name+'.log')).write_bytes(run.stdout);print(name,run.returncode,run.stdout.decode(errors='replace').strip(),flush=True)
        results.append(dict(case=name,exit=run.returncode))
        if run.returncode:raise AssertionError(f'{name}: actual subtitle consumer failed; evidence {output}')
    (output/'results.json').write_text(json.dumps(dict(source_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
        movie_source_sha256=hashlib.sha256((ROOT/'port/src/port_hdmovie.c').read_bytes()).hexdigest(),
        subtitle_helper_sha256=hashlib.sha256((ROOT/'port/include/port/port_hd_subtitles.h').read_bytes()).hexdigest(),
        exe_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),cases=results),indent=2))
    print('Evidence:',output)


if __name__=='__main__':main()
