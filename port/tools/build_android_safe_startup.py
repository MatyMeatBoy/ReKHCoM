"""Offline Java-first startup test over current native outputs, never public APK native bytes.
Same application ID/certificate/versionCode permits an update without clearing saves.
Build success is not install/runtime evidence. No SDK installation or emulator is run.
"""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,zipfile,xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[2]
NS='{http://schemas.android.com/apk/res/android}'
CERT='3a5a1740e8171c82b505907da26f3039e2153aeb8d16c2c22477d52edf74ecdc'
BASE_SHA='bd7999c234669ac88dfd5ec22ae3ccddd95db011eda0a1608fb8526819e19a4e'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise ValueError(message)
TEST_VERSIONS=('2.2-test374-safe-startup','2.2-test374-font-fallback')

def package_identity(version):
    require(version in TEST_VERSIONS,'Unsupported startup test identity')
    return 'ReKHCoM-v'+version+'.apk',version

def safe_manifest(text,template,*,application_id='com.rekhcom.android',label='ReKHCoM Safe Startup Test'):
    require('SDL_ENV.' not in text,'Autoplay fixture not allowed')
    ET.register_namespace('android',NS[1:-1]);root=ET.fromstring(text);app=root.find('application')
    if label is not None:app.set(NS+'label',label)
    official=ET.fromstring(template).find('application').find('activity').get(NS+'configChanges').split('|')
    activities={a.get(NS+'name'):a for a in app.findall('activity')}
    for name,role in [('LauncherActivity','launcher'),('GameActivity','game')]:
        activity=activities['com.rekhcom.android.'+name]
        activity.set(NS+'exported','false');activity.set(NS+'process',':'+role)
        current=activity.get(NS+'configChanges','').split('|')
        activity.set(NS+'configChanges','|'.join(dict.fromkeys(current+official)))
        for intent in list(activity.findall('intent-filter')):activity.remove(intent)
    entry=ET.SubElement(app,'activity',{NS+'name':'com.rekhcom.android.StartupDiagnosticActivity',NS+'exported':'true',NS+'hardwareAccelerated':'false',NS+'configChanges':'orientation|screenSize|screenLayout|smallestScreenSize|keyboardHidden|uiMode'})
    intent=ET.SubElement(entry,'intent-filter');ET.SubElement(intent,'action',{NS+'name':'android.intent.action.MAIN'});ET.SubElement(intent,'category',{NS+'name':'android.intent.category.LAUNCHER'})
    ET.SubElement(app,'provider',{NS+'name':'com.rekhcom.android.StartupReportProvider',NS+'authorities':application_id+'.startup-report',NS+'exported':'false',NS+'grantUriPermissions':'true'})
    return ET.tostring(root,encoding='unicode')
