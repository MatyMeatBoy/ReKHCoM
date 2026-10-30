"""Compile the production NetWait consumer with a controlled millisecond clock."""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cc', required=True)
    a = ap.parse_args()
    source = (ROOT/'src/sio/sio.c').read_text()
    start = source.index('static void NetWait(void) {')
    end = source.index('\n}\n', start)+2
    code = r'''
#include <stdint.h>
#include <assert.h>
#undef assert
#define assert(x) do { if(!(x)) return __LINE__; } while(0)
typedef uint32_t u32;typedef int32_t s32;
static struct {u32 waitStart,mine,acked,theirs,giveUpHold;int dropped;} sNet;
static u32 clockNow,sLinkFrame,sNetLastFirst,sNetLastCount,sNetSlotsIn;
static u32 PortNetlinkMs(void){return clockNow;}
static int PortNetlinkGiveUpSeconds(void){return 25;}
static int PortInputKeys(void){return 0;}
static void PortUiLinkWaitingNotice(int a,int b){}
static void PortLog(const char*format,...){}
static void NetSendSlots(void){}static void PortLinkIdle(void){}static void NetReceive(void){}
FUNCTION
int main(void){
    clockNow=100;NetWait();assert(!sNet.dropped);
    clockNow=101;NetWait();assert(!sNet.dropped);
    clockNow=25101;NetWait();assert(!sNet.dropped);
    clockNow=25102;NetWait();assert(sNet.dropped);
    sNet.waitStart=0;sNet.dropped=0;clockNow=0;NetWait();assert(!sNet.dropped);
    sNet.waitStart=0;sNet.dropped=0;clockNow=0xfffffffe;NetWait();assert(!sNet.dropped);
    clockNow=100;NetWait();assert(!sNet.dropped);
    clockNow=26000;NetWait();assert(sNet.dropped);
    return 0;
}
'''.replace('FUNCTION', source[start:end])
    with tempfile.TemporaryDirectory(prefix='netlink-wait-') as tmp:
        cpp, exe = Path(tmp)/'wait.c', Path(tmp)/'wait.exe'
        cpp.write_text(code)
        subprocess.run([a.cc, '-static', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print('PASS: production NetWait even/odd/zero/expiry/wrap clock cases')


if __name__ == '__main__':
    main()
