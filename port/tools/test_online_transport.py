"""Compile the production transport adapter against deterministic datagram seams."""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CODE = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port/port_online.h"
#include "port/port_netlink.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint32_t now;
static int role=1, incomingSize;
static unsigned sends;
static uint8_t incoming[512];
static ModernPeer other;
const char* PortConfigPath(const char* name){return name;}
int PortNetlinkRole(void){return role;}
uint32_t PortNetlinkMs(void){return now;}
void PortLog(const char* format,...){(void)format;}
uint64_t SDL_GetPerformanceCounter(void){return 123456789;}
static void peerSend(void* ctx,const uint8_t* data,size_t size){
    (void)ctx;memcpy(incoming,data,size);incomingSize=(int)size;
}
void PortNetlinkSend(const uint8_t* data,int size){
    sends++;ModernReceive(&other,data,(size_t)size,now);
}
int PortNetlinkRecv(uint8_t* data,int cap){
    int size=incomingSize;if(!size)return 0;
    if(size>cap)abort();memcpy(data,incoming,size);incomingSize=0;return size;
}
int main(int argc,char** argv){
    CHECK(argc==2);CHECK(PortOnlineModern()==atoi(argv[1]));
    CHECK(PortOnlineVersion()==(atoi(argv[1])?4:3));
    if(!atoi(argv[1])){PortOnlinePump();CHECK(sends==0);return 0;}
    ModernInit(&other,1,0,0);
    for(now=0;now<3000;now++){
        PortOnlinePump();ModernPump(&other,now,peerSend,0);
    }
    CHECK(PortOnlinePeer());CHECK(ModernConnected(PortOnlinePeer()));CHECK(ModernConnected(&other));
    uint8_t payload=42;CHECK(ModernQueueControl(PortOnlinePeer(),1,123,&payload,1,0));
    for(unsigned i=0;i<1000;i++,now++){PortOnlinePump();ModernPump(&other,now,peerSend,0);}
    ModernControl c;CHECK(ModernReadControl(&other,&c));CHECK(c.id==123&&c.data[0]==42);
    PortOnlineFail(MODERN_ERR_STATE);CHECK(PortOnlineError()==MODERN_ERR_STATE);
    PortOnlineReset();CHECK(!PortOnlineError());
    role=0;PortOnlinePump();CHECK(!PortOnlinePeer());
    return 0;
}
'''


def main():
    import os
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='online-transport-') as tmp:
        work = Path(tmp)
        (work/'port').mkdir()
        (work/'SDL3').mkdir()
        (work/'port/port.h').write_text('#include <stdint.h>\nconst char* PortConfigPath(const char*);\nvoid PortLog(const char*,...);\n')
        (work/'SDL3/SDL.h').write_text('#include <stdint.h>\nuint64_t SDL_GetPerformanceCounter(void);\n')
        src, exe = work/'consumer.c', work/'consumer.exe'
        src.write_text(CODE)
        subprocess.run([args.cc,'-std=c99','-Wall','-Wextra','-Werror','-static','-I',str(work),
                        '-I',str(ROOT/'port/include'),str(src),str(ROOT/'port/src/port_online.c'),
                        str(ROOT/'port/src/port_online_protocol.c'),'-o',str(exe)],check=True)
        for setting, expected in [('classic',0),('modern',1),('invalid',0)]:
            subprocess.run([str(exe),str(expected)],cwd=work,
                           env={**os.environ,'KHCOM_ONLINE_ENGINE':setting},check=True)
        for setting, expected in [('classic',0),('modern',1)]:
            (work/'online-engine.cfg').write_text(setting+'\n')
            env=dict(os.environ);env.pop('KHCOM_ONLINE_ENGINE',None)
            subprocess.run([str(exe),str(expected)],cwd=work,env=env,check=True)
    print('PASS production transport: config/env, classic isolation, modern handshake/control/reset')


if __name__ == '__main__':
    main()
