package com.rekhcom.android;

import android.content.Context;
import android.system.Os;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.Objects;

/** Plain Java gate; setting environment never loads SDL or changes saved preferences. */
final class StartupProfileSession {
 private static final String ORIGINAL=Os.getenv(StartupProfiles.KEY);
 private static String mode="default",expected=ORIGINAL;
 private static void set(String value)throws Exception{
  if(value==null)Os.unsetenv(StartupProfiles.KEY);else Os.setenv(StartupProfiles.KEY,value,true);
 }
 static synchronized void prepare(Context context,String requested)throws Exception{
  String value=StartupProfiles.value(requested,ORIGINAL); // Reject unknown before touching environment.
  try{set(value);}catch(Exception e){try{set(ORIGINAL);}catch(Exception ignored){}throw e;}
  mode=requested;expected=value;
  note(context,"requested="+mode+" libc="+display(Os.getenv(StartupProfiles.KEY)));
 }
 static synchronized void rejected(Context context){mode="default-rejected";expected=ORIGINAL;note(context,"request rejected; using original default");}
 static boolean isOwnProfileProcess(Context context){
  String own=context.getPackageName()+":startup-profile";
  android.app.ActivityManager manager=(android.app.ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
  if(manager==null)return false;
  java.util.List<android.app.ActivityManager.RunningAppProcessInfo> processes=manager.getRunningAppProcesses();
  if(processes!=null)for(android.app.ActivityManager.RunningAppProcessInfo p:processes)if(p.pid==android.os.Process.myPid())return own.equals(p.processName);
  return false;
 }
 static synchronized boolean verify(Context context,String actual){
  boolean ok=Objects.equals(expected,actual);
  note(context,"effective="+mode+" SDL_hint="+display(actual)+" match="+ok);
  return ok;
 }
 static synchronized void restore(Context context){
  try{set(ORIGINAL);note(context,"restored libc="+display(Os.getenv(StartupProfiles.KEY)));}
  catch(Exception e){android.util.Log.e("StartupProfile","Environment restoration failed",e);}
 }
 private static String display(String value){return value==null?"<absent>":value;}
 private static void note(Context context,String message){
  String line=System.currentTimeMillis()+" pid="+android.os.Process.myPid()+" "+message;
  android.util.Log.i("StartupProfile",line);
  try{
   File dir=new File(context.getCacheDir(),"startup-diagnostics");if(!dir.isDirectory()&&!dir.mkdirs())return;
   File file=new File(dir,"profile-attempts.txt");String old="";
   if(file.isFile()&&file.length()<=4096){byte[] bytes=new byte[(int)file.length()];try(InputStream in=new FileInputStream(file)){int n=in.read(bytes);if(n>0)old=new String(bytes,0,n,StandardCharsets.UTF_8);}}
   String text=old+line+"\n";if(text.length()>4096)text=text.substring(text.length()-4096);
   try(OutputStream out=new FileOutputStream(file)){out.write(text.getBytes(StandardCharsets.UTF_8));}
  }catch(Exception e){android.util.Log.w("StartupProfile","Could not record profile attempt",e);}
 }
}
