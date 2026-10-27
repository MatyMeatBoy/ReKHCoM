"""Actual Options-close consumer and content-keyed HUD loader, no game/GPU build.
The frozen OAM traversal is a boundary stub; runtime timing remains separate QA.
"""
from pathlib import Path
import ast, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
def fn(s,name):
 a=s.rfind("\n",0,s.index(name+"("))+1;b=s.index("{",a);e=b+1;depth=1
 while depth:
  depth+=(s[e]=="{")-(s[e]=="}");e+=1
 return s[a:e]
hud=(ROOT/"port/src/port_hdhud.c").read_text(encoding="utf8")
ui=(ROOT/"port/src/port_ui.c").read_text(encoding="utf8")
main=(ROOT/"port/src/port_main.c").read_text(encoding="utf8")
reset=ast.parse((ROOT/"port/tools/test_hd_hud_invalidation.py").read_text())
pre=next(ast.literal_eval(n.value) for n in reset.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=="PREAMBLE" for t in n.targets))
pre=pre[:pre.index("static struct{SDL_Texture*image,*white;}*sAssets;")]+pre[pre.index("static int sAttempted"):]
pre=pre.replace("static struct{int hdHud;}gPortConfig={1};","static struct{int hdHud,hdTitle;}gPortConfig={0,0};")
pre=pre.replace("static int generation=1,decodes,creates,destroys,failUpload;","static int generation=1,decodes,creates,destroys,failUpload,missing;")
pre=pre.replace("(void)p;decodes++;","(void)p;decodes++;if(missing)return NULL;")
pre+="\n#define SDL_calloc calloc\n#define SDL_snprintf snprintf\n#define SDL_arraysize(a) (sizeof(a)/sizeof((a)[0]))\n#define PORT_ENV_ONCE(v,f,k) ((v)=f(k))\n"
pre+=r'''
static int sOpen=1,sScenePause,sTouchEdit,sReturn,sClosing=1,sFade=16,sSkipPrompt,sSkipOwnLayer,sTitleHintFrames,sSceneStep,sSceneCursor,sHarnessTick,sHarnessShotPending;
static int sMenuLightDrawClock,sHdUiCount,sHdArt[3],sFbW,sFbH;
static uint32_t *sHdUiFrame,*sFb;
static struct{int originX,originY,width,height;}gPortScreen={0,0,240,160};
static int gPortHudScale=1,gPortHudW=240,gPortHudH=160,activeHud=1,renderCalls,cancels;
static SDL_Renderer renderer=1,*sRenderer=&renderer;
static uint32_t *sHudBuf;static size_t sHudCapacity;
#define MENU_FADE 8
#define MENU_FADE_COLOR 0
#define LANG_COUNT 6
#define GBA_SCREEN_W 240
static uint64_t SDL_GetPerformanceCounter(void){return 1;}
static uint64_t SDL_GetPerformanceFrequency(void){return 1000;}
static int Lang(void){return 0;}
static void QuitTrack(void){}
static void PortSceneGalleryPreviewPoll(void){}
static void PortSceneGalleryPreviewCancel(void){cancels++;}
static void DrawScenePause(int a,int b,int c){(void)a;(void)b;(void)c;}
static void HarnessOptionsShot(uint32_t*a,int b,int c){(void)a;(void)b;(void)c;}
static void PortHarnessShot(const char*a){(void)a;}
static int TextWidth(const char*a){return (int)strlen(a);}
static void MenuButton(int a,int b,const char*c,int d){(void)a;(void)b;(void)c;(void)d;}
static void HarnessFadeShot(int a,int b){(void)a;(void)b;}
static void DrawSkipPrompt(int a){(void)a;}
static void DrawTitleHint(int a){(void)a;}
static void DrawMenu(int a,int b,int c){(void)a;(void)b;(void)c;}
static void Shade(int a,int b,int c,int d,unsigned e,int f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;}
static void Close(void){sClosing=8;}
#define SDL_atoi atoi
static uint32_t PortFrameCount(void){return 10;}
static int PortPpuHudActive(void){return activeHud;}
static int reallocCalls;
static void* SDL_realloc(void*p,size_t n){(void)p;(void)n;reallocCalls++;return NULL;}
static void PortFatal(const char*m){fprintf(stderr,"unexpected optional fatal: %s",m);exit(88);}
'''
core=fn(main,"GrowPixelBuffer")+"\n"+fn(ui,"PortUiActive")+"\n"+hud[hud.index("typedef struct HudAsset {"):hud.index("void PortHdHudDrawSprites(")]
core+=r'''
static uint32_t piece[16]={0,0,0,0,0,0xffffffff,0,0,0,0,0,0,0,0,0,0};
static void PortPpuRenderHud(uint32_t*buf){(void)buf;renderCalls++;PortHdHudSpriteBegin();PortHdHudSpriteRegister(piece,4,4,0,0,0,0,255);}
'''
core+=fn(ui,"PortUiDraw")+"\n"
if "static void PrepareHudAfterOptions(" in main:core+=fn(main,"PrepareHudAfterOptions")+"\n"
a=main.index("    PortUiDraw(sComposite,");b=main.index("    {",a)
# Include the actual caller statements between UI draw and next presentation block.
core+="static void PresentBoundary(void){uint32_t frame[4]={0};uint32_t*sComposite=frame;int menuCanvas=sOpen;\n"+main[a:b]+"}\n"
test=r'''
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);return 77;}}while(0)
int main(void){
 sAssetRenderer=&renderer;
 sHudBuf=calloc((size_t)gPortHudW*gPortHudH,sizeof(*sHudBuf));sHudCapacity=(size_t)gPortHudW*gPortHudH;
 CHECK(sHudBuf);
 /* The real loader refuses work during Options. Toggle exactly as Change does. */
 TOGGLE_ACTUAL;
 CHECK(!PortHdHudSpriteRegister(piece,4,4,0,0,0,0,255)&&decodes==0);
 sClosing=2;PresentBoundary();CHECK(sOpen&&decodes==0&&renderCalls==0);
 PresentBoundary();CHECK(!sOpen&&cancels==1);
 CHECK(decodes==1&&creates==2&&sAssetCount==1&&sAssets[0].image&&sAssets[0].white);
 int d=decodes,u=creates;PortPpuRenderHud(piece);CHECK(decodes==d&&creates==u);
 sOpen=1;sClosing=1;PresentBoundary();CHECK(decodes==d&&creates==u);
 /* Missing and upload failure preserve native drawing; neither loops IO. */
 for(int mode=1;mode<=2;mode++){
  piece[5]=0xff000000u+(unsigned)mode;missing=mode==1;failUpload=mode==2;sOpen=1;sClosing=1;
  PresentBoundary();int at=decodes;CHECK(!sAssets[sAssetCount-1].image&&!sAssets[sAssetCount-1].white);
  CHECK(!PortHdHudSpriteRegister(piece,4,4,0,0,0,0,255)&&decodes==at);
 }
 missing=failUpload=0;d=decodes;
 /* Failed allocator stays armed. Optional warming never calls the real grow
    function or mutates the old pointer/capacity when dimensions no longer fit. */
 uint32_t* retained=sHudBuf;size_t capacity=sHudCapacity;sHudBuf[0]=0x12345678;
 sHudCapacity=1;sOpen=1;sClosing=1;int beforeRender=renderCalls;
 PresentBoundary();CHECK(!sOpen&&renderCalls==beforeRender&&reallocCalls==0&&decodes==d);
 CHECK(sHudBuf==retained&&sHudCapacity==1&&sHudBuf[0]==0x12345678);
 sHudCapacity=capacity;sHudBuf=NULL;sOpen=1;sClosing=1;PresentBoundary();
 CHECK(renderCalls==beforeRender&&reallocCalls==0);sHudBuf=retained;
 gPortConfig.hdHud=0;sOpen=1;sClosing=1;PresentBoundary();CHECK(decodes==d);
 gPortConfig.hdHud=1;activeHud=0;sOpen=1;sClosing=1;PresentBoundary();CHECK(decodes==d);
 puts("PASS actual Options close + HUD register/loader: before resume, cache reuse, missing/upload failure, off/non-HUD");return 0;
}
'''
toggle=ui[ui.index("gPortConfig.hdHud = !gPortConfig.hdHud;"):].split(";",1)[0]
test=test.replace("TOGGLE_ACTUAL",toggle)
base=next(p for p in ROOT.parents if (p/"toolchain").is_dir())
cc=next((base/"toolchain").glob("llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe"))
with tempfile.TemporaryDirectory(prefix="hud-options-warm-") as td:
 p=Path(td);(p/"test.c").write_text(pre+core+test,encoding="utf8")
 subprocess.run([str(cc),"-std=gnu11","-Werror","-Wno-unused-function",str(p/"test.c"),"-o",str(p/"test.exe")],check=True)
 result=subprocess.run([str(p/"test.exe")],capture_output=True,text=True,timeout=10,creationflags=0x08000000)
 print(result.stdout+result.stderr,end="")
 result.check_returncode()
