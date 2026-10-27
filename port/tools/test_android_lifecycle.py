"""Plain Java lifecycle logger contracts; no Android process, app or native build."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
TOOLS=next(p/'toolchain' for p in ROOT.parents if (p/'toolchain').is_dir())
JDK=sorted((TOOLS/'android/java').glob('jdk-*/bin'))[-1]
with tempfile.TemporaryDirectory(prefix='android-lifecycle-') as directory:
    subprocess.run([str(JDK/'javac.exe'),'-encoding','UTF-8','-d',directory,
        str(ROOT/'port/android/app/src/main/java/com/rekhcom/android/LifecycleJournal.java'),
        str(ROOT/'port/tools/LifecycleJournalTest.java')],check=True)
    subprocess.run([str(JDK/'java.exe'),'-cp',directory,'com.rekhcom.android.LifecycleJournalTest',
                    str(Path(directory)/'files')],check=True,timeout=20)
