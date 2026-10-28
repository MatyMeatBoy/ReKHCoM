package com.rekhcom.android;

import android.app.ActivityManager;
import android.os.Bundle;
import java.io.File;
import org.libsdl.app.SDLActivity;

/** Optional safe-entry instrumentation. The Java entry itself never extends SDL. */
abstract class StartupSdlActivity extends SDLActivity {
    private static final StartupGameAttempt.Registry attempts=new StartupGameAttempt.Registry();
    private StartupGameAttempt attempt;
    protected abstract String startupRole();
    private File startupFiles(){return new File(getApplicationInfo().dataDir,"files");}
    private void stage(String name,int code,boolean async){
        StartupGameAttempt own=attempt;if(own==null)return;
        if(async)StartupStageJournal.async(startupFiles(),startupRole(),own.nonce,own.pid,own.generation,name,code);
        else StartupStageJournal.record(startupFiles(),startupRole(),own.nonce,own.pid,own.generation,name,code);
    }
    @Override protected void onCreate(Bundle state){
        String nonce=getIntent().getStringExtra("startup_attempt");
        if(StartupGameAttempt.valid(nonce))attempt=attempts.begin(nonce,android.os.Process.myPid());
        stage("before_sdl_create",0,true);
        super.onCreate(state);
        stage("after_sdl_create",0,true); // Activity method returned, not proof of a working native window.
    }
    @Override protected void main(){
        if(attempt==null){super.main();return;}
        String library=getMainSharedObject(),function=getMainFunction();String[] arguments=getArguments();
        stage("native_enter",0,false); // SDLThread: durable marker before nativeRunMain.
        try{int result=SDLActivity.nativeRunMain(library,function,arguments);stage("native_return",result,false);}
        catch(RuntimeException|Error failure){stage("native_throw",-1,false);throw failure;}
    }
    private boolean ownProcess(StartupGameAttempt own,String role){
        if(own.pid!=android.os.Process.myPid())return false;
        try{
            ActivityManager manager=(ActivityManager)getSystemService(ACTIVITY_SERVICE);
            java.util.List<ActivityManager.RunningAppProcessInfo> processes=manager==null?null:manager.getRunningAppProcesses();
            if(processes!=null)for(ActivityManager.RunningAppProcessInfo process:processes)
                if(process.pid==own.pid)return (getPackageName()+":"+role).equals(process.processName);
        }catch(SecurityException unavailable){} // No permission to prove ownership means no retirement.
        return false;
    }
    @Override protected void onDestroy(){
        final StartupGameAttempt own=attempt;
        final Thread sdlThread=mSDLThread;
        final boolean finishing=isFinishing(),changing=isChangingConfigurations();
        final String role=startupRole();
        stage("destroy",0,true);
        super.onDestroy(); // SDL sends Quit, joins its thread, and performs native cleanup first.
        if(own==null||!finishing||changing)return;
        final File files=startupFiles();
        new Thread(()->{
            boolean terminal=sdlThread==null||!sdlThread.isAlive();
            StartupStageJournal.record(files,role,own.nonce,own.pid,own.generation,"retire_requested",terminal?1:0);
            boolean retired=ownProcess(own,role)&&attempts.retire(own,terminal,finishing,changing,
                ()->android.os.Process.killProcess(own.pid));
            if(!retired)StartupStageJournal.record(files,role,own.nonce,own.pid,own.generation,"retire_skipped",0);
        },"startup-owned-retire").start();
    }
}
