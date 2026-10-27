"""Exercise unchanged production package reader; generated fixtures contain no game data."""
from pathlib import Path
import importlib.util, io, os, shutil, struct, subprocess, tempfile, zlib
ROOT=Path(__file__).resolve().parents[2]
def main():
 source=(ROOT/'port/decompui/hd_import.inc').read_text(encoding='utf-8')
 key=source[source.index('static const unsigned char kMaster'):source.index('static std::string Md5Hex')]
 reader=source[source.index('static bool ReadAsset('):source.index('\n/* ---- SCD')]
 spec=importlib.util.spec_from_file_location('pkg',ROOT/'port/tools/khhd_pkg.py');pkg=importlib.util.module_from_spec(spec);spec.loader.exec_module(pkg)
 raw=lambda d,n,c,body=b'':struct.pack('<iiii',d,n,c,0)+body
 content=b'owned fixture '*100
 packed=zlib.compress(content);seed=raw(len(content),0,len(packed));encrypted=pkg.decrypt_head(pkg.make_key(seed),packed)
 plain_seed=raw(4,0,-1)
 remastered=raw(4,1,-2)+struct.pack('<32siiii',b'extra',0,0,4,-2)+b'OWN!'+bytes(12)+b'EXTR'
 huge_seed=raw(2147483647,0,len(packed))
 fixtures=[('huge-decoded-size',huge_seed+pkg.decrypt_head(pkg.make_key(huge_seed),packed),False,0),('valid-encrypted-raw',plain_seed+pkg.decrypt_head(pkg.make_key(plain_seed),b'OWN!'),True,4),('valid-remastered',remastered,True,4),('valid-raw',raw(4,0,-2,b'OWN!'),True,4),('valid-empty',raw(0,0,-2),True,0),('valid-compressed',seed+encrypted,True,len(content)),('zero',b'',False,0),('truncated-header',b'abc',False,0),('negative-real',raw(-1,0,-1),False,0),('negative-count',raw(0,-1,-2),False,0),('negative-codec',raw(0,0,-3),False,0),('huge-real',raw(2147483647,0,-2),False,0),('huge-count',raw(0,2147483647,-2),False,0),('truncated-body',raw(7,0,-2,b'abc'),False,0),('short-compressed',raw(3,0,1,b'x'),False,0),('declared-mismatch',raw(len(content)+1,0,len(packed))+pkg.decrypt_head(pkg.make_key(raw(len(content)+1,0,len(packed))),packed),False,0)]
 prefix='''#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <memory>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "port/decompui/third_party/stb_image.h"
'''
 test=r'''
int main(int argc,char**argv){if(argc!=4)return 3;std::ifstream f(argv[1],std::ios::binary);std::vector<unsigned char> out={42};
try{f.setstate(std::ios::eofbit);bool ok=ReadAsset(f,0,out);bool expected=std::string(argv[2])=="1";if(ok!=expected || (ok?out.size()!=std::stoul(argv[3]):out!=std::vector<unsigned char>{42}))return 4;}
catch(const std::exception&e){std::cerr<<e.what();return 5;}
out={42};if(ReadAsset(f,-1,out) || out!=std::vector<unsigned char>{42})return 6;
if(ReadAsset(f,0x7fffffffffffffffLL,out) || out!=std::vector<unsigned char>{42})return 7;return 0;}
'''
 with tempfile.TemporaryDirectory(prefix='khcom-reader-') as folder:
  temp=Path(folder);unit=temp/'reader.cpp';unit.write_text(prefix+key+reader+test,encoding="utf-8")
  compiler=os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
  if not compiler:compiler=str(next((ROOT.parent.parent/'toolchain').glob('llvm-mingw-*/bin/clang++.exe')))
  exe=temp/('test.exe' if os.name=='nt' else 'test')
  subprocess.run([compiler,'-std=c++17','-static','-I'+str(ROOT),str(unit),'-o',str(exe)],check=True)
  failures=[]
  for name,data,ok,size in fixtures:
   file=temp/(name+'.pkg');file.write_bytes(data)
   run=subprocess.run([str(exe),str(file),str(int(ok)),str(size)],capture_output=True,text=True,timeout=5)
   print(name,run.returncode,run.stderr)
   if run.returncode:failures.append(name)
  assert not failures,failures
  for name,data,ok,size in fixtures:
   try:
    got,_,_=pkg.read_asset(io.BytesIO(data),0)
    assert ok and len(got)==size, ('Python reader accepted invalid/mismatched asset',name)
   except (ValueError,struct.error,zlib.error):
    assert not ok, ('Python reader rejected valid control',name)
  got,meta,extra=pkg.read_asset(io.BytesIO(remastered),0,with_remastered=True)
  assert got==b'OWN!' and extra=={'extra':b'EXTR'}
  for offset in [-1,len(remastered)+1]:
   try:pkg.read_asset(io.BytesIO(remastered),offset)
   except ValueError:pass
   else:raise AssertionError('Python accepted invalid offset')
  print('PASS native Python reader: same generated package corpus, remastered control and invalid offsets')
if __name__=='__main__':main()
