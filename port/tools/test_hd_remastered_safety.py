"""Remastered package contract: actual readers, no game inputs."""
from pathlib import Path
import hashlib,importlib.util,io,json,os,shutil,struct,subprocess,tempfile,zlib
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def main():
 pkg=module('pkg',ROOT/'port/tools/khhd_pkg.py')
 raw=lambda real,count,packed,body=b'':struct.pack('<iiii',real,count,packed,0)+body
 def package(base,extra=None,declared=None,extra_declared=None,compressed=False):
  content=zlib.compress(base) if compressed else base
  blob=zlib.compress(extra) if compressed and extra is not None else extra
  seed=raw(len(base) if declared is None else declared,int(extra is not None),len(content) if compressed else -2)
  key=pkg.make_key(seed)
  data=seed
  if extra is not None:data+=struct.pack('<32siiii',b'UK_MVS000.binl',0,0,len(extra) if extra_declared is None else extra_declared,len(blob) if compressed else -2)
  data+=pkg.decrypt_head(key,content) if compressed else content
  if extra is not None:
   data+=bytes(-len(data)%16);data+=pkg.decrypt_head(key,blob) if compressed else blob
  return data
 mvs=b'MVS\0'+struct.pack('<HHI',1,1,0x30)+bytes(4)+struct.pack('<HHI',0,30,24)+b'owned line\0'
 expanded=package(bytes(64)+mvs,mvs,declared=8,extra_declared=8,compressed=True)
 cases=[('valid-raw',package(b'OWN!'),True,4,0),('valid-empty',package(b''),True,0,0),('valid-extra',package(b'OWN!',b'EXTR'),True,4,4),('valid-extra-padding',package(b'OWN!',b'EXTR')+bytes(12),True,4,4),('valid-empty-extra',package(b'OWN!',b''),True,4,0),('valid-expanded-language',expanded,True,64+len(mvs),len(mvs)),('zero',b'',False,0,0),('short-header',b'abc',False,0,0),('short-compressed',raw(8,0,1,b'x'),False,0,0),('negative-base',raw(-1,0,-1),False,0,0),('negative-extra',raw(4,1,-2)+struct.pack('<32siiii',b'UK_MVS000.binl',0,0,-1,-1)+b'OWN!'+bytes(12),False,0,0),('truncated-extra',package(b'OWN!',b'EXTR')[:-2],False,0,0),('invalid-codec',raw(4,0,-3,b'OWN!'),False,0,0)]
 s=(ROOT/'port/decompui/hd_import.inc').read_text(encoding='utf-8');key=s[s.index('static const unsigned char kMaster'):s.index('static std::string Md5Hex')];reader=s[s.index('static bool ReadAssetExtras('):s.index("/* the game's own letters",s.index('static bool ReadAssetExtras('))]
 prefix='''#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <memory>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "port/decompui/third_party/stb_image.h"
'''
 test='''int main(int argc,char**argv){if(argc!=5)return 3;std::ifstream f(argv[1],std::ios::binary);std::vector<unsigned char> base={42};std::map<std::string,std::vector<unsigned char>> extras={{"keep",{43}}};const auto old=extras;
try{bool ok=ReadAssetExtras(f,0,base,extras),expected=argv[2][0]=='1';if(ok!=expected)return 4;if(!ok)return base!=std::vector<unsigned char>{42}||extras!=old?5:0;
if(base.size()!=std::stoul(argv[3]))return 6;
if(std::string(argv[1]).find("expanded-language")!=std::string::npos){std::map<std::string,std::vector<unsigned char>> languages;if(!ReadAssetLanguages(f,0,languages)||languages[""].size()!=35||languages["UK"].size()!=35)return 9;}int extra=std::stoi(argv[4]);if(extra && extras["UK_MVS000.binl"].size()!=(size_t)extra)return 7;}
catch(const std::exception&){return 8;}return 0;}'''
 with tempfile.TemporaryDirectory(prefix='khcom-remastered-') as folder:
  temp=Path(folder);unit=temp/'test.cpp';unit.write_text(prefix+key+reader+test,encoding='utf-8');compiler=os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
  if not compiler:compiler=str(next((ROOT.parent.parent/'toolchain').glob('llvm-mingw-*/bin/clang++.exe')))
  exe=temp/('test.exe' if os.name=='nt' else 'test');subprocess.run([compiler,'-std=c++17','-static','-I'+str(ROOT),str(unit),'-o',str(exe)],check=True)
  failures=[]
  for name,data,valid,base_size,extra_size in cases:
   file=temp/(name+'.pkg');file.write_bytes(data);p=subprocess.run([str(exe),str(file),str(int(valid)),str(base_size),str(extra_size)],timeout=3)
   print(name,'cpp',p.returncode)
   if p.returncode:failures.append((name,'cpp',p.returncode))
   try:
    base,_,extras=pkg.read_asset(io.BytesIO(data),0,with_remastered=True)
    if not valid or len(base)!=base_size or (extra_size and len(extras['UK_MVS000.binl'])!=extra_size):raise AssertionError(name)
   except (ValueError,struct.error,zlib.error):
    if valid:failures.append((name,'python','rejected valid'))
  assert not failures,failures
  install=temp/'owned-install';image=install/'Image/dt';image.mkdir(parents=True)
  (image/'Recom.pkg').write_bytes(expanded)
  (image/'Recom.hed').write_bytes(struct.pack('<16sqii',hashlib.md5(b'FORM/5000/FO5000.BIN').digest(),0,len(expanded),99))
  subs=module('subs',ROOT/'port/tools/khhd_subs.py');data=temp/'data'
  assert subs.extract(install,data)==2
  for lang in ['us','uk']:assert (data/f'hd/subs/ROP.{lang}.txt').read_text(encoding='utf-8')=='0 30 owned line\n'
  print('PASS real ReadAssetLanguages and Python subtitle extractor: expanded base/UK tables retain complete cue text')
if __name__=='__main__':main()
