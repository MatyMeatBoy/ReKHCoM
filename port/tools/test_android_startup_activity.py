"""Execute actual StartupSdlActivity with JVM framework fixtures, not Android runtime."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
TOOLS=next(p/'toolchain' for p in ROOT.parents if (p/'toolchain').is_dir())
JDK=sorted((TOOLS/'android/java').glob('jdk-*/bin'))[-1]
STUBS={
'android/os/Bundle.java':'package android.os;public class Bundle{}',
'android/os/Process.java':'package android.os;public class Process{public static volatile int kills;public static int myPid(){return 123;}public static void killProcess(int pid){if(pid!=123)throw new AssertionError();kills++;}}',
'android/content/Intent.java':'package android.content;public class Intent{public String nonce;public String getStringExtra(String key){return nonce;}}',
'android/content/pm/ApplicationInfo.java':'package android.content.pm;public class ApplicationInfo{public String dataDir;}',
'android/app/ActivityManager.java':'''package android.app;public class ActivityManager{
public static class RunningAppProcessInfo{public int pid=123;public String processName="com.rekhcom.android:game";}
public java.util.List<RunningAppProcessInfo> getRunningAppProcesses(){return java.util.Arrays.asList(new RunningAppProcessInfo());}}''',
'org/libsdl/app/SDLActivity.java':'''package org.libsdl.app;
public class SDLActivity{
 public static final String ACTIVITY_SERVICE="activity";
 protected static Thread mSDLThread;public boolean finishing,changing;public String directory;
 public android.content.Intent intent=new android.content.Intent();
 public static int result,normalMain;public static boolean throwMain;
 public android.content.Intent getIntent(){return intent;}
 public android.content.pm.ApplicationInfo getApplicationInfo(){android.content.pm.ApplicationInfo info=new android.content.pm.ApplicationInfo();info.dataDir=directory;return info;}
 public String getPackageName(){return "com.rekhcom.android";}
 public Object getSystemService(String key){return new android.app.ActivityManager();}
 public boolean isFinishing(){return finishing;}public boolean isChangingConfigurations(){return changing;}
 protected void onCreate(android.os.Bundle b){}protected void onDestroy(){}
 protected void main(){normalMain++;}
 protected String getMainSharedObject(){return "libfixture";}protected String getMainFunction(){return "SDL_main";}protected String[] getArguments(){return new String[]{"owned-fixture"};}
 public static int nativeRunMain(String library,String function,Object arguments){if(!library.equals("libfixture")||!function.equals("SDL_main"))throw new AssertionError();if(throwMain)throw new IllegalStateException("fixture native loader");return result;}
}
''',
'com/rekhcom/android/StartupActivityTest.java':'''package com.rekhcom.android;
public class StartupActivityTest{
 static class Game extends StartupSdlActivity{protected String startupRole(){return "game";}void thread(Thread t){mSDLThread=t;}}
 static int checks;static void check(boolean ok){checks++;if(!ok)throw new AssertionError("check"+checks);}
 static Game create(String dir,String nonce){Game game=new Game();game.directory=dir;game.intent.nonce=nonce;game.onCreate(new android.os.Bundle());return game;}
 static void waitRetire()throws Exception{
  for(int n=0;n<200;n++){boolean alive=false;for(Thread t:Thread.getAllStackTraces().keySet())if(t.isAlive()&&t.getName().equals("startup-owned-retire"))alive=true;if(!alive)return;Thread.sleep(10);}
  throw new AssertionError("Fixture retirement observer timed out");
 }
 public static void main(String[]args)throws Exception{
  Game a=create(args[0],"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");org.libsdl.app.SDLActivity.result=1;a.main();
  String text=StartupStageJournal.summary(new java.io.File(args[0],"files"));check(text.contains("stage=native_return code=1"));
  Game b=create(args[0],"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");a.finishing=true;a.onDestroy();waitRetire();check(android.os.Process.kills==0);
  b.finishing=true;b.changing=true;b.onDestroy();waitRetire();check(android.os.Process.kills==0);b.changing=false;
  java.util.concurrent.CountDownLatch hold=new java.util.concurrent.CountDownLatch(1);Thread live=new Thread(()->{try{hold.await();}catch(Exception e){}});live.start();b.thread(live);b.onDestroy();waitRetire();check(android.os.Process.kills==0);hold.countDown();live.join();
  Game c=create(args[0],"cccccccccccccccccccccccccccccccc");c.thread(null);org.libsdl.app.SDLActivity.throwMain=true;
  try{c.main();throw new AssertionError();}catch(IllegalStateException expected){checks++;}org.libsdl.app.SDLActivity.throwMain=false;
  check(StartupStageJournal.summary(new java.io.File(args[0],"files")).contains("stage=native_throw code=-1"));
  c.finishing=true;c.onDestroy();waitRetire();check(android.os.Process.kills==1);
  Game normal=create(args[0],null);normal.main();check(org.libsdl.app.SDLActivity.normalMain==1);normal.finishing=true;normal.onDestroy();waitRetire();check(android.os.Process.kills==1);
  System.out.println("PASS actual StartupSdlActivity callbacks: "+checks+" checks; framework/native-call fixtures, NOT Android lifecycle certification");
 }
}
'''}
with tempfile.TemporaryDirectory(prefix='startup-activity-jvm-') as temp:
    out=Path(temp);files=[]
    for name,text in STUBS.items():
        file=out/name;file.parent.mkdir(parents=True,exist_ok=True);file.write_text(text);files.append(file)
    product=ROOT/'port/android/app/src/main/java/com/rekhcom/android'
    files += [product/name for name in ('StartupGameAttempt.java','StartupStageJournal.java','StartupSdlActivity.java')]
    subprocess.run([str(JDK/'javac.exe'),'-encoding','UTF-8','-d',str(out/'classes'),*map(str,files)],check=True)
    subprocess.run([str(JDK/'java.exe'),'-cp',str(out/'classes'),'com.rekhcom.android.StartupActivityTest',str(out/'data')],check=True,timeout=10)
