/* Appended to the extracted production affine sampler by test_halloween_floor.py.
 * Synthetic VRAM only; no ROM, renderer, GPU or gameplay claims. */
int main(void) {
 int x,y;uint16_t before[640],straight[640];
 gPortScreen.width=640;gPortScreen.originX=200;gPortScreen.wide=1;sLeft=-200;sRight=440;sBattleView=1;
 for(x=0;x<256;x++){palette[x]=(uint16_t)x;for(int p=0;p<64;p++)vram[x*64+p]=(uint8_t)(1+(p/8+x)%254);}
 for(x=0;x<4096;x++)vram[0x6000+x]=(uint8_t)(x%256);
 IO16(0x0c)=0x8c00;IO16(0x20)=256;IO16(0x24)=0;
 /* The current bottom clamp produces identical rows, irrespective of texture depth. */
 sAffX[0]=128*256;sAffY[0]=512*256;DrawAffineBg(2,170,1);memcpy(before,sBgLine[2],sizeof(before));
 sAffY[0]=540*256;DrawAffineBg(2,198,1);
 CHECK(memcmp(before,sBgLine[2],sizeof(before))!=0);
 /* The optimized identity transform and scalar affine path agree. */
 memcpy(straight,sBgLine[2],sizeof(straight));
 IO16(0x0c)|=64;IO16(0x4c)=1;DrawAffineBg(2,198,1);
 for(x=0;x<640;x+=2)CHECK(straight[x]==sBgLine[2][x]);
 IO16(0x0c)&=~64;IO16(0x4c)=0;
 /* Other arenas retain their prior edge; no selector means no projection. */
 sHalloweenFloor=0;sAffY[0]=512*256;DrawAffineBg(2,170,1);memcpy(before,sBgLine[2],sizeof(before));
 sAffY[0]=540*256;DrawAffineBg(2,198,1);CHECK(memcmp(before,sBgLine[2],sizeof(before))==0);
 /* Every authored row and all 160 native viewport rows remain identical. */
 for(y=0;y<512;y++){
  sAffY[0]=y*256;sHalloweenFloor=0;DrawAffineBg(2,180,1);memcpy(before,sBgLine[2],sizeof(before));
  sHalloweenFloor=1;DrawAffineBg(2,180,1);CHECK(memcmp(before,sBgLine[2],sizeof(before))==0);
 }
 for(y=0;y<160;y++){
  sAffY[0]=(512+y)*256;sHalloweenFloor=0;DrawAffineBg(2,y,1);memcpy(before,sBgLine[2],sizeof(before));
  sHalloweenFloor=1;DrawAffineBg(2,y,1);CHECK(memcmp(before,sBgLine[2],sizeof(before))==0);
 }
 /* Added ground comes exclusively from the clean authored floor, including
  * large/negative wide coordinates. It never samples wall or padding. */
 for(y=512;y<8192;y+=7)for(x=-4096;x<4096;x+=13){
  int sx=x,sy=y;CHECK(PortHalloweenFloorSource(&sx,&sy));
  CHECK(sx>=64&&sx<448&&sy>=400&&sy<464);
 }
 x=24;y=511;CHECK(!PortHalloweenFloorSource(&x,&y)&&x==24&&y==511);
 /* Sampling resolves live palette inks, including complete dark fade. */
 memset(palette,0,sizeof(palette));sAffY[0]=540*256;DrawAffineBg(2,180,1);
 for(x=0;x<640;x++)CHECK(sBgLine[2][x]==0);
 /* Wrong layer/register layout cannot opt into the arena floor. */
 IO16(0x0c)=0x8000;sHalloweenFloor=0;DrawAffineBg(2,180,1);memcpy(before,sBgLine[2],sizeof(before));
 sHalloweenFloor=1;DrawAffineBg(2,180,1);CHECK(memcmp(before,sBgLine[2],sizeof(before))==0);
 /* Actual BeginFrame selector requires all loaded bytes and battle/wide. */
 memcpy(gBtlBgHalloweenTownTiles,vram,16384);MatchLoadedFloor();CHECK(sHalloweenFloor);
 vram[16383]^=1;MatchLoadedFloor();CHECK(!sHalloweenFloor);vram[16383]^=1;
 sBattleView=0;MatchLoadedFloor();CHECK(!sHalloweenFloor);sBattleView=1;
 gPortScreen.wide=0;MatchLoadedFloor();CHECK(!sHalloweenFloor);gPortScreen.wide=1;
 MatchLoadedFloor();CHECK(sHalloweenFloor);
 puts("Halloween affine production: depth, fast/scalar, native512rows+160viewport, other arena, donor bounds, live palette, exact source selector PASS");return 0;
}
