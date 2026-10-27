package com.rekhcom.android;

import android.app.Activity;
import android.util.Log;
import java.io.File;
import java.util.concurrent.atomic.AtomicLong;

/** Per-process writer, per-Activity identity; contains no Activity reference. */
final class LifecycleTrace {
    private static final AtomicLong nextInstance=new AtomicLong();
    private static LifecycleJournal game,launcher;
    private final LifecycleJournal journal;
    private final long instance=nextInstance.incrementAndGet();
    private final boolean restored;
    private LifecycleTrace(LifecycleJournal journal,boolean restored){this.journal=journal;this.restored=restored;}
    static synchronized LifecycleTrace attach(Activity activity,String role,boolean restored){
        try {
            LifecycleJournal journal="game".equals(role)?game:launcher;
            if(journal==null){
                // No getFilesDir/mkdir or other filesystem work on the lifecycle thread.
                File directory=new File(activity.getApplicationInfo().dataDir,"files");
                journal=new LifecycleJournal(directory,role,android.os.Process.myPid());
                if("game".equals(role))game=journal;else launcher=journal;
            }
            return new LifecycleTrace(journal,restored);
        }catch(Throwable ignored){return null;}
    }
    void record(Activity activity,String event,String runtime){
        try {
            String line=journal.line(instance,event,runtime,activity.isChangingConfigurations(),
                activity.getChangingConfigurations(),activity.isFinishing(),restored);
            boolean queued=journal.enqueue(line);
            Log.i("ReKHCoMLifecycle",line+" queued="+queued+" disk_failed="+journal.diskFailed());
        }catch(Throwable ignored){} // Diagnostics never alter an Activity transition.
    }
    static String summary(Activity activity){
        try{return LifecycleJournal.summary(new File(activity.getApplicationInfo().dataDir,"files"));}
        catch(Throwable ignored){return "Lifecycle diagnostics unavailable.\n";}
    }
}