def validate_manifest(text,template,*,application_id='com.rekhcom.android'):
    root=ET.fromstring(text);app=root.find('application');activities={a.get(NS+'name'):a for a in app.findall('activity')}
    require('SDL_ENV.' not in text,'Unexpected autoplay fixture')
    main=[a for a in app.findall('activity') if any(c.get(NS+'name')=='android.intent.category.LAUNCHER' for c in a.findall('./intent-filter/category'))]
    require(len(main)==1 and main[0].get(NS+'name')=='com.rekhcom.android.StartupDiagnosticActivity','Java-only entry missing/ambiguous')
    require(main[0].get(NS+'hardwareAccelerated')=='false','Entry must not require a hardware surface')
    providers=app.findall('provider')
    require(len(providers)==1 and providers[0].get(NS+'name')=='com.rekhcom.android.StartupReportProvider','Missing/ambiguous report provider')
    require(providers[0].get(NS+'authorities')==application_id+'.startup-report','Wrong report provider identity')
    require(providers[0].get(NS+'exported')=='false' and providers[0].get(NS+'grantUriPermissions')=='true','Report provider must use private read-only grants')
    official=set(ET.fromstring(template).find('application').find('activity').get(NS+'configChanges').split('|'))
    for name,role in [('LauncherActivity','launcher'),('GameActivity','game')]:
        activity=activities['com.rekhcom.android.'+name]
        require(activity.get(NS+'process')==':'+role,'Wrong owned SDL process')
        require(activity.get(NS+'exported')=='false','SDL entry must remain private')
        require(official<=set(activity.get(NS+'configChanges','').split('|')),'Missing actual SDL config-change contracts')
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('base-apk','gradle-seed','toolchain','out'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--game-source',required=True)
    parser.add_argument('--version-name',choices=TEST_VERSIONS,default=TEST_VERSIONS[0])
    args=parser.parse_args();out=args.out.resolve();tools=args.toolchain.resolve();sdk=tools/'sdk'
    filename,version=package_identity(args.version_name)
    require(not out.exists(),'Use a new output folder; preserve prior evidence')
    require(sha(args.base_apk)==BASE_SHA,'Unexpected public asset baseline')
    dirty=subprocess.check_output(['git','-C',str(ROOT),'status','--porcelain'],text=True).strip()
    require(not dirty,'Commit/review sources before packaging')
    revision=subprocess.check_output(['git','-C',str(ROOT),'rev-parse','HEAD'],text=True).strip()
    native_diff=subprocess.check_output(['git','-C',str(ROOT),'diff','--name-only',args.game_source,revision,'--','src','include','port/src','port/include','port/ps5/guest','port/ps5/shared','port/switch','port/android64/host','port/android/native'],text=True).strip()
    require(not native_diff,'Game native inputs differ from the declared compiled source: '+native_diff)
    require((sdk/'platforms/android-35/android.jar').is_file(),'Cached SDK35 required; no installs')
    stage=out/'gradle';source=stage/'app/src/main';source.mkdir(parents=True)
    shutil.copytree(ROOT/'port/android/app/src/main/java',source/'java')
    shutil.copytree(ROOT/'port/android/app/src/main/res',source/'res')
    shutil.copytree(tools/'SDL3-3.4.18/android-project/app/src/main/java/org',source/'java/org')
    for file in (ROOT/'port/android/diagnostics').glob('*.java'):
        if not file.name.endswith('Test.java'):shutil.copy2(file,source/'java/com/rekhcom/android'/file.name)
    template=(tools/'SDL3-3.4.18/android-project/app/src/main/AndroidManifest.xml').read_text(encoding='utf-8')
    manifest=safe_manifest((ROOT/'port/android/app/src/main/AndroidManifest.xml').read_text(encoding='utf-8'),template)
    validate_manifest(manifest,template);(source/'AndroidManifest.xml').write_text(manifest,encoding='utf-8')
    assets={}
    with zipfile.ZipFile(args.base_apk) as archive:
        require(archive.testzip() is None,'Asset baseline ZIP corrupt')
        for name in archive.namelist():
            if not name.startswith('assets/') or name.endswith('/') or name.endswith('.macho'):continue
            require('..' not in Path(name).parts and Path(name).suffix.lower() not in ('.gba','.sav','.ini','.khst'),'Unexpected private input in assets')
            target=source/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(archive.read(name));assets[name]=sha(target)
    for region in ('eu','us','jp'):
        image=ROOT/f'build/android64/guest-{region}.macho';require(image.is_file(),'Current game image missing')
        shutil.copy2(image,source/'assets'/image.name);assets['assets/'+image.name]=sha(image)
    (source/'assets/startup-build.txt').write_text('Java/launcher source: '+revision+'\nGame native source: '+args.game_source+'\nTest374: Java-first entry; no physical-device cause certified.\n',encoding='utf-8')
    assets['assets/startup-build.txt']=sha(source/'assets/startup-build.txt')
    ndk=next((tools/'native').glob('android-ndk-*'));strip=ndk/'toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-strip.exe'
    libraries={};inputs={}
    for abi,folder,names in [('armeabi-v7a','android-native',['libSDL3.so','liblauncher.so','librekhcom_eu.so','librekhcom_us.so','librekhcom_jp.so']),('arm64-v8a','android-native-arm64-v8a',['libSDL3.so','liblauncher.so','librekhcom64.so'])]:
        for name in names:
            original=ROOT/'build'/folder/('SDL/'+name if name=='libSDL3.so' else name);require(original.is_file(),'Current native library missing: '+name)
            target=source/'jniLibs'/abi/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(original,target)
            inputs['lib/'+abi+'/'+name]=sha(original);subprocess.run([str(strip),'--strip-unneeded',str(target)],check=True)
            libraries['lib/'+abi+'/'+name]=sha(target)
    for name in ('gradlew.bat','build.gradle','settings.gradle'):shutil.copy2(args.gradle_seed/name,stage/name)
    shutil.copytree(args.gradle_seed/'gradle',stage/'gradle')
    (stage/'gradle.properties').write_text('org.gradle.jvmargs=-Xmx768m -Dfile.encoding=UTF-8\norg.gradle.workers.max=2\norg.gradle.daemon=false\norg.gradle.parallel=false\n')
    (stage/'local.properties').write_text('sdk.dir='+sdk.as_posix()+'\n')
    (stage/'app/build.gradle').write_text("""plugins { id 'com.android.application' }
android {
 namespace 'com.rekhcom.android'
 compileSdk 35
 buildToolsVersion '35.0.0'
 defaultConfig { applicationId 'com.rekhcom.android'; minSdk 21; targetSdk 35; versionCode 102020099; versionName '@@VERSION@@' }
 compileOptions { sourceCompatibility JavaVersion.VERSION_1_8; targetCompatibility JavaVersion.VERSION_1_8 }
 buildTypes { debug { debuggable true } }
 packaging { jniLibs { useLegacyPackaging true; keepDebugSymbols += ['**/*.so'] } }
}
""".replace('@@VERSION@@',version))
    temporary=out/'tmp';temporary.mkdir()
    # Invoke the existing distribution itself: --offline does not stop Gradle's
    # wrapper from downloading its bootstrap distribution when that is missing.
    cached=list((tools/'gradle-cache/wrapper/dists').glob('gradle-8.12-bin/*/gradle-8.12/bin/gradle.bat'))
    require(bool(cached),'Cached Gradle8.12 missing; no downloads allowed')
    gradle=sorted(cached)[0]
    (stage/'gradle.properties').write_text('org.gradle.jvmargs=-Xmx768m -Dfile.encoding=UTF-8 -Djava.io.tmpdir='+temporary.as_posix()+'\norg.gradle.workers.max=2\norg.gradle.daemon=false\norg.gradle.parallel=false\n')
    java=next((tools/'java').glob('jdk-17*'));env=dict(os.environ,JAVA_HOME=str(java),ANDROID_HOME=str(sdk),ANDROID_SDK_ROOT=str(sdk),GRADLE_USER_HOME=str(tools/'gradle-cache'),TEMP=str(temporary),TMP=str(temporary),TMPDIR=str(temporary));env['PATH']=str(java/'bin')+os.pathsep+env.get('PATH','')
    with (out/'gradle.log').open('wb') as log:
        result=subprocess.run([str(gradle),'--offline','--no-daemon','--max-workers=2','assembleDebug'],cwd=stage,env=env,stdout=log,stderr=subprocess.STDOUT)
    require(result.returncode==0,'Offline packaging failed; preserve gradle.log')
    apk=stage/'app/build/outputs/apk/debug/app-debug.apk'
    with zipfile.ZipFile(apk) as archive:
        require(archive.testzip() is None,'APK ZIP corrupt')
        actual={name:hashlib.sha256(archive.read(name)).hexdigest() for name in archive.namelist() if name in assets or name in libraries}
        require(actual==dict(assets,**libraries),'Native/assets bytes changed during packaging')
        require(not any(Path(name).suffix.lower() in ('.gba','.sav','.ini','.khst') for name in archive.namelist()),'Private input bundled')
    bt=sdk/'build-tools/35.0.0'
    signature=subprocess.check_output([str(bt/'apksigner.bat'),'verify','--verbose','--print-certs',str(apk)],env=env,text=True)
    badging=subprocess.check_output([str(bt/'aapt.exe'),'dump','badging',str(apk)],env=env,text=True)
    require(CERT in signature,'Unexpected signing certificate')
    require(("name='com.rekhcom.android' versionCode='102020099' versionName='"+version+"'") in badging,'Wrong install identity')
    require('com.rekhcom.android.StartupDiagnosticActivity' in badging,'Wrong entry')
    target=out/filename;shutil.copy2(apk,target)
    receipt=dict(source=revision,game_native_source=args.game_source,assets_baseline_apk_sha256=BASE_SHA,assets=assets,native_input_sha256=inputs,packaged_library_sha256=libraries,apk=str(target),sha256=sha(target),application_id='com.rekhcom.android',version_code=102020099,version_name=version,certificate_sha256=CERT,native_count=len(libraries),offline=True,runtime_tested=False,install_tested=False,limits='Java-first path is a tentative launcher bypass; no Moto/S22 causal fix or gameplay stability claim. Native current outputs must be tied to separate compile receipts; no mtime proof.')
    (out/'BUILD-RECEIPT.json').write_text(json.dumps(receipt,indent=2));(out/'signature.txt').write_text(signature);(out/'badging.txt').write_text(badging)
    print(json.dumps({key:receipt[key] for key in ('source','apk','sha256','native_count','install_tested','runtime_tested')}))
if __name__=='__main__':main()
