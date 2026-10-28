"""Portable ownership/journal controls. Does not certify Android lifecycle/runtime."""
from pathlib import Path
import argparse,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
TOOLS=next(p/'toolchain' for p in ROOT.parents if (p/'toolchain').is_dir())
JDK=sorted((TOOLS/'android/java').glob('jdk-*/bin'))[-1]
parser=argparse.ArgumentParser();parser.add_argument('--rom',type=Path);args=parser.parse_args()
with tempfile.TemporaryDirectory(prefix='safe-startup-jvm-') as temp:
    source=ROOT/'port/android/app/src/main/java/com/rekhcom/android'
    subprocess.run([str(JDK/'javac.exe'),'-encoding','UTF-8','-d',temp,str(source/'StartupGameAttempt.java'),str(source/'StartupStageJournal.java'),str(source/'RomImport.java'),str(ROOT/'port/tools/StartupGameAttemptTest.java')],check=True)
    subprocess.run([str(JDK/'java.exe'),'-cp',temp,'com.rekhcom.android.StartupGameAttemptTest',str(Path(temp)/'files'),*([str(args.rom.resolve())] if args.rom else [])],check=True,timeout=20)
