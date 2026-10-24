#ifndef REKHCOM_SAVE_IMPORT_H
#define REKHCOM_SAVE_IMPORT_H
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include "save_unpack.h"

namespace RekhcomSave {
inline uint16_t Checksum(const unsigned char* p, size_t size) {
    uint32_t sum=0;
    for(size_t i=0;i+1<size;i+=2) sum+=p[i]|(p[i+1]<<8);
    if(size&1)sum+=p[size-1];
    sum=(sum&65535)+(sum>>16);
    return static_cast<uint16_t>(~(sum+(sum>>16)));
}
struct Block { size_t offset,size; };
inline constexpr Block blocks[]={
    {0x10,0x40},{0x50,0x40},{0x90,0xf14},{0xfa4,0xf14},
    {0x1ec0,0x418},{0x22d8,0x418},{0x26f0,0x418},{0x2b08,0x418},
    {0x2f20,0xf14},{0x3e34,0xf14},{0x4d48,0xf14},{0x5c5c,0xf14}
};
inline int Version(const unsigned char* p) {
    if(!std::memcmp(p,"KHCOM_BACKUP_VER00000013",24))return 13;
    if(!std::memcmp(p,"KHCOM_BACKUP_VER00000012",24))return 12;
    return 0;
}
/* End of the last block: nothing the game reads lies beyond it (the rest of the 64 KB file is erased flash/SRAM). */
inline constexpr size_t kUsedEnd=0x5c5c+0xf14;

struct Found {
    Bytes image;
    int version=0,good=0;
    bool ambiguous=false;
    void Offer(Bytes&& candidate,int candidateVersion,int valid) {
        if(valid>good) {
            image=std::move(candidate);version=candidateVersion;good=valid;ambiguous=false;
        } else if(valid==good&&valid>0&&image!=candidate) {
            ambiguous=true;
        }
    }
};

/* Looks for a complete save image inside one buffer: a valid backup header (24-byte signature, checksum)
 * at SRAM offset 0x10 or 0x50 anchors it, and the blocks behind it are counted at their fixed offsets.
 * A truncated tail (erased space dropped by a tool) still counts: missing bytes read as 0xFF. */
inline void Scan(const unsigned char* data,size_t size,Found& found) {
    for(size_t at=0;at+0x40<=size;at++) {
        if(data[at]!='K')continue;
        int version=Version(data+at);
        if(!version||Checksum(data+at,0x40)!=0)continue;
        for(size_t headerOffset:{size_t(0x10),size_t(0x50)}) {
            if(at<headerOffset)continue;
            size_t base=at-headerOffset;
            Bytes candidate(65536,255);
            std::memcpy(candidate.data(),data+base,std::min<size_t>(kUsedEnd,size-base));
            int headers=0,files=0,valid=0;
            bool mixed=false;
            for(const auto& block:blocks) {
                const unsigned char* p=candidate.data()+block.offset;
                int v=Version(p);
                // A short header followed by erased SRAM can also satisfy a
                // longer ones-complement checksum. Check initialized common
                // data so it cannot masquerade as a system/file block.
                bool initialized=block.size==0x40 ||
                    (p[0xac]!=255 && p[0xad]!=255 &&
                     !(p[28]==255&&p[29]==255&&p[30]==255&&p[31]==255));
                if(v&&initialized&&Checksum(p,block.size)==0) {
                    if(v!=version)mixed=true;
                    valid++;
                    if(block.size==0x40)headers++;else files++;
                }
            }
            if(mixed||!headers||!files)continue;
            found.Offer(std::move(candidate),version,valid);
        }
    }
}

/* Byte-order variants some tools and emulators write: 16 / 32-bit swapped words, or every 8-byte block
 * reversed (the EEPROM order of GameShark exports and 3DS GBA saves). `shift` is the alignment of the
 * first group inside the buffer; the bytes before it are left as they are. */
inline Bytes Reorder(const Bytes& in,int kind,size_t shift) {
    Bytes out(in);
    size_t group=kind==1?2:kind==2?4:8;
    for(size_t at=shift;at+group<=out.size();at+=group)
        std::reverse(out.begin()+at,out.begin()+at+group);
    return out;
}

/* Detect a complete SRAM image inside a raw save, an emulator container (compressed or not) or a
 * backup-device wrapper, and rewrite it for `region`. A valid backup header is sufficient; never
 * realign SRAM to that copy. Distinct images in a multi-bank container are ambiguous and rejected.
 * `how` (optional) says where it was found: "raw", "unpacked", "swap16" ... */
inline int Convert(const std::vector<unsigned char>& input,int region,
                   std::vector<unsigned char>& output,int& fromRegion,const char** how=nullptr) {
    output.clear();fromRegion=-1;
    if(how)*how="";
    if(region<0||region>2||input.size()<0x80||input.size()>(4u<<20))return -1;
    Found found;
    Unpacker unpacked(input);
    const char* where="raw";
    for(size_t i=0;i<unpacked.buffers.size();i++) {
        int before=found.good;
        Scan(unpacked.buffers[i].data(),unpacked.buffers[i].size(),found);
        if(found.good>before)where=i?"unpacked":"raw";
    }
    if(found.good==0) {
        static const char* const names[]={"","swap16","swap32","reversed8"};
        for(int kind=1;kind<=3&&found.good==0;kind++) {
            size_t group=kind==1?2:kind==2?4:8;
            for(size_t i=0;i<unpacked.buffers.size();i++) {
                if(unpacked.buffers[i].size()>(1u<<20))continue;
                for(size_t shift=0;shift<group;shift++) {
                    Bytes swapped=Reorder(unpacked.buffers[i],kind,shift);
                    int before=found.good;
                    Scan(swapped.data(),swapped.size(),found);
                    if(found.good>before)where=names[kind];
                }
            }
        }
    }
    if(found.good==0||found.ambiguous)return -1;
    Bytes selected=std::move(found.image);
    int sourceVersion=found.version,goodCount=found.good;
    const char* signature=region==0?"KHCOM_BACKUP_VER00000013":"KHCOM_BACKUP_VER00000012";
    for(const auto& block:blocks) {
        auto* p=selected.data()+block.offset;
        if(Version(p)!=sourceVersion||Checksum(p,block.size)!=0)continue;
        std::memcpy(p,signature,24);
        // SaveHeader.data begins at 0x1c: flags at 0x1c, language at 0x1e.
        // Clearing 0x1c would silently erase Sora/Riku completion flags.
        if(block.size==0x40&&region!=0)p[0x1e]=p[0x1f]=0;
        p[24]=p[25]=0;
        uint16_t sum=Checksum(p,block.size);
        p[24]=static_cast<unsigned char>(sum);p[25]=static_cast<unsigned char>(sum>>8);
    }
    output=std::move(selected);fromRegion=sourceVersion==13?0:1;
    if(how)*how=where;
    return goodCount;
}
}
#endif
