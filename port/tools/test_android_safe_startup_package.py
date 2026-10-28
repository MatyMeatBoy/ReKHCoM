"""Real manifest/template contracts and -O rejection; not an install/runtime test."""
from pathlib import Path
import subprocess,sys,tempfile
import build_android_safe_startup as build
ROOT=Path(__file__).resolve().parents[2]
TOOLS=next(p/'toolchain' for p in ROOT.parents if (p/'toolchain').is_dir())
original=(ROOT/'port/android/app/src/main/AndroidManifest.xml').read_text(encoding='utf-8')
template=(TOOLS/'android/SDL3-3.4.18/android-project/app/src/main/AndroidManifest.xml').read_text(encoding='utf-8')
def reject(text):
    try:build.validate_manifest(text,template)
    except ValueError:return
    raise RuntimeError('Invalid manifest accepted')
reject(original) # Actual baseline is SDL-first, not the promised Java-only entry.
current=build.safe_manifest(original,template);build.validate_manifest(current,template)
reject(current.replace(':game',':startup-profile'))
reject(current.replace('android:exported="false"','android:exported="true"',1))
reject(current.replace('StartupDiagnosticActivity','GameActivity'))
reject(current.replace('|fontWeightAdjustment',''))
reject(current.replace('<application','<application data="SDL_ENV.KHCOM_QUIT"',1))
activity=(ROOT/'port/android/diagnostics/StartupDiagnosticActivity.java').read_text(encoding='utf-8')
if 'extends Activity' not in activity or 'extends SDLActivity' in activity or 'System.loadLibrary' in activity:
    raise RuntimeError('Safe entry depends on SDL/native loading')
if 'GameActivity.class' in activity or 'LauncherActivity.class' in activity:
    raise RuntimeError('Use string intents; do not load SDL subclasses in the Java entry')
code="import sys;sys.path.insert(0,sys.argv[1]);import build_android_safe_startup as b;b.require(False,'expected invalid package')"
result=subprocess.run([sys.executable,'-O','-c',code,str(ROOT/'port/tools')],capture_output=True,text=True)
if result.returncode==0:raise RuntimeError('Package validator vanished under python -O')
with tempfile.TemporaryDirectory(prefix='legacy-startup-') as directory:
    output=Path(directory)/'must-not-exist'
    old=subprocess.run([sys.executable,'-O',str(ROOT/'port/tools/build_android_diagnostics.py'),'--base-apk','placeholder','--gradle-seed','placeholder','--toolchain','placeholder','--out',str(output)],capture_output=True,text=True)
    if old.returncode!=2 or 'Historical test412 native bytes' not in old.stderr or output.exists():raise RuntimeError('Legacy native412 builder did not stop before staging under -O')
    jdk=sorted((TOOLS/'android/java').glob('jdk-*/bin'))[-1]
    subprocess.run([str(jdk/'javac.exe'),'-d',directory,str(ROOT/'port/android/diagnostics/ReportProcessScope.java'),str(ROOT/'port/android/diagnostics/ReportProcessScopeTest.java')],check=True)
    subprocess.run([str(jdk/'java.exe'),'-cp',directory,'com.rekhcom.android.ReportProcessScopeTest'],check=True)
print('PASS safe startup manifest/source/-O controls; actual baseline SDL-first rejected; no install/runtime claim')
