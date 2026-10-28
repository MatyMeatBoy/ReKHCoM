package com.rekhcom.android;

import java.io.*;
import java.security.MessageDigest;
import java.util.Enumeration;
import java.util.Locale;
import java.util.zip.CRC32;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/** Bounded picker import, independent of Android UI. Owns/closes the input before
 * committing. Archive names never become output paths; only verified region
 * slots or the existing HD-pack filename can be replaced. */
public final class RomImport {
    private RomImport(){}
    private static final long ROM_SIZE=32L*1024*1024;
    private static final long ARCHIVE_LIMIT=8L*1024*1024*1024;
    private static final long ROM_ARCHIVE_LIMIT=64L*1024*1024;
    private static final int ENTRY_LIMIT=16384;
    private static final String[] CODES={"B8CP","B8CE","B8CJ"};
    private static final String[] HASHES={"8db73586cdb11b3795907edebf43228dbcd3e6b2",
        "10729bd884f8fdca7a310b6d606c52e46657aa48","59ec0a0a4ccd1e6acb3bbd7bfb21d63988958cfa"};
    public enum Kind {ROM,HD_PACK}
    public static final class Result {
        public final Kind kind;
        public final String code;
        private Result(Kind k,String c){kind=k;code=c;}
    }
    /** Caller supplies an atomic same-filesystem replacement. Failure must leave
     * the old destination intact; Android uses Os.rename (API21), not delete. */
    public interface Replacer {void replace(File pending,File target)throws IOException;}
    private static final class Copied {long size,crc;String hash;}
    private static Copied copy(InputStream in,File target,long limit,boolean hash)throws Exception{
        MessageDigest digest=hash?MessageDigest.getInstance("SHA-1"):null;
        CRC32 crc=hash?new CRC32():null;
        Copied result=new Copied();byte[] block=new byte[65536];int n;
        try(FileOutputStream out=new FileOutputStream(target)){
            while((n=in.read(block))!=-1){
                if(n==0)throw new IOException("File provider made no read progress");
                if(n>limit-result.size)throw new IOException("File exceeds import size limit");
                out.write(block,0,n);result.size+=n;
                if(digest!=null){digest.update(block,0,n);crc.update(block,0,n);}
            }
            out.getFD().sync();
        }
        if(digest!=null){StringBuilder hex=new StringBuilder();for(byte b:digest.digest())hex.append(String.format(Locale.ROOT,"%02x",b&255));result.hash=hex.toString();result.crc=crc.getValue();}
        return result;
    }
    private static int region(Copied data)throws IOException{
        if(data.size!=ROM_SIZE)throw new IOException("A clean 32 MiB GBA dump is required");
        for(int i=0;i<HASHES.length;i++)if(HASHES[i].equals(data.hash))return i;
        throw new IOException("ROM not recognized; use a supported clean EU, US or JP dump");
    }
    /** Read-only validation of a previous installation; never asks the user to
     * reimport a valid existing dump and never writes ROM/save/config data. */
    public static String verifyInstalled(File file)throws Exception{
        if(!file.isFile()||file.length()!=ROM_SIZE)throw new IOException("A clean 32 MiB GBA dump is required");
        MessageDigest digest=MessageDigest.getInstance("SHA-1");Copied data=new Copied();byte[] block=new byte[65536];
        try(InputStream in=new FileInputStream(file)){
            int n;while((n=in.read(block))!=-1){
                if(n==0||n>ROM_SIZE-data.size)throw new IOException("ROM read failed or size changed");
                digest.update(block,0,n);data.size+=n;
            }
        }
        StringBuilder hex=new StringBuilder();for(byte value:digest.digest())hex.append(String.format(Locale.ROOT,"%02x",value&255));
        data.hash=hex.toString();return CODES[region(data)];
    }
    private static void safeName(String name)throws IOException{
        if(name.length()>1024||name.startsWith("/")||name.indexOf('\\')>=0||name.indexOf(':')>=0||name.indexOf('\0')>=0)
            throw new IOException("Unsafe ZIP entry name");
        for(String part:name.split("/"))if(part.equals(".."))throw new IOException("Unsafe ZIP entry name");
    }
    private static boolean hdMember(String name){
        String rel=name.startsWith("hd/")?name.substring(3):name;
        String lower=rel.toLowerCase(Locale.ROOT);
        return (rel.startsWith("music/")&&(lower.endsWith(".ogg")||lower.endsWith(".loop")))||
            ((rel.startsWith("movies/")||rel.startsWith("scenes/"))&&lower.endsWith(".mp4"))||
            (rel.startsWith("sprites/")&&lower.endsWith(".png"))||
            (rel.startsWith("subs/")&&lower.endsWith(".txt"))||
            ((rel.startsWith("media/en/")||rel.startsWith("media/ja/"))&&
             (lower.endsWith(".mp4")||lower.endsWith(".png")||lower.endsWith(".txt")||lower.endsWith(".meta")));
    }
    public static Result install(InputStream input,File files,Replacer replace)throws Exception{
        File staged=null,romPart=null;
        try{
            boolean archive;Copied copied;
            try(InputStream owned=input){
                if(owned==null)throw new IOException("Selected file is not accessible");
                if(!files.isDirectory()&&!files.mkdirs())throw new IOException("No import directory");
                PushbackInputStream stream=new PushbackInputStream(owned,4);
                byte[] head=new byte[4];int used=0,n;
                while(used<4&&(n=stream.read(head,used,4-used))!=-1){if(n==0)throw new IOException("File provider made no read progress");used+=n;}
                if(used>0)stream.unread(head,0,used);
                archive=used==4&&head[0]=='P'&&head[1]=='K'&&
                    ((head[2]==3&&head[3]==4)||(head[2]==5&&head[3]==6)||(head[2]==7&&head[3]==8));
                staged=File.createTempFile("import-",".tmp",files);
                copied=copy(stream,staged,archive?ARCHIVE_LIMIT:ROM_SIZE,!archive);
            }
            Result result;File pending,target;
            if(archive){
                ZipEntry candidate=null;boolean hd=false;long total=0;int count=0;
                try(ZipFile zip=new ZipFile(staged)){
                    Enumeration<? extends ZipEntry> entries=zip.entries();
                    while(entries.hasMoreElements()){
                        ZipEntry entry=entries.nextElement();
                        if(++count>ENTRY_LIMIT)throw new IOException("ZIP entry limit exceeded");
                        safeName(entry.getName());if(entry.isDirectory())continue;
                        long size=entry.getSize();
                        if(size<0||size>ARCHIVE_LIMIT-total)throw new IOException("ZIP uncompressed size limit exceeded");
                        total+=size;
                        boolean media=hdMember(entry.getName());hd|=media;
                        String lower=entry.getName().toLowerCase(Locale.ROOT);
                        if(!media&&(lower.endsWith(".gba")||lower.endsWith(".bin")||size==ROM_SIZE)){
                            if(candidate!=null)throw new IOException("Select a ZIP containing only one ROM dump");
                            if(size!=ROM_SIZE)throw new IOException("ZIP ROM must be a clean 32 MiB dump");
                            candidate=entry;
                        }
                    }
                    if(candidate!=null){
                        if(hd)throw new IOException("ROM and HD entries must be imported separately");
                        if(total>ROM_ARCHIVE_LIMIT)throw new IOException("ROM ZIP uncompressed size limit exceeded");
                        File dir=new File(files,"roms");if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("No ROM directory");
                        romPart=File.createTempFile("rom-",".tmp",dir);
                        try(InputStream in=zip.getInputStream(candidate)){copied=copy(in,romPart,ROM_SIZE,true);}
                        if(copied.size!=candidate.getSize()||copied.crc!=candidate.getCrc())throw new IOException("ZIP ROM size or CRC mismatch");
                        int version=region(copied);result=new Result(Kind.ROM,CODES[version]);
                        pending=romPart;target=new File(dir,result.code+".gba");
                    }else{
                        if(!hd)throw new IOException("ZIP contains no supported ROM or HD content");
                        result=new Result(Kind.HD_PACK,"");pending=staged;target=new File(files,"ReKHCoM-HD.zip");
                    }
                }
            }else{
                int version=region(copied);File dir=new File(files,"roms");
                if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("No ROM directory");
                result=new Result(Kind.ROM,CODES[version]);pending=staged;target=new File(dir,result.code+".gba");
            }
            replace.replace(pending,target);
            return result;
        }finally{
            if(romPart!=null&&romPart.exists())romPart.delete();
            if(staged!=null&&staged.exists())staged.delete();
        }
    }
}
