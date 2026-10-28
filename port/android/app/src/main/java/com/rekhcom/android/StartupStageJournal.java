package com.rekhcom.android;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicLong;

/** Fixed-name, bounded diagnostics. Never stores a URI, ROM path, SAV or auth secret. */
final class StartupStageJournal {
    static final int LIMIT=16384;
    private static final String STAGES="java_entry|game_requested|launcher_requested|before_sdl_create|after_sdl_create|native_enter|native_return|native_throw|destroy|retire_requested|retire_skipped|rom_verified|rom_import|picker_requested";
    private static final AtomicLong sequence=new AtomicLong();
    private static final ThreadPoolExecutor writer=new ThreadPoolExecutor(1,1,0,TimeUnit.MILLISECONDS,
        new ArrayBlockingQueue<Runnable>(32),task->{Thread t=new Thread(task,"startup-stage");t.setDaemon(true);return t;},new ThreadPoolExecutor.AbortPolicy());
    private static boolean role(String value){return "safe".equals(value)||"game".equals(value)||"launcher".equals(value);}
    private static String line(String role,String nonce,int pid,long generation,String stage,int code){
        if(!role(role)||!StartupGameAttempt.valid(nonce)||pid<=0||stage==null||
           generation<0||!stage.matches(STAGES))return null;
        return "at_ms="+System.currentTimeMillis()+" mono_ms="+(System.nanoTime()/1000000)+" seq="+sequence.incrementAndGet()+
            " pid="+pid+" nonce="+nonce+" generation="+generation+" role="+role+" stage="+stage+" code="+code+"\n";
    }
    private static synchronized boolean append(File directory,String role,String line){
        if(line==null)return false;
        try {
            if(!directory.isDirectory()&&!directory.mkdirs())return false;
            File file=new File(directory,"startup-"+role+".log"),previous=new File(directory,file.getName()+".1");
            byte[] data=line.getBytes(StandardCharsets.UTF_8);
            if(data.length>512||file.exists()&&(!file.isFile()||file.length()>LIMIT))return false;
            if(file.length()+data.length>LIMIT){
                if(previous.exists()&&(!previous.isFile()||previous.length()>LIMIT||!previous.delete()))return false;
                if(!file.renameTo(previous))return false;
            }
            try(FileOutputStream out=new FileOutputStream(file,true)){out.write(data);out.getFD().sync();}
            return true;
        }catch(IOException error){return false;} // Optional diagnostics cannot prevent starting the game.
    }
    static boolean record(File directory,String role,String nonce,int pid,String stage,int code){return record(directory,role,nonce,pid,0,stage,code);}
    static boolean record(File directory,String role,String nonce,int pid,long generation,String stage,int code){return append(directory,role,line(role,nonce,pid,generation,stage,code));}
    static boolean async(File directory,String role,String nonce,int pid,String stage,int code){
        return async(directory,role,nonce,pid,0,stage,code);
    }
    static boolean async(File directory,String role,String nonce,int pid,long generation,String stage,int code){
        final String event=line(role,nonce,pid,generation,stage,code);if(event==null)return false;
        try{writer.execute(()->append(directory,role,event));return true;}catch(RejectedExecutionException error){return false;}
    }
    static String summary(File directory){
        StringBuilder result=new StringBuilder("Startup boundaries (best effort; order seq per PID; return0 is not a device health certificate):\n");
        for(String role:new String[]{"safe","launcher","game"})for(String suffix:new String[]{"",".1"}){
            File file=new File(directory,"startup-"+role+".log"+suffix);
            if(!file.isFile()||file.length()>LIMIT)continue;
            try(BufferedReader in=new BufferedReader(new InputStreamReader(new FileInputStream(file),StandardCharsets.UTF_8))){
                String line;while((line=in.readLine())!=null){
                    if(line.length()<=512&&line.matches("at_ms=[0-9]+ mono_ms=-?[0-9]+ seq=[0-9]+ pid=[0-9]+ nonce=[a-f0-9]{32} generation=[0-9]+ role=(safe|launcher|game) stage=("+STAGES+") code=-?[0-9]+")){
                        if(result.length()+line.length()>32768)return result+"\n[summary truncated]\n";
                        result.append(line).append('\n');
                    }
                }
            }catch(IOException ignored){}
        }
        return result.toString();
    }
}
