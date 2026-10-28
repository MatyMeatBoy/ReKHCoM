"""QueueHdPart + actual loader/queue: only accepted replacements cover a strip."""
from pathlib import Path
import tempfile,subprocess
import test_hd_sprite_fallback as f
unit=f.unit[:f.unit.index('static void reset(void){')]
unit=unit.replace('static void PortPaletteFade(const uint16_t*a,const volatile uint16_t*b,int c,int*d,int*e){*d=*e=0;}',
 'static int fakeDark,fakeWhite;static void PortPaletteFade(const uint16_t*a,const volatile uint16_t*b,int c,int*d,int*e){*d=fakeDark;*e=fakeWhite;}')
unit+=r'''
static const void* testTiles;
static void reset(void){
 memset(sState,0,sizeof(sState));memset(sDrawCount,0,sizeof(sDrawCount));
 memset(sOamHd,0,sizeof(sOamHd));memset(oam,0,sizeof(oam));sHdPieceCount=0;
 memset(sOamTag,0,sizeof(sOamTag));fakeDark=fakeWhite=0;
 sBytes=0;width=height=decodedW=decodedH=16;infoOK=decodeOK=1;
 creates=destroys=decodes=failTexture=0;sRenderer=(void*)1;mainThread=1;
 kHdSprites[0]=(HdSpriteDef){.name="test",.w=16,.h=16};
 testTiles=gSynthesisHdIdentities;
}
static int queue(int layer,int x,int y,int w,int h,int alpha,int flags,int affine){
 sOamHd[0]=(OamHdPart){.id=1,.x=x,.y=y,.w=w,.h=h,.flags=flags,.palette=palette,.tiles=testTiles};
 return QueueHdPart(layer,0,x,y,w,h,w,h,affine,256,0,0,256,alpha);
}
int main(void){FILE*f=fopen("test","wb");assert(f);fputs("exists",f);fclose(f);
 /* This exact rejected rectangle used to steal the top half of a valid one. */
 reset();assert(!queue(0,0,-8,16,16,255,0,0));assert(sHdPieceCount==0);
 assert(queue(0,0,0,16,16,255,0,0));assert(sDraws[0][0].v0==0&&sDraws[0][0].v1==1);
 /* Rejected decoder/upload/capacity must not leave phantom coverage either. */
 reset();decodeOK=0;assert(!queue(0,0,0,16,8,255,0,0));assert(sHdPieceCount==0);
 memset(sState,0,sizeof(sState));decodeOK=1;assert(queue(0,0,0,16,16,255,0,0));assert(sDraws[0][0].v0==0);
 reset();failTexture=2;assert(!queue(0,0,0,16,8,255,0,0));assert(sHdPieceCount==0);
 memset(sState,0,sizeof(sState));creates=failTexture=0;assert(queue(0,0,0,16,16,255,0,0));assert(sDraws[0][0].v0==0);
 reset();sDrawCount[0]=MAX_DRAWS;assert(!queue(0,0,0,16,8,255,0,0));assert(sHdPieceCount==0);
 sDrawCount[0]=0;assert(queue(0,0,0,16,16,255,0,0));assert(sDraws[0][0].v0==0);
 /* Identical-colour edge strips are deduplicated, with matching UV/geometry. */
 reset();assert(queue(0,0,0,16,8,255,0,0));assert(queue(0,0,4,16,8,255,0,0));
 assert(sDrawCount[0]==2&&sDraws[0][1].v0==.5f&&sDraws[0][1].v1==.75f);
 assert(sDraws[0][1].y[0]==8&&sDraws[0][1].y[3]==12);
 reset();assert(queue(0,0,0,8,16,255,0,0));assert(queue(0,4,0,8,16,255,0,0));
 assert(sDraws[0][1].u0==.5f&&sDraws[0][1].u1==.75f);
 /* Only the independently tagged synthesis family changes. Unrelated cards,
    null tags and the first address past the family preserve old composition. */
 static const unsigned char otherTiles[1];
 const void* excluded[]={NULL,otherTiles,gSynthesisHdIdentities+SYNTHESIS_HD_COUNT};
 for(unsigned n=0;n<sizeof(excluded)/sizeof(excluded[0]);n++){
  reset();testTiles=excluded[n];assert(queue(0,0,0,16,8,255,0,0));
  assert(queue(0,0,4,16,8,255,0,0));assert(sDraws[0][1].v0==.25f&&sHdPieceCount==0);
 }
 for(unsigned n=0;n<SYNTHESIS_HD_COUNT;n++){
  reset();testTiles=gSynthesisHdIdentities+n;assert(queue(0,0,0,16,8,255,0,0));
  assert(queue(0,0,4,16,8,255,0,0));assert(sDraws[0][1].v0==.5f);
 }
 /* Different opacity/layer, flipped or affine pieces keep their old drawing. */
 reset();assert(queue(0,0,0,16,8,128,0,0));assert(queue(0,0,4,16,8,255,0,0));assert(sDraws[0][1].v0==.25f);
 reset();assert(queue(0,0,0,16,8,0,0,0));assert(queue(0,0,4,16,8,255,0,0));assert(sDraws[0][1].v0==.25f);
 for(int bright=0;bright<2;bright++){
  reset();assert(queue(0,0,0,16,8,255,0,0));
  if(bright)fakeWhite=120;else fakeDark=120;
  assert(queue(0,0,4,16,8,255,0,0));assert(sDraws[0][1].v0==.25f);
 }
 reset();assert(queue(0,0,0,16,8,255,0,0));assert(queue(1,0,4,16,8,255,0,0));assert(sDraws[1][0].v0==.25f);
 reset();assert(queue(0,0,0,16,8,255,0,0));oam[2]=0x400;
 assert(queue(0,0,4,16,8,255,0,0));assert(sDraws[0][1].v0==.25f);
 reset();assert(queue(0,0,0,16,8,255,0,0));sOamTag[0].clipX0=-20;sOamTag[0].clipX1=30;
 assert(queue(0,0,4,16,8,255,0,0));assert(sDraws[0][1].v0==.25f);
 for(int transform=0;transform<2;transform++){
  reset();assert(queue(0,0,0,16,8,255,transform?0:1,transform));
  assert(queue(0,0,4,16,8,255,transform?0:1,transform));assert(sDraws[0][1].v0==.25f);
 }
 /* Full containment and middle overlaps remain separate draws: a generic
    omission would also need to preserve per-OAM ownership, not just pixels. */
 reset();assert(queue(0,0,0,16,16,255,0,0));assert(queue(0,0,0,16,16,255,0,0));assert(sDrawCount[0]==2);
 reset();assert(queue(0,0,4,16,4,255,0,0));assert(queue(0,0,0,16,16,255,0,0));assert(sDraws[0][1].v0==0&&sDraws[0][1].v1==1);
 reset();for(int n=0;n<64;n++)assert(queue(0,0,0,16,16,255,0,0));assert(sHdPieceCount==64);
 assert(queue(0,0,4,16,8,128,0,0));assert(sHdPieceCount==64&&sDrawCount[0]==65);
 puts("PASS: accepted edge-strip coverage, exact UVs/geometry and native fallback controls");return 0;
}
'''
cc=next((f.ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='hd-piece-acceptance-') as folder:
 d=Path(folder);src=d/'test.c';exe=d/'test.exe';src.write_text(unit)
 subprocess.run([str(cc),'-std=c11','-Werror=implicit-function-declaration','-Wno-compare-distinct-pointer-types',str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],cwd=d,check=True,timeout=15,creationflags=0x08004000)
print('PASS: production queue acceptance, rejection recovery, strip trimming and preserved ownership-sensitive paths')
