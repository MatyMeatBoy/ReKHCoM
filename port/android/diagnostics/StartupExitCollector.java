package com.rekhcom.android;

import android.app.ActivityManager;
import android.app.ApplicationExitInfo;
import android.content.Context;
import android.os.Build;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.zip.*;

/** Variant only: own-package historical exit records, never global logcat or saves. */
final class StartupExitCollector {
    static File report(Context context){return new File(new File(context.getCacheDir(),"startup-diagnostics"),"report.zip");}
    static File sharedDirectory(Context context){return new File(report(context).getParentFile(),"shared");}
    static synchronized File shareSnapshot(Context context)throws IOException {return ReportSnapshots.create(report(context),sharedDirectory(context));}
    static synchronized String collect(Context context)throws IOException {
        File directory=report(context).getParentFile();
        if(!directory.isDirectory()&&!directory.mkdirs())throw new IOException("Cannot create report folder");
        ReportSnapshots.preservePrior(report(context));
        StringBuilder text=new StringBuilder("ReKHCoM optional safe startup test (device root cause not established)\n");
        try{android.content.pm.PackageInfo info=context.getPackageManager().getPackageInfo(context.getPackageName(),0);text.append("Current build: ").append(info.versionName).append(" code=").append(info.versionCode).append('\n');}catch(Exception error){text.append("Build identity unavailable\n");}
        try(InputStream input=context.getAssets().open("startup-build.txt")){ByteArrayOutputStream bytes=new ByteArrayOutputStream();ReportLimits.copy(input,bytes,4096);text.append(new String(bytes.toByteArray(),StandardCharsets.UTF_8)).append('\n');}catch(IOException ignored){}
        text.append("Model: ").append(ReportLimits.text(Build.MANUFACTURER+" "+Build.MODEL,200)).append("\nAndroid API: ").append(Build.VERSION.SDK_INT).append('\n');
        text.append("Only this app's exit records. Exit reason is not a verified root cause. Updates/force-stop may add unrelated exit records.\n");
        text.append("An Activity/native return can finish with no process-exit record. Safe-entry retirement of its own terminated SDL process is intentional; correlate nonce/PID/stages.\n");
        text.append(StartupStageJournal.summary(context.getFilesDir())).append('\n');
        text.append(LifecycleJournal.summary(context.getFilesDir())).append('\n');
        LinkedHashMap<String,byte[]> ownLogs=new LinkedHashMap<>();
        for(String name:new String[]{"launcher-startup-error.log","kh_com_host.log","kh_com.log"}){
            File file=new File(context.getFilesDir(),name);int limit=name.startsWith("launcher-")?1024:28000;
            if(!file.isFile())continue;
            try(RandomAccessFile input=new RandomAccessFile(file,"r")){
                byte[] bytes=new byte[(int)Math.min(limit,input.length())];input.seek(input.length()-bytes.length);input.readFully(bytes);
                ownLogs.put("native-"+name.replace('.','-')+".txt",bytes);
                text.append("Included own log tail: ").append(name).append(" bytes=").append(bytes.length).append('\n');
            }catch(IOException error){text.append("Own log unavailable: ").append(name).append('\n');}
        }
        LinkedHashMap<String,File> traces=new LinkedHashMap<>();
        if(Build.VERSION.SDK_INT<30)text.append("Android does not expose historical process exits before API30. No retained trace available.\n");
        else {
            try { Api30.collect(context,directory,text,traces); }
            catch(Exception e){text.append("Exit history unavailable: ").append(ReportLimits.text(e.toString(),400)).append('\n');}
        }
        String summary=ReportLimits.utf8Text(text.toString(),ReportLimits.TEXT_LIMIT);
        File pending=new File(directory,"report.pending");
        try {
            try(ZipOutputStream zip=new ZipOutputStream(new FileOutputStream(pending))){
                zip.putNextEntry(new ZipEntry("summary.txt"));zip.write(summary.getBytes(StandardCharsets.UTF_8));zip.closeEntry();
                for(Map.Entry<String,byte[]> item:ownLogs.entrySet()){
                    zip.putNextEntry(new ZipEntry(item.getKey()));zip.write(item.getValue());zip.closeEntry();
                }
                for(Map.Entry<String,File> item:traces.entrySet()){
                    if(!ReportLimits.safeName(item.getKey()))throw new IOException("Invalid report entry");
                    zip.putNextEntry(new ZipEntry(item.getKey()));
                    try(InputStream input=new FileInputStream(item.getValue())){ReportLimits.copy(input,zip,ReportLimits.TRACE_LIMIT);}
                    zip.closeEntry();
                }
            }
            if(!pending.renameTo(report(context)))throw new IOException("Cannot finalize report; previous report retained");
        }finally{pending.delete();for(File file:traces.values())file.delete();}
        return summary;
    }

