package com.rekhcom.android;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicLong;
import java.util.regex.Pattern;

/** Own bounded diagnostic stream. No Activity, native resource or game state retained. */
final class LifecycleJournal {
    static final int FILE_LIMIT=16384, LINE_LIMIT=512, QUEUE_LIMIT=32, SUMMARY_LIMIT=6144, SHARE_LIMIT=65536;
    private static final Pattern SAFE_LINE=Pattern.compile("at_ms=[0-9]+ mono_ms=-?[0-9]+ session=[a-f0-9-]+ pid=[0-9]+ instance=[0-9]+ role=(game|launcher) event=(onCreate|onPause|onStop|onDestroy|runtime|unknown) runtime=(pending|launcher|host64|native32-eu|native32-us|native32-jp|unknown) changing=(true|false) config=0x[a-f0-9]{8} finishing=(true|false) restored=(true|false) dropped=[0-9]+");
    private final File directory;
    private final String role,session;
    private final int pid,limit;
    private final AtomicLong dropped=new AtomicLong();
    private final ThreadPoolExecutor writer;
    private boolean initialized;
    private volatile boolean failed;

    LifecycleJournal(File directory,String role,int pid){this(directory,role,pid,FILE_LIMIT);}
    LifecycleJournal(File directory,String role,int pid,int limit){
        if(!"game".equals(role)&&!"launcher".equals(role))throw new IllegalArgumentException("role");
        if(limit<LINE_LIMIT||limit>FILE_LIMIT)throw new IllegalArgumentException("limit");
        this.directory=directory;this.role=role;this.pid=Math.max(0,pid);this.limit=limit;
        session=Long.toHexString(System.currentTimeMillis())+"-"+Integer.toHexString(this.pid);
        writer=new ThreadPoolExecutor(1,1,0,TimeUnit.MILLISECONDS,new ArrayBlockingQueue<Runnable>(QUEUE_LIMIT),
            task->{Thread thread=new Thread(task,"lifecycle-"+role);thread.setDaemon(true);return thread;},new ThreadPoolExecutor.AbortPolicy());
    }
    String session(){return session;}
    boolean diskFailed(){return failed;}
    private static String event(String value){
        return "onCreate".equals(value)||"onPause".equals(value)||"onStop".equals(value)||"onDestroy".equals(value)||"runtime".equals(value)?value:"unknown";
    }
    private static String runtime(String value){
        return "pending".equals(value)||"launcher".equals(value)||"host64".equals(value)||"native32-eu".equals(value)||"native32-us".equals(value)||"native32-jp".equals(value)?value:"unknown";
    }
    String line(long instance,String event,String runtime,boolean changing,int config,boolean finishing,boolean restored){
        String hex=Integer.toHexString(config);hex="00000000".substring(hex.length())+hex;
        return "at_ms="+System.currentTimeMillis()+" mono_ms="+(System.nanoTime()/1000000)+" session="+session+
            " pid="+pid+" instance="+Math.max(0,instance)+" role="+role+" event="+event(event)+" runtime="+runtime(runtime)+
            " changing="+changing+" config=0x"+hex+" finishing="+finishing+" restored="+restored+" dropped="+dropped.get();
    }
    boolean record(long instance,String event,String runtime,boolean changing,int config,boolean finishing,boolean restored){
        return enqueue(line(instance,event,runtime,changing,config,finishing,restored));
    }
    boolean enqueue(String line){
        try {
            if(line.length()>LINE_LIMIT||!SAFE_LINE.matcher(line).matches()){dropped.incrementAndGet();return false;}
            final byte[] data=(line+"\n").getBytes(StandardCharsets.UTF_8);
            if(data.length>LINE_LIMIT){dropped.incrementAndGet();return false;}
            writer.execute(()->write(data));return true;
        }catch(Throwable error){dropped.incrementAndGet();return false;}
    }
    private File file(int previous){return new File(directory,"lifecycle-"+role+".log"+(previous==0?"":"."+previous));}
    private boolean rotate(){
        // Never remove an unexpected directory or overwrite oversized old evidence.
        for(int i=0;i<=5;i++)if(file(i).exists()&&(!file(i).isFile()||file(i).length()>FILE_LIMIT))return false;
        if(file(5).exists()&&!file(5).delete())return false;
        for(int i=4;i>=0;i--)if(file(i).exists()&&!file(i).renameTo(file(i+1)))return false;
        return true;
    }
    private void write(byte[] data){
        if(failed)return;
        try {
            if(!initialized){
                if((!directory.isDirectory()&&!directory.mkdirs())||!rotate()){failed=true;return;}
                initialized=true;
            }
            if(file(0).length()+data.length>limit&&!rotate()){failed=true;return;}
            try(FileOutputStream stream=new FileOutputStream(file(0),true)){stream.write(data);}
        }catch(Throwable error){failed=true;}
    }
    static String summary(File directory){
        StringBuilder out=new StringBuilder("Lifecycle diagnostics (best effort; no game-state recovery):\n");
        // Read fixed tails only, newest files first; no locks or waits for the writer.
        for(int previous=0;previous<=5;previous++)for(String role:new String[]{"game","launcher"}){
            File file=new File(directory,"lifecycle-"+role+".log"+(previous==0?"":"."+previous));
            try(RandomAccessFile stream=new RandomAccessFile(file,"r")){
                int count=(int)Math.min(1024,stream.length());byte[] tail=new byte[count];
                stream.seek(stream.length()-count);stream.readFully(tail);
                String[] lines=new String(tail,StandardCharsets.UTF_8).split("\n");
                for(String line:lines)if(line.length()<=LINE_LIMIT&&SAFE_LINE.matcher(line).matches()){
                    String entry="["+role+"/"+previous+"] "+line+"\n";
                    if(out.length()+entry.length()>SUMMARY_LIMIT)return out.toString();
                    out.append(entry);
                }
            }catch(Throwable ignored){} // absent/locked/partial files are optional diagnostics
        }
        return out.toString();
    }
    static String clipShare(String text){return text.length()<=SHARE_LIMIT?text:text.substring(0,SHARE_LIMIT-13)+"\n[truncated]\n";}
    // Test-only barriers. Activity callbacks and ShareLog NEVER await/shutdown the writer.
    void awaitForTests()throws Exception{writer.submit(()->{}).get(5,TimeUnit.SECONDS);}
    int queuedForTests(){return writer.getQueue().size();}
    void closeForTests()throws Exception{writer.shutdown();if(!writer.awaitTermination(10,TimeUnit.SECONDS))throw new AssertionError("writer did not drain");}
}
