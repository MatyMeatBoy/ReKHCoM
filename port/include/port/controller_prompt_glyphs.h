#ifndef PORT_CONTROLLER_PROMPT_GLYPHS_H
#define PORT_CONTROLLER_PROMPT_GLYPHS_H
#include <string.h>
/* Private two-unit token; only the port's scoped tutorial resolver emits it. */
#define PORT_PROMPT_TOKEN 26
#define PORT_PROMPT_SEMANTIC_TOKEN 27
#define PORT_PROMPT_LETTERS "ABXYLRZSDCVKJ+-?"
static inline int KhPromptGlyphId(const char* label) {
    static const char* const names[]={"Cross","Circle","Square","Triangle","Up","Down","Left","Right"};
    static const char* const pairs[]={"LB","RB","LT","RT","L1","R1","L2","R2","ZL","ZR","LS","RS","Menu","View","Options","Share"};
    for(int i=0;i<8;i++)if(!strcmp(label,names[i]))return i+1;
    for(int i=0;i<16;i++)if(!strcmp(label,pairs[i]))return 128+i;
    if(label[0] && !label[1] && strchr(PORT_PROMPT_LETTERS,label[0]))return (unsigned char)label[0];
    return 0;
}
static inline void KhPromptGlyphDot(unsigned char out[128],int x,int y,int value) {
    if(x<0 || x>=16 || y<0 || y>=16)return;
    int at=((y/8)*2+x/8)*32+(y%8)*4+(x%8)/2,shift=(x&1)*4;
    out[at]=(unsigned char)((out[at]&~(15<<shift)) | (value<<shift));
}
/* Same deliberate 3x5 lettering as the existing control diagram. The fixed
   12x10 button body and PS primitives need no external artwork or font glyph. */
static inline void KhPromptGlyphPixels(int id, unsigned char out[128]) {
    static const char* const pairs[]={"LB","RB","LT","RT","L1","R1","L2","R2","ZL","ZR","LS","RS","ST","SE","ST","SE"};
    char one[2]={(char)id,0};
    const char* text=id>=128 && id<144 ? pairs[id-128] : id>=32 && id<127 ? one : "";
    memset(out,0,128);
    if(!id || id>=144)return;
    for(int y=0;y<10;y++)for(int x=0;x<12;x++)
        if((x>1 && x<10) || (y>1 && y<8))
            KhPromptGlyphDot(out,x,y,(x==0 || x==11 || y==0 || y==9)?2:1);
    if(id<=4) {
        for(int y=2;y<=7;y++)for(int x=3;x<=8;x++) {
            int ink=id==1 ? (x-3==y-2 || x-3==7-y) :
                    id==2 ? (((x==3||x==8)&&y>=3&&y<=6)||((y==2||y==7)&&x>=4&&x<=7)) :
                    id==3 ? (x==3||x==8||y==2||y==7) :
                    (y==7 || x==5-(y-2)/2 || x==6+(y-2)/2);
            if(ink)KhPromptGlyphDot(out,x,y,2);
        }
    } else if(id<=8) {
        int d=id-5;
        for(int p=0;p<5;p++)for(int q=-2;q<=2;q++)
            if(q==0 || (p>=2 && q>=-(p-2) && q<=p-2))
                KhPromptGlyphDot(out,d<2?5+q:3+(d==2?p:4-p),d<2?2+(d==0?p:4-p):4+q,2);
    } else {
        int start=strlen(text)==1?4:2;
        for(int c=0;text[c];c++) {
            unsigned bits=0;
            switch(text[c]) {
                case 'A':bits=0x2BED;break;case 'B':bits=0x6BAE;break;case 'X':bits=0x5AAD;break;case 'Y':bits=0x5A92;break;
                case 'L':bits=0x4927;break;case 'R':bits=0x6BAD;break;case 'T':bits=0x7492;break;case 'S':bits=0x788F;break;
                case 'Z':bits=0x72A7;break;case 'E':bits=0x79E7;break;case '1':bits=0x2C97;break;case '2':bits=0x62A7;break;
                case 'C':bits=034443;break;case 'D':bits=065556;break;case 'V':bits=055552;break;case 'K':bits=055655;break;
                case 'J':bits=011153;break;case '?':bits=061202;break;
                case '-':bits=0x01C0;break;case '+':bits=0x05D0;break;
            }
            for(int y=0;y<5;y++)for(int x=0;x<3;x++)if(bits&(1u<<(14-y*3-x)))KhPromptGlyphDot(out,start+c*4+x,y+2,2);
        }
    }
}
#endif
