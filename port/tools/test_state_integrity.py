"""Exercise production state Save/Load with real files and injected I/O failures."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[2]
class StateIntegrity(unittest.TestCase):
 def test_state_transactions(self):
  with tempfile.TemporaryDirectory() as tmp:
   for i, name in enumerate(['port/src/port_state.c', 'port/ps5/guest/src/state_ps5.c']):
    source=(ROOT/name).read_text()
    visitors=source[source.index('static void VisitWrite'):source.index('static void VisitAll')]
    operations=source[source.index('/* Config paths') if '/* Config paths' in source else source.index('static int Save('):source.index('void PortStateRequestSave')]
    code=Path(tmp)/f'test{i}.c'; exe=code.with_suffix('.exe')
    paths=source[source.index('const char* PortStatePath'):source.index('static void MakeParentDir')]
    request=source[source.index('void PortStateRequestLoad'):source.index('/* The same,') if '/* The same,' in source else source.index('/* Boss rematches')]
    code.write_text(PREAMBLE+visitors+'static void VisitAll(void(*v)(void*,size_t)){v(memory,7);v(memory+7,oversized ? (size_t)64*1024*1024 : 9); }\n'+paths+request+operations+HARNESS.replace('NAMESPACE', 'guest' if i else 'native32'))
    subprocess.run([os.environ.get('CC','clang'),'-std=c99','-Wall','-Wextra','-Werror',str(code),'-o',str(exe)],check=True)
    result=subprocess.run([str(exe)],cwd=tmp,capture_output=True,text=True)
    self.assertEqual(result.returncode,0,result.stdout+result.stderr)
PREAMBLE=r'''#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#define CreateDirectoryA(p,x) mkdir(p,0755)
#endif
#define STATE_MAGIC 0x5453484Bu
#define STATE_VERSION 1
#define MAX_PATH 260
#define SDL_snprintf snprintf
#define SDL_strlcpy(a,b,n) snprintf(a,n,"%s",b)
#define MakeDir(p) ((void)(p))
#define h_mkdir(p) ((void)(p))
static const char* PortConfigPath(const char*p){return p;}
static char sPendingPath[MAX_PATH];
static int sPendingLoad, gPortStateSlot=3;
static unsigned char memory[16];
static FILE* sFile;
static int sOk, failure, resets, achievements, oversized;
static uint32_t BuildId(void){return 123;}
static void MakeParentDir(const char*p){(void)p;}
static void PortLog(const char*p,...){(void)p;}
static void PortAudioReset(void){resets++;}
void PortAchievementsInvalidate(const char*p){(void)p;achievements++;}
static FILE* Open(const char*p,const char*m){if(failure==1&&strchr(m,'w'))return NULL;return fopen(p,m);}
static size_t Write(const void*p,size_t a,size_t b,FILE*f){if(failure==2)return fwrite(p,a,b/2,f);return fwrite(p,a,b,f);}
static size_t Read(void*p,size_t a,size_t b,FILE*f){return fread(p,a,failure==5?b/2:b,f);}
static void* Allocate(size_t n){return failure==6?NULL:malloc(n);}
static int Close(FILE*f){int r=fclose(f);return failure==3?EOF:r;}
static int Replace(const char*a,const char*b){
 if(failure==4)return -1;
#ifdef _WIN32
 return !MoveFileExA(a,b,MOVEFILE_REPLACE_EXISTING);
#else
 return rename(a,b);
#endif
}
#define malloc Allocate
#define fread Read
#define fopen Open
#define fwrite Write
#define fclose Close
#define rename Replace
#define h_rename Replace
#define h_remove remove
#define MoveFileExA(a,b,c) (!Replace(a,b))
'''
HARNESS=r'''
int main(void){
 unsigned char old[28],now[28];FILE*f;int n;
 assert(!strcmp(PortStatePath(3),"states/NAMESPACE/slot3.khst"));
 CreateDirectoryA("states",NULL);CreateDirectoryA("states/NAMESPACE",NULL);
 remove("states/NAMESPACE/slot3.khst");
 PortStateRequestLoad(NULL);assert(sPendingLoad&&!strcmp(sPendingPath,"states/slot3.khst"));
 memset(memory,42,16);assert(Save(sPendingPath)==1);
 assert(Save(PortStatePath(3))==1);
 PortStateRequestLoad(NULL);assert(!strcmp(sPendingPath,"states/NAMESPACE/slot3.khst"));
 memset(memory,0,16);assert(Load("states/slot3.khst")==1&&memory[0]==42);
 memset(memory,42,16);assert(Save("state.khst")==1);
 f=fopen("state.khst","rb");assert(fread(old,1,28,f)==28);fclose(f);
 for(n=1;n<=4;n++){failure=n;memset(memory,99,16);assert(Save("state.khst")==0);failure=0;f=fopen("state.khst","rb");assert(fread(now,1,28,f)==28);fclose(f);assert(!memcmp(old,now,28));}
 memset(memory,0,16);assert(Load("state.khst")==1);assert(memory[0]==42&&memory[15]==42);
 for(n=0;n<28;n++){f=fopen("broken.khst","wb");assert(fwrite(old,1,n,f)==(size_t)n);fclose(f);memset(memory,77,16);resets=achievements=0;assert(Load("broken.khst")<=0);assert(memory[0]==77&&memory[15]==77);assert(!resets&&!achievements);}
 for(n=3;n<=6;n++){if(n==4)continue;failure=n;memset(memory,77,16);resets=achievements=0;assert(Load("state.khst")<=0);assert(memory[0]==77&&!resets&&!achievements);}failure=0;
 oversized=1;assert(Load("state.khst")==0);assert(memory[0]==77);oversized=0;
 old[8]^=1;f=fopen("broken.khst","wb");fwrite(old,1,28,f);fclose(f);assert(Load("broken.khst")==-1);assert(memory[0]==77);
 return 0;
}
'''
if __name__=='__main__':unittest.main()
