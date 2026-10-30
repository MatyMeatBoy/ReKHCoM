"""Execute the actual game-side direct input bridge with controlled port seams."""
import argparse
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
CODE=r'''
#include <stdio.h>
#include "types.h"
#include "sio/sio_api.h"
#include "port/port_online.h"
#include "port/port_netlink.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static ModernPeer peer;
static unsigned tickCount,pumpCount;
static int replay,reserved=1,engine=1;
static u16 hardware,keys0,keys1;
u32 gSioStatus,gSioPlayerId;
int PortOnlineModern(void){return engine;}
ModernPeer* PortOnlinePeer(void){return &peer;}
int PortOnlineError(void){return peer.error;}
void PortOnlineFail(int error){peer.error=(uint8_t)error;}
void PortOnlinePump(void){pumpCount++;}
void PortNetlinkFrame(void){tickCount++;}
int PortRollbackResimulating(void){return replay;}
int PortRollbackReserve(void){return reserved;}
int PortNetlinkFakeDesync(void){return 0;}
u16 GetKeysHeld(void){return hardware;}
void SioKeyStateUpdateA(u16 keys){keys0=keys;}
void SioKeyStateUpdateB(u16 keys){keys1=keys;}
u32 VsBattleStateHash(void){return 0;}
#include "src/sio/modern_inputs.inc"
static void connected(unsigned role){
    ModernInit(&peer,role,0,123);peer.ready=peer.peerReady=1;peer.session=123;
    gSioPlayerId=role;tickCount=pumpCount=0;replay=0;
}
int main(void){
    connected(0);CHECK(ModernGameBeginInputs(1));CHECK(ModernGameRollbackSession());
    gSioStatus=SIO_STAT_RECV_EMPTY;hardware=0x11;
    CHECK(ModernGameInputUpdate());CHECK(keys0==0x11&&keys1==0);CHECK(!(gSioStatus&SIO_STAT_RECV_EMPTY));
    CHECK(ModernGameFrame()==1&&peer.made==1&&tickCount==1);
    /* Restore the game counter as a snapshot does; the peer stays outside snapshots. */
    sModernFrame=0;replay=1;hardware=0x22;
    CHECK(ModernGameInputUpdate());CHECK(keys0==0x11&&ModernGameFrame()==1&&peer.made==1);
    CHECK(tickCount==1&&pumpCount==1);
    ModernGameEndInputs();CHECK(!ModernGameRollbackSession());
    connected(1);CHECK(ModernGameBeginInputs(2));hardware=0x44;
    CHECK(ModernGameInputUpdate());CHECK(keys0==0&&keys1==0x44);
    peer.rollbackPending=1;peer.rollback=0;CHECK(ModernGameTakeRollback()==0);
    CHECK(ModernGameTakeRollback()==0xffffffffu);
    ModernGameEndInputs();reserved=0;CHECK(!ModernGameBeginInputs(3));CHECK(peer.error==MODERN_ERR_STATE);
    CHECK(!ModernGameRollbackSession());
    return 0;
}
'''
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--cc',required=True);args=ap.parse_args()
    with tempfile.TemporaryDirectory(prefix='online-game-inputs-') as tmp:
        src,exe=Path(tmp)/'test.c',Path(tmp)/'test.exe';src.write_text(CODE)
        subprocess.run([args.cc,'-std=c99','-Wall','-Wextra','-Werror','-static','-DPLATFORM_PC',
                        '-I',str(ROOT),'-I',str(ROOT/'include'),'-I',str(ROOT/'port/include'),
                        str(src),str(ROOT/'port/src/port_online_protocol.c'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
    print('PASS production game input bridge: immediate keys, immutable replay, role ordering, no replay I/O, reserve failure')
if __name__=='__main__':main()
