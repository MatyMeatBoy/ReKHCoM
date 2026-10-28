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
        StringBuilder text=new StringBuilder("ReKHCoM optional startup diagnostic (not a crash fix)\n");
        text.append("Current build: 2.2-test412; base APK SHA256 bd7999c234669ac88dfd5ec22ae3ccddd95db011eda0a1608fb8526819e19a4e\n");
        text.append("Model: ").append(ReportLimits.text(Build.MANUFACTURER+" "+Build.MODEL,200)).append("\nAndroid API: ").append(Build.VERSION.SDK_INT).append('\n');
        text.append("Only this app's exit records. Exit reason is not a verified root cause. Updates/force-stop may add unrelated exit records.\n");
        File profiles=new File(directory,"profile-attempts.txt");
        if(profiles.isFile()&&profiles.length()<=4096){
            try(InputStream input=new FileInputStream(profiles);ByteArrayOutputStream data=new ByteArrayOutputStream()){
                ReportLimits.copy(input,data,4096);text.append("\nLauncher profile attempts (timestamps; not proof of crash cause):\n").append(new String(data.toByteArray(),StandardCharsets.UTF_8));
            }catch(IOException e){text.append("Profile evidence unavailable.\n");}
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
                if(!process.equals(context.getPackageName())&&!process.equals(context.getPackageName()+":game"))continue;
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
