"""Actual UI launch producer with queued-UI/framework fixtures; no Android runtime claim."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
TOOLS=next(p/'toolchain' for p in ROOT.parents if (p/'toolchain').is_dir())
JDK=sorted((TOOLS/'android/java').glob('jdk-*/bin'))[-1]
source=(ROOT/'port/android/diagnostics/StartupDiagnosticActivity.java').read_text(encoding='utf-8')
def block(start,text=None):
    text=source if text is None else text
    a=text.index(start);end=text.index('{',a)+1;depth=1
    while depth:
        if text[end]=='{':depth+=1
        elif text[end]=='}':depth-=1
        end+=1
    return text[a:end]
launch=block('private void launch(boolean graphical)')
result=block('if(request==PLAY_GAME||request==OPEN_GRAPHICAL)')
code=r'''
import java.io.*;
class LaunchFixture{
 volatile boolean working,collecting,busy;boolean childPending,safeChildPending;int starts;
 final int SAFE_GAME=50;
 final int PICK_ROM=10,PLAY_GAME=60,OPEN_GRAPHICAL=61;
 final String[] CODES={"B8CP","B8CE","B8CJ"},REGIONS={"eu","us","jp"};
 static class Spinner{int getSelectedItemPosition(){return 0;}}Spinner region=new Spinner();
 static class Context{File getFilesDir(){return new File("fixture");}}
 static class GameActivity{}
 static class Intent{Intent(){}Intent(Object c,Class<?>t){}String getStringExtra(String k){return "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";}Intent setClassName(String p,String c){return this;}Intent putExtra(String a,String b){return this;}}
 static class StartupGameAttempt{static boolean valid(String n){return true;}}
 static class RomImport{static String verifyInstalled(File file){return "B8CP";}}
 static class StartupProcessGate{static boolean awaitAbsent(Context c,String role){return true;}}
 static class StartupStageJournal{static void record(File f,String r,String n,int p,String stage,int code){}}
 static class Thread{Runnable task;Thread(Runnable r,String n){task=r;}void start(){task.run();}}
 java.util.ArrayDeque<Runnable> ui=new java.util.ArrayDeque<>();
 Context getApplicationContext(){return new Context();}String getPackageName(){return "com.rekhcom.android";}
 File getFilesDir(){return new File("fixture");}Intent getIntent(){return new Intent();}void progress(String s){}void finish(String s,boolean e){busy=false;}
 void controls(){}void collect(){}boolean isFinishing(){return false;}boolean isDestroyed(){return false;}
 String t(String es,String en){return en;}void showFailure(Exception e){throw new AssertionError(e);}
 void runOnUiThread(Runnable r){ui.add(r);}void startActivityForResult(Intent i,int request){starts++;}void startActivity(Intent i){starts++;}
 LAUNCH
 void result(int request){RESULT}
 NATIVE_LAUNCH
 void nativeResult(int request){NATIVE_RESULT}
 public static void main(String[]args){
  LaunchFixture a=new LaunchFixture();a.launch(false);a.ui.remove().run();
  // The first child has NOT registered a process yet; the gate still says absent.
  a.launch(false);while(!a.ui.isEmpty())a.ui.remove().run();
  if(a.starts!=1)throw new AssertionError("two launch requests admitted before child registration");
  a.result(a.PICK_ROM);if(!a.working)throw new AssertionError("unrelated picker/result released latch");
  a.result(a.PLAY_GAME);if(a.working||a.childPending)throw new AssertionError("child result did not release latch");
  a.launch(false);a.ui.remove().run();if(a.starts!=2)throw new AssertionError("next real attempt rejected");
  LaunchFixture g=new LaunchFixture();g.nativeLaunch("eu","unused");g.nativeLaunch("eu","unused");while(!g.ui.isEmpty())g.ui.remove().run();
  g.nativeLaunch("eu","unused");while(!g.ui.isEmpty())g.ui.remove().run();if(g.starts!=1||!g.busy)throw new AssertionError("graphical any-thread launch admission not latched");
  g.nativeResult(g.SAFE_GAME);g.nativeLaunch("eu","unused");while(!g.ui.isEmpty())g.ui.remove().run();if(g.starts!=2)throw new AssertionError("graphical child result did not release latch");
  System.out.println("PASS actual queued UI launch producer; child-result latch; no runtime claim");
 }
}
'''
with tempfile.TemporaryDirectory(prefix='startup-launch-') as directory:
    out=Path(directory);stub=out/'android/os/Process.java';stub.parent.mkdir(parents=True);stub.write_text('package android.os;public class Process{public static int myPid(){return 123;}}')
    for mutant in (False,True):
        actual=launch.replace('runOnUiThread(()->{','runOnUiThread(()->{working=false;',1) if mutant else launch
        graphical=(ROOT/'port/android/app/src/main/java/com/rekhcom/android/LauncherActivity.java').read_text(encoding='utf-8')
        native=block('public void nativeLaunch(',graphical)+'\n'+block('private void safeLaunchGame(',graphical)
        native_result=block('if(request==SAFE_GAME)',graphical)
        java=out/'LaunchFixture.java';java.write_text(code.replace('NATIVE_LAUNCH',native).replace('NATIVE_RESULT',native_result).replace('LAUNCH',actual).replace('RESULT',result),encoding='utf-8')
        subprocess.run([str(JDK/'javac.exe'),'-encoding','UTF-8','-d',str(out),str(java),str(stub)],check=True)
        run=subprocess.run([str(JDK/'java.exe'),'-cp',str(out),'LaunchFixture'],capture_output=True,text=True)
        if mutant:
            if run.returncode==0:raise RuntimeError('Prior-WIP early-release mutation was not detected')
            print('PASS negative mutation control: early working=false admits two child starts (fixture, not reporter crash)')
        elif run.returncode:raise RuntimeError(run.stderr)
        else:print(run.stdout.strip())
