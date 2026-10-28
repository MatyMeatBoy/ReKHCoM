package com.rekhcom.android;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

/** Small limits shared by the variant-only collector and ROM-free JVM tests. */
final class ReportLimits {
    static final int TEXT_LIMIT=65536, TRACE_LIMIT=4*1024*1024;
    static final class TooLarge extends IOException { TooLarge(){super("Trace exceeds report limit");} }
    static long copy(InputStream in,OutputStream out,long limit)throws IOException {
        byte[] buffer=new byte[8192];long total=0;int count;
        while((count=in.read(buffer))!=-1){
            if(count==0)continue;
            if(count>limit-total)throw new TooLarge();
            out.write(buffer,0,count);total+=count;
        }
        return total;
    }
    static String text(String value,int limit){
        if(value==null)return "";
        return value.length()<=limit?value:value.substring(0,limit);
    }
    static String utf8Text(String value,int limit){
        byte[] bytes=value.getBytes(StandardCharsets.UTF_8);
        if(bytes.length<=limit)return value;
        int end=limit;
        while(end>0&&(bytes[end]&0xC0)==0x80)end--;
        return new String(Arrays.copyOf(bytes,end),StandardCharsets.UTF_8);
    }
    static boolean safeName(String name){return name!=null&&name.matches("[A-Za-z0-9_-]+\\.(txt|pb)");}
}
