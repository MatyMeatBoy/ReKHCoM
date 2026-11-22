/* Coordinates only: the actual production sampler, without ROM assets. */
#include <assert.h>
#include <stdio.h>
#include "port/port_preliminar_v1_tiles.h"
static const PortPreliminarRecipe* recipe(int bg) {
 unsigned i;
 for(i=0;i<sizeof(kPreliminarRecipes)/sizeof(kPreliminarRecipes[0]);i++)
  if(kPreliminarRecipes[i].v[0]==81 && kPreliminarRecipes[i].v[1]==bg) return &kPreliminarRecipes[i];
 return NULL;
}
int main(void) {
 int bg,x,y,sx,sy,ex,ey;
 /* ROM mask (8,160) is opaque black and (24,144) its purple cave edge;
  * the old remap samples transparent mask pixels from the corridor. */
 assert(PortPreliminarPixelSource(recipe(1),8,160,&sx,&sy));
 if(sx!=8 || sy!=160) {fprintf(stderr,"black cave mask replaced by donor (%d,%d)\n",sx,sy);return 1;}
 assert(PortPreliminarPixelSource(recipe(1),24,144,&sx,&sy));assert(sx==24 && sy==144);
 for(bg=1;bg<=3;bg++) {
  const PortPreliminarRecipe*p=recipe(bg);assert(p);
  for(y=0;y<512;y+=3) for(x=0;x<128;x+=3) {
   assert(PortPreliminarPixelSource(p,x,y,&sx,&sy));
   if(sx!=x || sy!=y) {fprintf(stderr,"native cave cap changed bg%d (%d,%d)->(%d,%d)\n",bg,x,y,sx,sy);return 1;}
  }
  for(y=-128;y<640;y+=7) for(x=-256;x<0;x+=7) {
   assert(PortPreliminarPixelSource(p,x,y,&sx,&sy));assert(sx==0 && sy==0);
  }
  for(y=-128;y<640;y+=7) for(x=128;x<1024;x+=7) {
   if(bg==1 || bg==3) {
    int width=bg==1 && x>=192 ? 32 : 64;
    int start=width==32 ? 160 : 128;
    ex=start+PortSceneTileMod(x-start,width);ey=y+(x-ex)/2;
    if(bg==3 && x>=192 && ((ex<144 && ey>=32 && ey<48) ||
       (ex>=144 && ex<176 && ey>=16 && ey<32) || (ex>=176 && ey>=0 && ey<16))) ey+=16;
    if(ey<0 || ey>=256) ex=ey=0;
   } else {ex=x;ey=y;if(x>=256 || y>=208 || y<0)ex=ey=504;}
   sx=x;sy=y; /* A zero return preserves the native coordinate. */
   PortPreliminarPixelSource(p,x,y,&sx,&sy);assert(sx==ex && sy==ey);
  }
 }
 assert(PortPreliminarPixelSource(recipe(1),192,32,&sx,&sy));
 assert(sx==160 && sy==48); /* complete32px wave, no64px contour jump */
 assert(PortPreliminarPixelSource(recipe(3),256,-32,&sx,&sy));assert(sx==128 && sy==48);
 puts("PASS081: native cap, black exterior, continuous32px mask, empty wall cells completed; opaque floor/props unchanged");return 0;
}
