"""Build optional Java-first test412 APK using exact public2.2 native/assets bytes.

No native compilation, SDK download or mutation of the normal Android build.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import zipfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
PIN = '38f84bd36b176c926be2d236fdf320eb844c152b'
BASE_SHA = 'bd7999c234669ac88dfd5ec22ae3ccddd95db011eda0a1608fb8526819e19a4e'
CERT = '3a5a1740e8171c82b505907da26f3039e2153aeb8d16c2c22477d52edf74ecdc'
NS = '{http://schemas.android.com/apk/res/android}'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base-apk', type=Path, required=True)
    parser.add_argument('--gradle-seed', type=Path, required=True, help='Existing generated project; wrapper/config read-only')
    parser.add_argument('--toolchain', type=Path, required=True, help='Existing toolchain/android directory')
    parser.add_argument('--out', type=Path, required=True, help='New dedicated output directory')
    args = parser.parse_args()
    args.base_apk = args.base_apk.resolve()
    args.gradle_seed = args.gradle_seed.resolve()
    args.out = args.out.resolve()
    assert sha(args.base_apk.read_bytes()) == BASE_SHA, 'Wrong public APK'
    assert not args.out.exists(), 'Preserve prior build/evidence; select a new output directory'
    stage = args.out.resolve() / 'gradle'
    source = stage / 'app/src/main'
    source.mkdir(parents=True)
    android_tools = args.toolchain.resolve()
    sdk = android_tools / 'sdk'
    assert (sdk / 'platforms/android-35/android.jar').is_file(), 'Existing SDK35 required; downloads are not allowed'
    assert (sdk / 'build-tools/35.0.0/aapt.exe').is_file(), 'Existing build-tools35 required'
    java = next((android_tools / 'java').glob('jdk-17*'))
    env = dict(os.environ, JAVA_HOME=str(java), ANDROID_HOME=str(sdk), ANDROID_SDK_ROOT=str(sdk), GRADLE_USER_HOME=str(android_tools / 'gradle-cache'))
    env['PATH'] = str(java / 'bin') + os.pathsep + env.get('PATH', '')
    immutable = {}
    with zipfile.ZipFile(args.base_apk) as archive:
        assert archive.testzip() is None
        for name in archive.namelist():
            if not (name.startswith('assets/') or name.startswith('lib/')) or name.endswith('/'):
                continue
            assert not Path(name).is_absolute() and '..' not in Path(name).parts
            data = archive.read(name)
            immutable[name] = sha(data)
            relative = Path('jniLibs') / Path(name).relative_to('lib') if name.startswith('lib/') else Path(name)
            target = source / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
    assert sum(name.startswith('lib/') and name.endswith('.so') for name in immutable) == 8
    # Source Java/resources of the exact binary producer, not current game code.
    files = subprocess.check_output(['git', '-C', str(ROOT), 'ls-tree', '-r', '--name-only', PIN, '--', 'port/android/app/src/main'], text=True).splitlines()
    prefix = 'port/android/app/src/main/'
    for name in files:
        if name == prefix + 'AndroidManifest.xml' or name.startswith(prefix + 'java/') or name.startswith(prefix + 'res/'):
            target = source / name[len(prefix):]
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(subprocess.check_output(['git', '-C', str(ROOT), 'show', PIN + ':' + name]))
    sdl_java = android_tools / 'SDL3-3.4.18/android-project/app/src/main/java/org'
    shutil.copytree(sdl_java, source / 'java/org')
    diagnostic = ROOT / 'port/android/diagnostics'
    for file in diagnostic.glob('*.java'):
        if not file.name.endswith('Test.java'):
            shutil.copy2(file, source / 'java/com/rekhcom/android' / file.name)
    manifest = source / 'AndroidManifest.xml'
    ET.register_namespace('android', NS[1:-1])
    tree = ET.parse(manifest)
    root = tree.getroot()
    for permission in list(root.findall('uses-permission')):
        root.remove(permission)  # No networking/global privileges in this diagnostic variant.
    application = root.find('application')
    application.set(NS + 'label', 'ReKHCoM Startup Diagnostic')
    launcher = next(a for a in application.findall('activity') if a.get(NS + 'name') == 'com.rekhcom.android.LauncherActivity')
    for intent in list(launcher.findall('intent-filter')):
        launcher.remove(intent)
    launcher.set(NS + 'exported', 'false')
    entry = ET.SubElement(application, 'activity', {NS + 'name': 'com.rekhcom.android.StartupDiagnosticActivity', NS + 'exported': 'true', NS + 'hardwareAccelerated': 'false', NS + 'configChanges': 'orientation|screenSize|screenLayout|smallestScreenSize|keyboardHidden|uiMode'})
    intent = ET.SubElement(entry, 'intent-filter')
    ET.SubElement(intent, 'action', {NS + 'name': 'android.intent.action.MAIN'})
    ET.SubElement(intent, 'category', {NS + 'name': 'android.intent.category.LAUNCHER'})
    ET.SubElement(application, 'provider', {NS + 'name': 'com.rekhcom.android.StartupReportProvider', NS + 'authorities': 'com.rekhcom.android.startup-report', NS + 'exported': 'false', NS + 'grantUriPermissions': 'true'})
    tree.write(manifest, encoding='utf-8', xml_declaration=True)
    for name in ('gradlew.bat', 'build.gradle', 'settings.gradle'):
        shutil.copy2(args.gradle_seed / name, stage / name)
    shutil.copytree(args.gradle_seed / 'gradle', stage / 'gradle')
    (stage / 'gradle.properties').write_text('org.gradle.jvmargs=-Xmx768m -Dfile.encoding=UTF-8\norg.gradle.workers.max=1\norg.gradle.daemon=false\norg.gradle.parallel=false\n', encoding='utf-8')
    (stage / 'local.properties').write_text('sdk.dir=' + sdk.as_posix() + '\n', encoding='utf-8')
    (stage / 'app/build.gradle').write_text("""plugins { id 'com.android.application' }
