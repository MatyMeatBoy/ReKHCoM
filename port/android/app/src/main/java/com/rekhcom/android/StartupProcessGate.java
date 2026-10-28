package com.rekhcom.android;
import android.app.ActivityManager;
import android.content.Context;
import android.os.Looper;
import java.io.IOException;

/** Bounded worker-only check. Never stops a running game or an unrelated PID. */
final class StartupProcessGate {
    static boolean awaitAbsent(Context context,String role)throws IOException,InterruptedException{
        if(Looper.myLooper()==Looper.getMainLooper())throw new IllegalStateException("Startup gate requires worker thread");
        if(!"game".equals(role)&&!"launcher".equals(role))throw new IllegalArgumentException("role");
        ActivityManager manager=(ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
        if(manager==null)throw new IOException("Process information unavailable");
        for(int poll=0;poll<12;poll++){
            java.util.List<ActivityManager.RunningAppProcessInfo> list=manager.getRunningAppProcesses();
            if(list==null)throw new IOException("Process information unavailable");
            boolean running=false;for(ActivityManager.RunningAppProcessInfo process:list)
                if((context.getPackageName()+":"+role).equals(process.processName))running=true;
            if(!running)return true;
            Thread.sleep(80);
        }
        return false;
    }
}
