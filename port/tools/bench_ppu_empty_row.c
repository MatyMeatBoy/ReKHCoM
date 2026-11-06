/* Synthetic row microbenchmark, not a game FPS benchmark.
 * Prepared only: do not compile/run until the user's build restriction lifts.
 * Example: cc -O2 -Iport/include port/tools/bench_ppu_empty_row.c -o bench-row
 * Compare both methods with identical data; five rounds alternate ordering. */
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "port/port_ppu_tile_row.h"

enum { ROWS=4096, PASSES=256, TRANSPARENT=0x8000 };
static uint8_t rows[ROWS][4];
static uint16_t colors[16], out[8];
typedef void (*Decode)(const uint8_t*, int, int, int);
static volatile uint64_t checksum;

static void scalar(const uint8_t* pixels,int xx,int step,int run) {
    int j;
    for (j=0;j<run;j++,xx+=step) {
        int c=(pixels[xx>>1]>>((xx&1)*4))&15;
        out[j]=c ? colors[c]&0x7fff : TRANSPARENT;
    }
}
static void empty_first(const uint8_t* pixels,int xx,int step,int run) {
    int j;
    if (PortPpu4bppRowEmpty(pixels)) {
        for (j=0;j<run;j++) out[j]=TRANSPARENT;
    } else scalar(pixels,xx,step,run);
}
static double measure(Decode method,uint64_t* sum) {
    Decode volatile selected=method;
    clock_t start=clock();
    int pass,r;
    checksum=0;
    for (pass=0;pass<PASSES;pass++) for (r=0;r<ROWS;r++) {
        int run=(r&7)+1,start=(r>>3)%(9-run),flip=(r>>6)&1;
        selected(rows[r],flip?7-start:start,flip?-1:1,run);
        checksum+=out[(pass+r)%run];
    }
    *sum=checksum;
    return (double)(clock()-start)/CLOCKS_PER_SEC;
}
int main(void) {
    static const int mixes[]={0,50,90,100};
    unsigned seed=1;
    int mix,round,r,k;
    for (k=0;k<16;k++) colors[k]=(uint16_t)(0x7000+k*137);
    puts("microbenchmark only: no game frames, no GPU/PPU pipeline measurement");
    puts("target_zero_percent,zero_rows,total_rows,round,scalar_seconds,empty_first_seconds,checksum");
    for (mix=0;mix<4;mix++) {
        int zero_rows=0;
        uint64_t warm_a,warm_b;
        for (r=0;r<ROWS;r++) for (k=0;k<4;k++) {
            seed=seed*1664525u+1013904223u;
            rows[r][k]=r%100<mixes[mix]?0:(uint8_t)(seed>>24);
        }
        for (r=0;r<ROWS;r++) zero_rows+=PortPpu4bppRowEmpty(rows[r]);
        (void)measure(scalar,&warm_a);(void)measure(empty_first,&warm_b);
        if (warm_a!=warm_b) { fputs("warmup checksum mismatch\n",stderr);return 1; }
        for (round=0;round<5;round++) {
            uint64_t a,b;double ta,tb;
            if (round&1) { tb=measure(empty_first,&b);ta=measure(scalar,&a); }
            else { ta=measure(scalar,&a);tb=measure(empty_first,&b); }
            if (a!=b) { fputs("pixel checksum mismatch\n",stderr);return 1; }
            printf("%d,%d,%d,%d,%.9f,%.9f,%llu\n",mixes[mix],zero_rows,ROWS,round,ta,tb,(unsigned long long)a);
        }
    }
    return 0;
}