    /** Isolate API30 references from the API21 entry screen. */
    private static final class Api30 {
        static void collect(Context context,File directory,StringBuilder text,Map<String,File> traces)throws IOException {
            ActivityManager manager=(ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
            if(manager==null){text.append("ActivityManager unavailable.\n");return;}
            List<ApplicationExitInfo> entries=manager.getHistoricalProcessExitReasons(context.getPackageName(),0,8);
            long remaining=ReportLimits.TRACE_LIMIT;int accepted=0;
            for(ApplicationExitInfo entry:entries){
                String process=entry.getProcessName();
                if(!ReportProcessScope.owns(context.getPackageName(),process))continue;
                accepted++;
                text.append("\nExit ").append(accepted).append(" timestamp_ms=").append(entry.getTimestamp()).append(" process=").append(process).append('\n');
                text.append("reason=").append(reason(entry.getReason())).append(" (").append(entry.getReason()).append(") status=").append(entry.getStatus()).append(" importance=").append(entry.getImportance()).append('\n');
                text.append("description=").append(ReportLimits.text(entry.getDescription(),1024)).append('\n');
                if(remaining<=0){text.append("Trace omitted: total4MiB report limit reached.\n");continue;}
                boolean nativeCrash=entry.getReason()==ApplicationExitInfo.REASON_CRASH_NATIVE;
                if(nativeCrash&&Build.VERSION.SDK_INT<31){text.append("Native tombstones are unavailable through this API before Android12/API31.\n");continue;}
                String name=(nativeCrash?"tombstone-":"trace-")+entry.getTimestamp()+"-"+accepted+(nativeCrash?".pb":".txt");
                File file=new File(directory,name);boolean retained=false;
                try(InputStream input=entry.getTraceInputStream()){
                    if(input==null){text.append("No retained trace (may be unavailable or overwritten).\n");continue;}
                    long bytes;
                    try(OutputStream output=new FileOutputStream(file)){bytes=ReportLimits.copy(input,output,remaining);}
                    traces.put(name,file);remaining-=bytes;retained=true;
                    text.append("Trace: ").append(name).append(" bytes=").append(bytes).append(nativeCrash?" (raw Android protobuf, not text)\n":" (system trace bytes)\n");
                }catch(ReportLimits.TooLarge e){text.append("Trace omitted: exceeds remaining report limit; no truncated protobuf exported.\n");}
                catch(IOException|SecurityException e){text.append("Trace unavailable: ").append(ReportLimits.text(e.toString(),300)).append('\n');}
                finally{if(!retained)file.delete();}
            }
            if(accepted==0)text.append("No retained app-exit records. Reproduce via Open launcher, then reopen this diagnostic screen. Android retention is not guaranteed.\n");
        }
        private static String reason(int value){
            switch(value){
                case ApplicationExitInfo.REASON_CRASH:return "Java crash";
                case ApplicationExitInfo.REASON_CRASH_NATIVE:return "Native crash";
                case ApplicationExitInfo.REASON_ANR:return "Not responding (ANR)";
                case ApplicationExitInfo.REASON_EXIT_SELF:return "Process exited itself";
                case ApplicationExitInfo.REASON_LOW_MEMORY:return "Low-memory termination";
                case ApplicationExitInfo.REASON_INITIALIZATION_FAILURE:return "Initialization failure";
                case ApplicationExitInfo.REASON_USER_REQUESTED:return "User/system request";
                case ApplicationExitInfo.REASON_SIGNALED:return "Signaled";
                default:return "Other/system reason";
            }
        }
    }
}
