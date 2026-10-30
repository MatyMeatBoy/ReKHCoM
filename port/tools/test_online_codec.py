"""Exercise actual game profile/deck structures through explicit wire encoders."""
import argparse
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
CODE = r'''
#include <stdio.h>
#include <string.h>
#include "sio/chara_types.h"
#include "card/card_types.h"
#include "port/port_online_codec.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    CharaLinkData p={0},q={0};Deck a={0},b={0};uint8_t data[400];
    p.hp=123;p.maxHp=456;p.level=99;p.winCount=321;p.loseCount=654;p.ap=88;
    p.learnedStocks=0x123456789abcdef0ULL;p.learnedStocks2=0xfedcba9876543210ULL;p.worldFlags=8191;p.seed=0x789a;
    CHECK(ModernEncodeProfile(data,sizeof(data),&p)==32);
    CHECK(data[0]==123&&data[12]==0xf0&&data[31]==0x78);
    CHECK(ModernDecodeProfile(&q,data,32));CHECK(!memcmp(&p,&q,sizeof(p)));
    for(unsigned n=0;n<32;n++)CHECK(!ModernDecodeProfile(&q,data,n));
    CHECK(!ModernDecodeProfile(&q,data,33));CHECK(!ModernEncodeProfile(data,31,&p));
    p.level=100;CHECK(!ModernEncodeProfile(data,sizeof(data),&p));
    for(unsigned i=0;i<99;i++)a.cards[i]=(u16)(0x8000+i);memcpy(a.name,"ABCDEFGHIJKLMNOPQRST",20);
    a.cardCount=99;a.cpCost=1500;a.unk_DE=123;
    CHECK(ModernEncodeDeck(data,sizeof(data),&a)==224);CHECK(data[1]==0x80&&data[218]==0xdc);
    CHECK(ModernDecodeDeck(&b,data,224));CHECK(!memcmp(&a,&b,sizeof(a)));
    for(unsigned n=0;n<224;n++)CHECK(!ModernDecodeDeck(&b,data,n));
    CHECK(!ModernDecodeDeck(&b,data,225));CHECK(!ModernEncodeDeck(data,223,&a));
    data[220]=100;CHECK(!ModernDecodeDeck(&b,data,224));
    return 0;
}
'''
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cc',required=True);args=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='online-codec-') as tmp:
        src,exe=Path(tmp)/'test.c',Path(tmp)/'test.exe';src.write_text(CODE)
        subprocess.run([args.cc,'-std=c99','-Wall','-Wextra','-Werror','-static','-DPLATFORM_PC',
                        '-I',str(ROOT/'include'),'-I',str(ROOT/'port/include'),str(src),
                        str(ROOT/'port/src/port_online_codec.c'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
    print('PASS profile/deck explicit wire roundtrip, short/oversize/invalid rejection')
if __name__=='__main__':main()
