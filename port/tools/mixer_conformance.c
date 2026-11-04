/* ROM-free mixer boundary regression: poison bytes after a sample must not
   affect its sound. Compile with function sections + linker GC. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/m4a_port.c"
PortConfig gPortConfig;
MusicPlayerInfo gMPlayInfo_BGM;
SoundInfo* gSoundInfoPtr;
const u8 gClockTable[49] = {0};
const MPlayFunc gMPlayJumpTableTemplate[36] = {0};
void ClearChain(void* x) { abort(); }
void TrkVolPitSet(MusicPlayerInfo* m, MusicPlayerTrack* t) { abort(); }
u32 MidiKeyToFreq(WaveData* w, u8 k, u8 f) { abort(); }
void FadeOutBody(MusicPlayerInfo* m) { abort(); }
void Clear64byte(void* x) { abort(); }
void PortLog(const char* fmt, ...) { abort(); }
/* the music-player table (m4a_port.c's playback check) and SDL's environment lookup are outside this mixer fixture */
const MusicPlayer gMPlayTable[1] = {{0}};
char gNumMusicPlayers[1] = {0};
const char* SDL_getenv(const char* name) { (void)name; return NULL; }
static void render(int smooth, int fixed, int length, int loop, int poison, int32_t* out) {
    unsigned char storage[64];
    WaveData* wav = (WaveData*)storage;
    SoundInfo info = {0};
    SoundChannel channel = {0};
    memset(storage, poison, sizeof(storage));
    wav->size = length; wav->loopStart = 0;
    for (int i=0;i<length;i++) wav->data[i] = 32;
    info.masterVolume = 15; info.divFreq = 1 << 23;
    channel.statusFlags = SOUND_CHANNEL_SF_ENV_SUSTAIN | (loop ? 0x10 : 0);
    channel.type = fixed ? TONEDATA_TYPE_FIX : 0;
    channel.envelopeVolume = 255; channel.rightVolume = channel.leftVolume = 255;
    channel.frequency = 1; channel.wav = wav;
    channel.currentPointer = wav->data; channel.count = length;
    gPortMix.musicGain = gPortMix.sfxGain = 1.0f;
    gPortConfig.smoothAudio = smooth;
    memset(gPortMix.right,0,sizeof(gPortMix.right));
    memset(gPortMix.left,0,sizeof(gPortMix.left));
    MixChannel(&info,&channel,32,4);
    memcpy(out,gPortMix.right,32*sizeof(*out));
}
int main(void) {
    int32_t a[32],b[32]; int cases=0,failed=0;
    for(int smooth=0;smooth<2;smooth++) for(int fixed=0;fixed<2;fixed++)
    for(int length=1;length<=3;length++) for(int loop=0;loop<2;loop++) {
        render(smooth,fixed,length,loop,0x60,a);
        render(smooth,fixed,length,loop,0xa0,b);cases++;
        int bad=memcmp(a,b,sizeof(a)) != 0;
        for(int i=0;i<32;i++) if(a[i] != ((loop || i<length*4) ? 8128 : 0)) bad=1;
        if(bad) {failed++;printf("sample boundary failure: smooth=%d fixed=%d length=%d loop=%d\n",smooth,fixed,length,loop);}
    }
    printf("%d/%d boundary cases passed\n",cases-failed,cases);return failed?1:0;
}
