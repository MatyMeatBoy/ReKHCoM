package com.rekhcom.android;

import org.libsdl.app.SDLActivity;

/** Runs the selected native core in :game, separate from the ROM launcher. */
public class GameActivity extends StartupSdlActivity {
    @Override protected String startupRole(){return "game";}
    private LifecycleTrace lifecycle;
    private String lifecycleRuntime="pending";
    private void lifecycle(String event){if(lifecycle!=null)lifecycle.record(this,event,lifecycleRuntime);}
    @Override protected void onCreate(android.os.Bundle state){
        try{lifecycle=LifecycleTrace.attach(this,"game",state!=null);}catch(Throwable ignored){}
        lifecycle("onCreate");super.onCreate(state);
    }
    @Override protected void onPause(){lifecycle("onPause");super.onPause();}
    @Override protected void onStop(){lifecycle("onStop");super.onStop();}
    @Override protected void onDestroy(){lifecycle("onDestroy");super.onDestroy();}
    private String region() {
        String r=getIntent().getStringExtra("region");
        return "us".equals(r)||"jp".equals(r)?r:"eu";
    }
    /** The 64-bit host where Android installed the 64-bit libraries (any 64-bit phone: ANDROID_64.md), else the
     *  region's 32-bit core. */
    private boolean use64() {
        return new java.io.File(getApplicationInfo().nativeLibraryDir,"librekhcom64.so").exists();
    }
    @Override protected String[] getLibraries() {
        boolean host64=use64();
        lifecycleRuntime=host64?"host64":"native32-"+region();lifecycle("runtime");
        return host64 ? new String[]{"SDL3","rekhcom64"} : new String[]{"SDL3","rekhcom_"+region()};
    }
    /** Test app only (application id ending in ".smoke"): KEY=VALUE lines of files/smoke_env.txt become
     *  environment variables, so one APK runs many harness scenarios. The normal app never reads them. */
    private void smokeEnvironment() {
        if (!getPackageName().endsWith(".smoke")) return;
        try {
            java.io.File file=new java.io.File(getFilesDir(),"smoke_env.txt");
            if (!file.exists()) return;
            java.io.ByteArrayOutputStream bytes=new java.io.ByteArrayOutputStream();
            try (java.io.FileInputStream in=new java.io.FileInputStream(file)) {
                byte[] chunk=new byte[4096];
                for (int n; (n=in.read(chunk))>0;) bytes.write(chunk,0,n);
            }
            for (String line: bytes.toString("UTF-8").split("\r?\n")) {
                int eq=line.indexOf('=');
                if (eq>0) nativeSetenv(line.substring(0,eq).trim(),line.substring(eq+1).trim());
            }
        } catch (Exception e) { android.util.Log.w("ReKHCoM","smoke env: "+e); }
    }
    /** Called by the 64-bit host's error box (android64_host.c, ShareLog): the logs and the device as plain text in
     *  Android's share sheet, so a tester can send them without looking for files (issues #22, #34). */
    public void shareLog() {
        StringBuilder text=new StringBuilder();
        text.append("ReKHCoM log\nDevice: ").append(android.os.Build.MANUFACTURER).append(' ').append(android.os.Build.MODEL)
            .append("\nAndroid ").append(android.os.Build.VERSION.RELEASE).append(" (API ").append(android.os.Build.VERSION.SDK_INT)
            .append(")\nABIs: ").append(android.text.TextUtils.join(",",android.os.Build.SUPPORTED_ABIS)).append('\n');
        text.append("\n").append(LifecycleTrace.summary(this));
        for (String name: new String[]{"kh_com_host.log","kh_com.log"}) {
            java.io.File file=new java.io.File(getFilesDir(),name);
            if (!file.exists()) continue;
            text.append("\n--- ").append(name).append(" ---\n");
            try (java.io.FileInputStream in=new java.io.FileInputStream(file)) {
                byte[] data=new byte[(int)Math.min(file.length(),28000)];
                int n=in.read(data);
                if (n>0) text.append(new String(data,0,n,"UTF-8"));
            } catch (Exception e) { text.append("Log unavailable\n"); }
        }
        android.content.Intent send=new android.content.Intent(android.content.Intent.ACTION_SEND);
        send.setType("text/plain");
        send.putExtra(android.content.Intent.EXTRA_SUBJECT,"ReKHCoM log");
        send.putExtra(android.content.Intent.EXTRA_TEXT,LifecycleJournal.clipShare(text.toString()));
        android.content.Intent chooser=android.content.Intent.createChooser(send,"ReKHCoM log");
        chooser.addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(chooser);
    }
    @Override protected String[] getArguments() {
        smokeEnvironment();
        /* No --language: the European cartridge asks for it itself. */
        return new String[]{"--data-dir",getFilesDir().getAbsolutePath(),"--rom",
            getIntent().getStringExtra("rom")};
    }
}
