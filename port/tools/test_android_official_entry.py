"""Actual official staging hook, shared manifest validator and AAPT35; no install claim."""
from pathlib import Path
import argparse,subprocess,shutil,sys,xml.etree.ElementTree as ET
import build_android as build
import build_android_safe_startup as safe
ROOT=Path(__file__).resolve().parents[2]
tools=next(p/'toolchain' for p in ROOT.parents if (p/'toolchain').is_dir())
p=argparse.ArgumentParser();p.add_argument('--work',type=Path,required=True);args=p.parse_args();args.work.mkdir(parents=True,exist_ok=True)
sdl=tools/'android/SDL3-3.4.18';template=(sdl/'android-project/app/src/main/AndroidManifest.xml').read_text(encoding='utf-8')
original=(ROOT/'port/android/app/src/main/AndroidManifest.xml').read_text(encoding='utf-8')
for name,identity in [('official',build.package_identity('v2.2.5')),('separate',build.package_identity('v2.2.5',test_build=True,test_version='2.2.5-nightly.20261028.1'))]:
 stage=args.work/name;source=stage/'app/src/main';shutil.copytree(ROOT/'port/android/app/src/main',source,dirs_exist_ok=True)
 if name=='separate':(source/'AndroidManifest.xml').write_text(build.playable_test_manifest(original,identity),encoding='utf-8')
 if hasattr(build,'prepare_java_first_entry'):
  if not build.prepare_java_first_entry(stage,sdl,identity):raise RuntimeError('Official entry not prepared')
 manifest=(source/'AndroidManifest.xml').read_text(encoding='utf-8')
 if hasattr(build,'prepare_java_first_entry'):safe.validate_manifest(manifest,template,application_id=identity['application_id'])
 else:safe.validate_manifest(manifest,template)
 app=ET.fromstring(manifest).find('application');ns=safe.NS
 expected_label='ReKHCoM Beta' if name=='official' else identity['label']
 if app.get(ns+'label')!=expected_label:raise RuntimeError('Package label replaced with test-only label')
 provider=app.find('provider')
 if provider.get(ns+'authorities')!=identity['application_id']+'.startup-report':raise RuntimeError('Report provider collides across app identities')
 for path in (ROOT/'port/android/diagnostics').glob('*.java'):
  copied=source/'java/com/rekhcom/android'/path.name
  if path.name.endswith('Test.java'):
   if copied.exists():raise RuntimeError('Test-only Java packaged')
  elif copied.read_bytes()!=path.read_bytes():raise RuntimeError('Diagnostic class not staged exactly')
 aapt=tools/'android/sdk/build-tools/35.0.0/aapt.exe';android=tools/'android/sdk/platforms/android-35/android.jar'
 # Gradle supplies package/version/minSdk; model that merge for standalone AAPT.
 merged=ET.fromstring(manifest);merged.set('package',identity['application_id']);merged.set(ns+'versionCode',str(identity['version_code']));merged.set(ns+'versionName',identity['version_name'])
 ET.SubElement(merged,'uses-sdk',{ns+'minSdkVersion':'21',ns+'targetSdkVersion':'35'})
 merged_path=stage/'aapt/AndroidManifest.xml';merged_path.parent.mkdir(exist_ok=True);merged_path.write_text(ET.tostring(merged,encoding='unicode'),encoding='utf-8')
 subprocess.run([str(aapt),'package','-f','-M',str(merged_path),'-S',str(source/'res'),'-I',str(android),'-F',str(stage/'manifest.ap_')],check=True)
 badging=subprocess.check_output([str(aapt),'dump','badging',str(stage/'manifest.ap_')],text=True,encoding='utf-8')
 if "launchable-activity: name='com.rekhcom.android.StartupDiagnosticActivity'" not in badging:raise RuntimeError('AAPT entry is not Java-first')
 (stage/'BADGING.txt').write_text(badging,encoding='utf-8')
 if name=='official' and identity['version_code']!=102020599:raise RuntimeError('Official patch version changed')
 broken=manifest.replace(identity['application_id']+'.startup-report','wrong.startup-report')
 try:safe.validate_manifest(broken,template,application_id=identity['application_id'])
 except ValueError:pass
 else:raise RuntimeError('Wrong provider identity accepted')
stage=args.work/'smoke';source=stage/'app/src/main';source.mkdir(parents=True,exist_ok=True)
(source/'AndroidManifest.xml').write_text(original,encoding='utf-8')
if build.prepare_java_first_entry(stage,sdl,build.package_identity('v2.2.5',smoke=True),smoke=True):raise RuntimeError('Smoke path unexpectedly transformed')
if (source/'AndroidManifest.xml').read_text(encoding='utf-8')!=original:raise RuntimeError('Smoke entry changed')
print('PASS official/separate Java-first entry, shared manifest validation, AAPT35, provider isolation, exact Java staging, version102020599 and unchanged smoke path; install/runtime unverified')
