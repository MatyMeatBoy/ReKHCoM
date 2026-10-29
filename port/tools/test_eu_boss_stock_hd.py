#!/usr/bin/env python3
"""Actual HD lookup must match EU boss stock tuples and preserve precise fallback."""
from pathlib import Path
import argparse,os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
CASES=[(family,letter)for family,letters in [('Axel','AB'),('Hades','AB'),('Larxene','ABC'),('Riku','ABCD')]for letter in letters]

def block(s,m):
    start=s.index(m);brace=s.index('{',start);depth=1;end=brace+1
    while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
    return s[start:end]

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--cc',default=os.environ.get('CC','cc'));a=ap.parse_args()
    text=(ROOT/'port/src/port_hdsprites.c').read_text(encoding='utf-8');find=block(text,'int PortHdSpriteFind(')
    header=ROOT/'port/include/port/port_eu_boss_stock_alias.h'
    helper=header.read_text(encoding='utf-8')if header.exists()else''
    families=['Axel','Hades','Larxene','Riku'];names=[f'g{f}{l}SmallCard{x}'for f,l in CASES+[(f,'')for f in families]for x in ['Tiles','Frame0']]
    pre='#include <stddef.h>\n#include <stdio.h>\n#include <assert.h>\n'
    pre+='\n'.join('const unsigned char '+n+'[1]={0};'for n in names)
    pre+='\nconst unsigned char unrelated[1]={0};\n'
    pre+='\n'.join('enum { TARGET_'+f+'='+str(i+1)+' };'for i,f in enumerate(families))
    pre+='\nstruct {int hdCards,hdTitle;}gPortConfig;\n'
    pre+='typedef struct {const void* tiles;const void* frame;int group;}HdSpriteDef;\n'
    pre+='#define HD_SPRITE_COUNT 4\nstatic int ready[4]={1,1,1,1};\n'
    pre+='static const HdSpriteDef kHdSprites[]={' + ','.join('{g'+f+'SmallCardTiles,g'+f+'SmallCardFrame0,6}'for f in families)+'};\n'
    pre+='static int GroupOn(int group){return group==6&&gPortConfig.hdCards;}\nstatic int SpriteReady(int i){return ready[i];}\n'
    run='int main(void){int failures=0;gPortConfig.hdCards=1;assert(!PortHdSpriteFind(NULL,NULL));\n'
    for f,l in CASES:
        alias=f'g{f}{l}SmallCard';canonical=f'g{f}SmallCard'
        run+=f'if(PortHdSpriteFind({alias}Tiles,{alias}Frame0)!=EXPECT_EU*TARGET_{f}){{fprintf(stderr,"missing {alias}\\n");failures++;}}\n'
        run+=f'assert(!PortHdSpriteFind({alias}Tiles,unrelated));assert(!PortHdSpriteFind(unrelated,{alias}Frame0));assert(!PortHdSpriteFind({alias}Tiles,{canonical}Frame0));\n'
        run+=f'assert(PortHdSpriteFind({canonical}Tiles,{canonical}Frame0)==TARGET_{f});\n'
        run+=f'gPortConfig.hdCards=0;gPortConfig.hdTitle=1;assert(!PortHdSpriteFind({alias}Tiles,{alias}Frame0));gPortConfig.hdCards=1;\n'
        run+=f'ready[TARGET_{f}-1]=0;assert(!PortHdSpriteFind({alias}Tiles,{alias}Frame0));ready[TARGET_{f}-1]=1;\n'
    run+='return failures?1:0;}\n'
    with tempfile.TemporaryDirectory(prefix='eu-boss-stock-')as td:
        d=Path(td)
        for region in ['EU','US','JP']:
            src=d/f'{region}.c';exe=d/f'{region}.exe';src.write_text(pre+f'\n#define VERSION_{region}\n#define EXPECT_EU '+str(int(region=='EU'))+'\n'+helper+'\n'+find+'\n'+run,encoding='utf-8')
            subprocess.run([a.cc,'-std=c11','-O0',str(src),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
    print('PASS11 EU exact stock aliases, wrong frames/tiles, HDoff, missing texture, canonical IDs reused, US/JP unchanged')
if __name__=='__main__':main()
