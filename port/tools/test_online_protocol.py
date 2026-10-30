"""Execute the pure modern protocol under a deterministic simulated network."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]

CODE=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "port/port_online_protocol.h"
#undef assert
#define assert(x) do { if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;} }while(0)
typedef struct {uint8_t data[512];size_t size;unsigned target;uint32_t due;} Packet;
static ModernPeer p[2];static Packet packets[32768];static unsigned count,from;
static uint32_t now,delay;static unsigned dropEvery,sent,jitter,asymmetry,burst;
static uint64_t wireBytes;
static void send_packet(void*context,const uint8_t*data,size_t size){
    (void)context;
    wireBytes+=size;
    if(size>512||count>=32768){fprintf(stderr,"unbounded packet\n");return;}
    sent++;if(dropEvery&&sent%dropEvery==0)return;
    if(burst&&sent%200<40)return;
    int transit=(int)delay+(from?(int)asymmetry:-(int)asymmetry);
    if(jitter)transit+=(int)((sent*37u)%(jitter*2+1))-(int)jitter;
    if(transit<0)transit=0;
    Packet*q=&packets[count++];memcpy(q->data,data,size);q->size=size;q->target=from^1;q->due=now+(unsigned)transit;
}
static void pump(void){
    for(unsigned i=0;i<2;i++){from=i;ModernPump(&p[i],now,send_packet,0);}
    for(unsigned i=0;i<count;){
        if((int32_t)(now-packets[i].due)>=0){Packet q=packets[i];packets[i]=packets[--count];ModernReceive(&p[q.target],q.data,q.size,now);}
        else i++;
    }
}
static int connect_peers(unsigned d,unsigned drops){
    now=0;count=sent=jitter=asymmetry=burst=0;delay=d;dropEvery=drops;
    ModernInit(&p[0],0,0,0x123456789abcdef0ULL);ModernInit(&p[1],1,0,0);
    for(now=0;now<5000;now++)pump();
    return ModernConnected(&p[0])&&ModernConnected(&p[1]);
}
static int inputs(void){
    assert(connect_peers(120,0));
    ModernStartInputs(&p[0],1,0);ModernStartInputs(&p[1],1,0);
    uint16_t a,b;assert(ModernInput(&p[0],0,0x11,&a,&b));assert(a==0x11&&b==0);
    assert(ModernInput(&p[0],0,0x22,&a,&b));assert(a==0x11); /* immutable local frame */
    assert(ModernInput(&p[1],0,0x42,&a,&b));assert(a==0&&b==0x42);
    for(unsigned i=0;i<300;i++){now++;pump();}
    uint32_t f;assert(ModernTakeRollback(&p[0],&f)&&f==0);assert(!ModernTakeRollback(&p[0],&f));
    assert(ModernInput(&p[0],0,0xffff,&a,&b));assert(a==0x11&&b==0x42);
    assert(p[0].made==1&&p[1].made==1);assert(!p[0].error&&!p[1].error);
    return 0;
}
static int controls(void){
    assert(connect_peers(150,7));
    for(unsigned i=0;i<32;i++){uint32_t seq;uint8_t data=(uint8_t)i;assert(ModernQueueControl(&p[0],2,i,&data,1,&seq));}
    assert(!ModernQueueControl(&p[0],2,99,0,0,0));
    unsigned got=0;ModernControl c;
    for(unsigned t=0;t<15000;t++){
        now++;pump();
        while(ModernReadControl(&p[1],&c)){assert(c.kind==2&&c.id==got&&c.size==1&&c.data[0]==got);got++;}
    }
    assert(got==32&&p[0].txCount==0);assert(ModernControlAcked(&p[0],32));
    assert(!ModernQueueControl(&p[0],2,0,0,401,0));
    return 0;
}
static int delayed_frames(unsigned rtt,unsigned drops,uint32_t base,unsigned impaired){
    assert(connect_peers(rtt/2,drops));
    wireBytes=0;unsigned firstSent=sent;
    if(impaired==1){jitter=35;asymmetry=75;burst=1;}
    ModernStartInputs(&p[0],3,base);ModernStartInputs(&p[1],3,base);
    for(uint32_t f=0;f<1200;f++){
        uint16_t a,b;
        assert(ModernInput(&p[0],base+f,(uint16_t)(f%1024),&a,&b));assert(a==(f%1024));
        assert(ModernInput(&p[1],base+f,(uint16_t)((f*7)%1024),&a,&b));assert(b==((f*7)%1024));
        if(impaired){now+=17;pump();} /* Real adapter is pumped at render-frame cadence. */
        else for(unsigned t=0;t<17;t++){now++;pump();}
        uint32_t to;ModernTakeRollback(&p[0],&to);ModernTakeRollback(&p[1],&to);
        assert(!p[0].error&&!p[1].error);
    }
    for(unsigned t=0;t<3000;t++){now++;pump();}
    assert(p[0].remoteNext==base+1200&&p[1].remoteNext==base+1200);
    assert(p[0].localAck==base+1200&&p[1].localAck==base+1200);
    assert(p[0].corrections&&p[1].corrections);
    printf("METRIC {\"rtt_ms\":%u,\"drop_every\":%u,\"asymmetric_jitter_burst\":%u,\"frames_per_peer\":1200,\"base_frame\":%u,\"bytes_both_peers\":%llu,\"packets_both_peers\":%u}\n",
           rtt,drops,impaired==1,base,(unsigned long long)wireBytes,sent-firstSent);
    return 0;
}
static int control_wrap(void){
    assert(connect_peers(150,11));
    const uint32_t first=0xfffffff0u;
    p[0].txNext=p[1].rxNext=first;p[0].txAck=first-1;
    for(unsigned i=0;i<32;i++){uint32_t sequence;uint8_t payload=(uint8_t)i;
        assert(ModernQueueControl(&p[0],2,i,&payload,1,&sequence));assert(sequence==first+i);
    }
    unsigned got=0;ModernControl c;
    for(unsigned t=0;t<15000;t+=17){now+=17;pump();
        while(ModernReadControl(&p[1],&c)){assert(c.id==got&&c.sequence==first+got&&c.data[0]==got);got++;}
    }
    assert(got==32&&p[0].txCount==0&&ModernControlAcked(&p[0],first+31));
    assert(!p[0].error&&!p[1].error);return 0;
}
static int malformed(void){
    assert(connect_peers(0,0));
    uint8_t raw[600]={0};uint64_t nonce=p[0].session;
    for(unsigned len=0;len<600;len++)ModernReceive(&p[0],raw,len,now);
    assert(p[0].session==nonce&&!p[0].error);
    ModernStartInputs(&p[0],1,0);uint16_t a,b;assert(!ModernInput(&p[0],1,0,&a,&b));
    assert(ModernInput(&p[0],0,1,&a,&b));
    /* With no peer acknowledgement, stop explicitly before overwriting history.
       The caller receives an error, never a blocking wait or false success. */
    for(unsigned f=1;f<256;f++)assert(ModernInput(&p[0],f,1,&a,&b));
    assert(!ModernInput(&p[0],256,1,&a,&b));assert(p[0].error==MODERN_ERR_HISTORY);
    return 0;
}
static int hashes(void){
    assert(connect_peers(120,0));
    ModernStartInputs(&p[0],7,0);ModernStartInputs(&p[1],7,0);
    uint16_t a,b;
    for(unsigned f=0;f<150;f++){
        assert(ModernInput(&p[0],f,0,&a,&b));assert(ModernInput(&p[1],f,0,&a,&b));
        if(f==120){ModernSetHash(&p[0],f,123);ModernSetHash(&p[1],f,123);}
        for(unsigned t=0;t<17;t++){now++;pump();}
    }
    for(unsigned t=0;t<2000;t++){now++;pump();}
    assert(p[0].checks==1&&p[1].checks==1&&!p[0].error&&!p[1].error);
    ModernSetHash(&p[0],121,111);ModernSetHash(&p[1],121,222);
    for(unsigned t=0;t<2000;t++){now++;pump();}
    assert(p[0].error==MODERN_ERR_DESYNC&&p[1].error==MODERN_ERR_DESYNC);
    return 0;
}
static void le32(uint8_t* d,uint32_t v){for(unsigned i=0;i<4;i++)d[i]=(uint8_t)(v>>(8*i));}
static void header(uint8_t* d,unsigned type){
    memset(d,0,64);memcpy(d,"KHM2",4);d[4]=4;d[5]=(uint8_t)type;d[6]=1;
    for(unsigned i=0;i<8;i++)d[8+i]=(uint8_t)(p[0].session>>(8*i));
}
static int hostile_and_stale(void){
    uint8_t raw[64];uint16_t a,b;uint32_t before;
    assert(connect_peers(0,0));ModernStartInputs(&p[0],1,0);
    for(unsigned f=0;f<200;f++)assert(ModernInput(&p[0],f,1,&a,&b));
    /* A future input sharing a ring slot must not erase an unconfirmed local key. */
    header(raw,MODERN_INPUT);le32(raw+24,1);raw[36]=1;le32(raw+37,256);raw[41]=1;raw[42]=2;
    ModernReceive(&p[0],raw,44,now);
    assert(p[0].inputs[0].frame==0&&p[0].inputs[0].localValid);
    assert(ModernInput(&p[0],0,999,&a,&b)&&a==1);
    /* Stale nonce, wrong role and truncated payload cannot mutate acknowledgements. */
    before=p[0].txAck;raw[8]^=1;le32(raw+20,123);ModernReceive(&p[0],raw,44,now);
    assert(p[0].txAck==before);raw[8]^=1;raw[6]=0;ModernReceive(&p[0],raw,44,now);
    assert(p[0].txAck==before);raw[6]=1;ModernReceive(&p[0],raw,43,now);assert(p[0].txAck==before);
    /* Delayed packets from a prior session cannot fail the live one by version/region. */
    header(raw,MODERN_HELLO);raw[8]^=1;raw[4]=3;raw[7]=2;
    ModernReceive(&p[0],raw,36,now);assert(!p[0].error);
    header(raw,MODERN_HELLO);raw[4]=3;ModernReceive(&p[0],raw,36,now);
    assert(p[0].error==MODERN_ERR_VERSION);
    assert(connect_peers(0,0));header(raw,MODERN_HELLO);raw[7]=2;
    ModernReceive(&p[0],raw,36,now);assert(p[0].error==MODERN_ERR_REGION);
    /* Receiving acknowledgements is insufficient if remote input itself is absent. */
    assert(connect_peers(0,0));ModernStartInputs(&p[0],1,0);
    for(unsigned f=0;f<256;f++){p[0].localAck=f;assert(ModernInput(&p[0],f,1,&a,&b));}
    p[0].localAck=256;assert(!ModernInput(&p[0],256,1,&a,&b));assert(p[0].error==MODERN_ERR_HISTORY);
    return 0;
}
static int pending_hash(void){
    assert(connect_peers(120,0));
    ModernStartInputs(&p[0],7,0);ModernStartInputs(&p[1],7,0);
    uint16_t a,b;
    assert(ModernInput(&p[0],0,1,&a,&b));assert(ModernInput(&p[1],0,2,&a,&b));
    ModernSetHash(&p[0],1,111);ModernSetHash(&p[1],1,222);
    for(unsigned t=0;t<2000;t++){now++;pump();}
    /* Both hashes describe speculative state until the queued correction replays. */
    assert(p[0].rollbackPending&&p[1].rollbackPending);
    assert(!p[0].error&&!p[1].error&&p[0].checks==0&&p[1].checks==0);
    uint32_t f;ModernTakeRollback(&p[0],&f);ModernTakeRollback(&p[1],&f);
    for(unsigned t=0;t<1000;t++){now++;pump();}
    assert(!p[0].error&&!p[1].error&&p[0].checks==0&&p[1].checks==0);
    ModernSetHash(&p[0],1,333);ModernSetHash(&p[1],1,333);
    ModernFinishRollback(&p[0]);ModernFinishRollback(&p[1]);
    for(unsigned t=0;t<2000;t++){now++;pump();}
    assert(!p[0].error&&!p[1].error&&p[0].checks==1&&p[1].checks==1);
    return 0;
}
static int acknowledged_tail(void){
    assert(connect_peers(0,0));ModernStartInputs(&p[0],1,0);ModernStartInputs(&p[1],1,0);
    uint16_t a,b;
    for(unsigned f=0;f<40;f++){
        assert(ModernInput(&p[0],f,1,&a,&b));assert(ModernInput(&p[1],f,2,&a,&b));now+=17;pump();
    }
    for(unsigned t=0;t<400;t++){now++;pump();}
    assert(p[0].localAck==40);count=0;from=0;now+=101;ModernPump(&p[0],now,send_packet,0);
    unsigned checked=0;
    for(unsigned i=0;i<count;i++)if(packets[i].data[5]==MODERN_INPUT){
        assert(packets[i].size==37&&packets[i].data[36]==0);checked++;
    }
    assert(checked==1);return 0;
}
#ifdef MODERN_BENCHMARK
int main(void){return delayed_frames(24,0,0,2)||delayed_frames(240,0,0,1)||delayed_frames(300,13,0,1);}
#else
int main(void){
    if(inputs()||controls()||control_wrap()||delayed_frames(240,0,0,0)||delayed_frames(300,11,0,0)||
       delayed_frames(300,13,0xfffffff0,0)||delayed_frames(300,13,0,1)||malformed()||hashes()||hostile_and_stale()||pending_hash()||acknowledged_tail())return 1;
    puts("PASS modern core: direct/immutable input, earliest rollback, ordered reliable controls including sequence wrap, bounded history, 240/300ms loss, asymmetric jitter/burst loss at frame cadence, malformed packets and confirmed hashes");
    return 0;
}
#endif
'''

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--cc',required=True)
    ap.add_argument('--core',type=Path,default=ROOT/'port/src/port_online_protocol.c')
    ap.add_argument('--report',type=Path);ap.add_argument('--benchmark',action='store_true');a=ap.parse_args()
    with tempfile.TemporaryDirectory(prefix='modern-core-') as tmp:
        source,exe=Path(tmp)/'test.c',Path(tmp)/'test.exe';source.write_text(CODE)
        subprocess.run([a.cc,'-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-static',
                        *(['-DMODERN_BENCHMARK','-Wno-unused-function'] if a.benchmark else []),
                        '-I',str(ROOT/'port/include'),str(source),str(a.core),'-o',str(exe)],check=True)
        result=subprocess.run([str(exe)],capture_output=True,text=True)
        print(result.stdout,end='')
        if result.stderr:print(result.stderr,end='')
        result.check_returncode()
        if a.report:
            a.report.write_text(json.dumps({'core_sha256':hashlib.sha256(a.core.read_bytes()).hexdigest(),
                'compiler':a.cc,'benchmark':a.benchmark,
                'metrics':[json.loads(line[7:]) for line in result.stdout.splitlines() if line.startswith('METRIC ')]},indent=2)+'\n')

if __name__=='__main__':main()