android {
 namespace 'com.rekhcom.android'
 compileSdk 35
 buildToolsVersion '35.0.0'
 defaultConfig { applicationId 'com.rekhcom.android'; minSdk 21; targetSdk 35; versionCode 102020099; versionName '2.2-test412-startup-diagnostic' }
 compileOptions { sourceCompatibility JavaVersion.VERSION_1_8; targetCompatibility JavaVersion.VERSION_1_8 }
 buildTypes { debug { debuggable true } }
 packaging { jniLibs { useLegacyPackaging true; keepDebugSymbols += ['**/*.so'] } }
}
""", encoding='utf-8')
    command = [str(stage / 'gradlew.bat'), '--offline', '--no-daemon', '--max-workers=1', 'assembleDebug']
    with (args.out / 'gradle.log').open('wb') as log:
        result = subprocess.run(command, cwd=stage, env=env, stdout=log, stderr=subprocess.STDOUT, creationflags=0x08000000 if os.name == 'nt' else 0)
    assert result.returncode == 0, 'Offline Java packaging failed; inspect gradle.log'
    apk = stage / 'app/build/outputs/apk/debug/app-debug.apk'
    with zipfile.ZipFile(apk) as archive:
        actual = {name: sha(archive.read(name)) for name in archive.namelist() if (name.startswith('assets/') or name.startswith('lib/')) and not name.endswith('/')}
        assert actual == immutable, 'Native libraries/assets changed during packaging'
    build_tools = sdk / 'build-tools/35.0.0'
    signature = subprocess.check_output([str(build_tools / ('apksigner.bat' if os.name == 'nt' else 'apksigner')), 'verify', '--verbose', '--print-certs', str(apk)], env=env, text=True)
    assert CERT in signature
    badging = subprocess.check_output([str(build_tools / ('aapt.exe' if os.name == 'nt' else 'aapt')), 'dump', 'badging', str(apk)], env=env, text=True)
    assert "name='com.rekhcom.android' versionCode='102020099' versionName='2.2-test412-startup-diagnostic'" in badging
    assert 'com.rekhcom.android.StartupDiagnosticActivity' in badging
    assert 'android.permission.' not in badging, 'Unexpected diagnostic permission'
    destination = args.out / 'ReKHCoM-v2.2-test412-startup-diagnostic.apk'
    shutil.copy2(apk, destination)
    (args.out / 'signature.txt').write_text(signature)
    (args.out / 'badging.txt').write_text(badging)
    staged_java = {file.name: sha((source / 'java/com/rekhcom/android' / file.name).read_bytes()) for file in diagnostic.glob('*.java') if not file.name.endswith('Test.java')}
    receipt = {'source': subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip(), 'source_dirty': bool(subprocess.check_output(['git', '-C', str(ROOT), 'status', '--porcelain'], text=True).strip()), 'diagnostic_staged_java_sha256': staged_java, 'base_source': PIN, 'base_apk_sha256': BASE_SHA, 'apk': str(destination), 'apk_sha256': sha(destination.read_bytes()), 'application_id': 'com.rekhcom.android', 'version_code': 102020099, 'version_name': '2.2-test412-startup-diagnostic', 'certificate_sha256': CERT, 'immutable_entries': immutable, 'native_library_count': 8, 'native_rebuild': False, 'offline': True, 'normal_release_modified': False}
    (args.out / 'BUILD-RECEIPT.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in receipt.items() if k != 'immutable_entries'}))


if __name__ == '__main__':
    main()
