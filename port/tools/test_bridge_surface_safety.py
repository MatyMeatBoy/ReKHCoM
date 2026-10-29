"""Shared production surface operations, allocator faults and exact image controls."""
import argparse
from test_bridge_texture_safety import run

TEST=r'''
int main(int argc,char**argv) {
 unsigned char pixels[16]={10,20,30,255,40,50,60,255,70,80,90,255,100,110,120,255};
 int which=atoi(argv[1]);
 switch(which) {
 case 0: assert(!SDL_ConvertSurface(NULL,SDL_PIXELFORMAT_RGBA32));break;
 case 1: assert(!SDL_BlitSurfaceScaled(NULL,NULL,NULL,NULL,SDL_SCALEMODE_NEAREST));break;
 case 2: assert(!SDL_SaveBMP(NULL,NULL));break;
 case 3: failCountdown=1;assert(!SDL_CreateSurface(0x40000001,4,SDL_PIXELFORMAT_RGBA32));assert(failCountdown==1);break;
 case 4: assert(!SDL_CreateSurfaceFrom(2,2,SDL_PIXELFORMAT_RGBA32,pixels,4));break;
 case 5: {
  SDL_Surface*s=SDL_CreateSurfaceFrom(2,2,SDL_PIXELFORMAT_RGBA32,NULL,0);assert(s);
  assert(!SDL_ConvertSurface(s,SDL_PIXELFORMAT_RGBA32));SDL_DestroySurface(s);break;
 }
 case 6: case 7: {
  failCountdown=which==6?1:2;assert(!SDL_CreateSurface(2,2,SDL_PIXELFORMAT_RGBA32));assert(!SDL_SetSurfaceBlendMode(NULL,SDL_BLENDMODE_BLEND));break;
 }
 case 8: {
  SDL_Surface*s=SDL_CreateSurfaceFrom(2,2,SDL_PIXELFORMAT_RGBA32,pixels,8);assert(s);
  SDL_Surface*d=SDL_ConvertSurface(s,SDL_PIXELFORMAT_ARGB8888);assert(d);
  unsigned char bgra[16]={30,20,10,255,60,50,40,255,90,80,70,255,120,110,100,255};
  assert(!memcmp(d->pixels,bgra,16));
  SDL_Surface*back=SDL_ConvertSurface(d,SDL_PIXELFORMAT_RGBA32);assert(back&&!memcmp(back->pixels,pixels,16));
  assert(SDL_BlitSurfaceScaled(s,NULL,d,NULL,SDL_SCALEMODE_NEAREST)&&!memcmp(d->pixels,bgra,16));
  assert(SDL_SaveBMP(s,"control.bmp"));FILE*f=fopen("control.bmp","rb");assert(f);unsigned char bmp[70];assert(fread(bmp,1,70,f)==70);fclose(f);
  assert(bmp[0]=='B'&&bmp[1]=='M'&&bmp[54]==90&&bmp[55]==80&&bmp[56]==70&&bmp[62]==30&&bmp[63]==20&&bmp[64]==10);
  SDL_DestroySurface(back);SDL_DestroySurface(d);SDL_DestroySurface(s);break;
 }
 case 9: {
  SDL_Surface*s=SDL_CreateSurfaceFrom(2,2,SDL_PIXELFORMAT_RGBA32,pixels,8);assert(s);
  SDL_Surface*d=SDL_CreateSurface(2,2,SDL_PIXELFORMAT_RGBA32);assert(d);
  SDL_Rect huge={-2147483646,0,2147483647,1};assert(SDL_BlitSurfaceScaled(s,NULL,d,&huge,SDL_SCALEMODE_NEAREST));
  assert(!memcmp(d->pixels,pixels+4,4));
  SDL_DestroySurface(d);SDL_DestroySurface(s);break;
 }
 case 10: {
  SDL_Surface*s=SDL_CreateSurfaceFrom(2,2,SDL_PIXELFORMAT_RGBA32,pixels,8);assert(s);
  SDL_Surface*d=SDL_CreateSurface(4,3,SDL_PIXELFORMAT_RGBA32);assert(d);
  SDL_Rect clipped={-1,1,4,2};assert(SDL_BlitSurfaceScaled(s,NULL,d,&clipped,SDL_SCALEMODE_NEAREST));
  unsigned char expected[48]={0};
  memcpy(expected+16,pixels,4);memcpy(expected+20,pixels+4,4);memcpy(expected+24,pixels+4,4);
  memcpy(expected+32,pixels+8,4);memcpy(expected+36,pixels+12,4);memcpy(expected+40,pixels+12,4);
  assert(!memcmp(d->pixels,expected,48));SDL_DestroySurface(d);SDL_DestroySurface(s);
  SDL_Surface*empty=SDL_CreateSurface(0,2,SDL_PIXELFORMAT_RGBA32);assert(empty);
  SDL_Surface*copy=SDL_ConvertSurface(empty,SDL_PIXELFORMAT_ARGB8888);assert(copy&&copy->w==0&&copy->h==2);
  SDL_DestroySurface(copy);SDL_DestroySurface(empty);break;
 }
 }
 assert(liveAllocations==0);return 0;
}
'''

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--repro-null-only',action='store_true');a=p.parse_args()
 run(TEST,[0] if a.repro_null_only else range(11))
 print('PASS surface contracts: NULL/deferred input, allocation failures, byte-exact conversions/blit/BMP and huge clipped rectangles')
