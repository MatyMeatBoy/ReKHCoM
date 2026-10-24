"""Build the experimental ARMv7 cores and ROM-free Android launcher APK.

Requires the official SDK/NDK/JDK/SDL sources installed under ../toolchain/android.
No emulator/device testing or release upload is implied by a successful build.
"""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import struct
import urllib.request

ROOT=Path(__file__).resolve().parents[2]


def verify_fixed_symbols(path):
    """Absolute cartridge pointers must not receive the ELF load bias.

    Android API24's loader rebases SHN_ABS dynamic relocations. Local binding
    resolves these at link time; reject a library that reintroduces them.
    """
    data=path.read_bytes()
    if data[:6]!=b'\x7fELF\x01\x01':
        raise ValueError(f'Expected little-endian ELF32: {path}')
    start=struct.unpack_from('<I',data,32)[0]
    stride,count=struct.unpack_from('<HH',data,46)
    sections=[struct.unpack_from('<10I',data,start+i*stride) for i in range(count)]
    bad=[]
    for section in sections:
        if section[1] not in (4,9):continue # SHT_RELA / SHT_REL
        symbols=sections[section[6]]
        strings=sections[symbols[6]]
        for at in range(section[4],section[4]+section[5],section[9]):
            info=struct.unpack_from('<I',data,at+4)[0]
            index=info>>8
            if not index:continue
            symbol=symbols[4]+index*symbols[9]
            name=struct.unpack_from('<I',data,symbol)[0]
            shndx=struct.unpack_from('<H',data,symbol+14)[0]
            if shndx==0xfff1:
                begin=strings[4]+name;end=data.index(b'\0',begin)
                bad.append(data[begin:end].decode())
    if bad:raise ValueError('Rebased GBA absolute symbols: '+', '.join(sorted(set(bad))[:12]))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--regions',nargs='+',choices=['eu','us','jp'],default=['eu','us','jp'])
    p.add_argument('--jobs',type=int,default=3)
    p.add_argument('--abi',choices=['armeabi-v7a','x86'],default='armeabi-v7a')
    p.add_argument('--no-arm64',action='store_true',help='Leave out the 64-bit build (arm64-v8a host + game images, ANDROID_64.md)')
    p.add_argument('--native-only',action='store_true')
    p.add_argument('--ubsan',action='store_true',help='Test build: log shifts by 32+, divisions by zero, NULL and misaligned accesses to logcat (tag ReKHCoM-UBSAN)')
    p.add_argument('--smoke',action='store_true',help='Separate test app: starts a debug battle, captures frame 600 and exits at 900')
    p.add_argument('--smoke-env',default='',help='Smoke app only: KEY=VALUE;KEY=VALUE harness settings replacing the debug battle')
    args=p.parse_args()
    # seconds instead of a failed guest or Gradle build minutes later (check_sources.py)
    subprocess.run([sys.executable,str(ROOT/'port/tools/check_sources.py')],check=True)
    release=re.search(r'#define REKHCOM_VERSION\s+"([^"]+)"',
                      (ROOT/'port/include/port/port_version.h').read_text(encoding='utf-8'))[1]
    version_name=release.removeprefix('v')+'-android-beta'
    parts=re.match(r'v(\d+)\.(\d+)(?:\.(\d+))?',release)  # v2.0 = v2.0.0
    major,minor,patch=(int(g or 0) for g in parts.groups())
    nightly=re.search(r'nightly\.(\d{8})\.(\d+)$',release)
    version_code=major*1000000+minor*10000+patch*100+(int(nightly[2]) if nightly else 99)
    if args.jobs<1:p.error('--jobs must be positive')
    tools=ROOT.parent/'toolchain/android';sdk=tools/'sdk'
    ndk=next((tools/'native').glob('android-ndk-*'))
    ndk_version=next(line.split('=',1)[1].strip() for line in (ndk/'source.properties').read_text().splitlines() if line.startswith('Pkg.Revision'))
    java=next((tools/'java').glob('jdk-*'))
    sdl=tools/'SDL3-3.4.18';skeleton=sdl/'android-project'
    cmake=sdk/'cmake/3.22.1/bin/cmake.exe'
    ninja=ROOT.parent/'toolchain/bin/ninja.exe'
    native_suffix='' if args.abi=='armeabi-v7a' else '-'+args.abi
    suffix=native_suffix+('-smoke' if args.smoke else '')
    native=ROOT/('build/android-native'+native_suffix)
    env=dict(os.environ,JAVA_HOME=str(java),ANDROID_HOME=str(sdk),ANDROID_SDK_ROOT=str(sdk),
             GRADLE_USER_HOME=str(tools/'gradle-cache'),CMAKE_BUILD_PARALLEL_LEVEL=str(args.jobs))
    def run(cmd,cwd=ROOT):subprocess.run([str(x) for x in cmd],cwd=cwd,env=env,check=True)
    for region in args.regions:
        if not (ROOT/f'build/{region}/pc/rom_symbols.s').is_file():
            run([sys.executable,'port/tools/gen_pc_assets.py','--version',region])
    run([cmake,'-S',ROOT/'port/android','-B',native,'-G','Ninja',f'-DCMAKE_MAKE_PROGRAM={ninja}',
         f'-DCMAKE_TOOLCHAIN_FILE={ndk}/build/cmake/android.toolchain.cmake','-DANDROID_ABI='+args.abi,
         '-DANDROID_PLATFORM=android-21',f'-DSDL_SOURCE_DIR={sdl}','-DCMAKE_BUILD_TYPE=Release',
         f'-DFREETYPE_SOURCE_DIR={ROOT.parent / "toolchain/freetype-2.14.3"}',
         '-DREGIONS='+';'.join(args.regions)]+(['-DREKHCOM_UBSAN=ON'] if args.ubsan else []))
    run([ninja,'-C',native,'-j',args.jobs])
    for region in args.regions:
        verify_fixed_symbols(native/f'librekhcom_{region}.so')
    # the 64-bit build (port/progress/ANDROID_64.md): its host library and the game images (AArch64, 32-bit pointers)
    native64=ROOT/'build/android-native-arm64-v8a'
    if not args.no_arm64:
        run([cmake,'-S',ROOT/'port/android','-B',native64,'-G','Ninja',f'-DCMAKE_MAKE_PROGRAM={ninja}',
             f'-DCMAKE_TOOLCHAIN_FILE={ndk}/build/cmake/android.toolchain.cmake','-DANDROID_ABI=arm64-v8a',
             '-DANDROID_PLATFORM=android-21',f'-DSDL_SOURCE_DIR={sdl}','-DCMAKE_BUILD_TYPE=Release',
             f'-DFREETYPE_SOURCE_DIR={ROOT.parent / "toolchain/freetype-2.14.3"}'])
        run([ninja,'-C',native64,'-j',args.jobs])
        import sys as _sys
        run([_sys.executable,ROOT/'port/android64/build_android64.py','--regions',*args.regions,'--jobs',str(args.jobs)])
    if args.native_only:return
    stage=ROOT/('build/android-app'+suffix);stage.mkdir(exist_ok=True)
    shutil.copytree(ROOT/'port/android/app',stage/'app',dirs_exist_ok=True)
    manifest=stage/'app/src/main/AndroidManifest.xml'
    if args.smoke:
        text=manifest.read_text(encoding='utf-8')
        settings={'KHCOM_BATTLE':'10:4:0','KHCOM_KEYS':'60-63:A','KHCOM_OUTPUT_SHOT':'600',
                  'KHCOM_QUIT':'900','KHCOM_TRACE_MODES':'1'}
        if args.smoke_env:
            settings=dict(item.split('=',1) for item in args.smoke_env.split(';') if item)
        metadata=''.join(f'<meta-data android:name="SDL_ENV.{k}" android:value="{v}"/>\n' for k,v in settings.items())
        # the test app's game activity is exported: Android 14+ no longer lets "run-as ... am start" reach it
        text=text.replace('android:name="com.rekhcom.android.GameActivity" android:exported="false"',
                          'android:name="com.rekhcom.android.GameActivity" android:exported="true"')
        manifest.write_text(text.replace('  <activity',metadata+'  <activity',1),encoding='utf-8')
    else:
        assert 'SDL_ENV.' not in manifest.read_text(encoding='utf-8')

    shutil.copytree(skeleton/'app/src/main/java/org',stage/'app/src/main/java/org',dirs_exist_ok=True)
    shutil.copytree(skeleton/'gradle',stage/'gradle',dirs_exist_ok=True)
    shutil.copy2(skeleton/'gradlew.bat',stage/'gradlew.bat')
    # Pin and verify the distribution Gradle's official wrapper downloads.
    sha=urllib.request.urlopen('https://services.gradle.org/distributions/gradle-8.12-bin.zip.sha256',timeout=60).read().decode().strip()
    (stage/'gradle/wrapper/gradle-wrapper.properties').write_text(
        'distributionBase=GRADLE_USER_HOME\ndistributionPath=wrapper/dists\n'
        'distributionUrl=https\\://services.gradle.org/distributions/gradle-8.12-bin.zip\n'
        f'distributionSha256Sum={sha}\nzipStoreBase=GRADLE_USER_HOME\nzipStorePath=wrapper/dists\n')
    (stage/'settings.gradle').write_text("pluginManagement { repositories { google(); mavenCentral(); gradlePluginPortal() } }\ndependencyResolutionManagement { repositories { google(); mavenCentral() } }\nrootProject.name='ReKHCoM'\ninclude ':app'\n")
    (stage/'build.gradle').write_text("plugins { id 'com.android.application' version '8.7.3' apply false }\n")
    (stage/'gradle.properties').write_text('org.gradle.jvmargs=-Xmx1536m -Dfile.encoding=UTF-8\norg.gradle.workers.max=2\norg.gradle.daemon=false\n')
    (stage/'local.properties').write_text('sdk.dir='+str(sdk).replace('\\','/')+'\nndk.dir='+str(ndk).replace('\\','/')+'\n')
    (stage/'app/build.gradle').write_text("""plugins { id 'com.android.application' }
android {
 namespace 'com.rekhcom.android'
 compileSdk 35
 ndkVersion '__NDK_VERSION__'
 defaultConfig { applicationId 'com.rekhcom.android'; minSdk 21; targetSdk 35; versionCode __VERSION_CODE__; versionName '__VERSION_NAME__' }
 compileOptions { sourceCompatibility JavaVersion.VERSION_1_8; targetCompatibility JavaVersion.VERSION_1_8 }
 buildTypes { debug { debuggable true }; release { minifyEnabled false } }
 lint { abortOnError true }
 packaging { jniLibs { useLegacyPackaging true } }
}
""".replace('__NDK_VERSION__',ndk_version).replace('__VERSION_CODE__',str(version_code)).replace('__VERSION_NAME__',version_name))
    if args.smoke:
        gradle=stage/'app/build.gradle'
        gradle.write_text(gradle.read_text().replace("applicationId 'com.rekhcom.android'","applicationId 'com.rekhcom.android.smoke'"))
    libs=stage/'app/src/main/jniLibs'/args.abi;libs.mkdir(parents=True,exist_ok=True)
    desired={'libSDL3.so',*(f'librekhcom_{r}.so' for r in args.regions)}
    # Remove only stale native binaries from this generated staging folder.
    for old in libs.glob('*.so'):
        assert old.resolve().is_relative_to(stage.resolve())
        if old.name not in desired:old.unlink()
    strip=ndk/'toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-strip.exe'
    for name in desired:
        source=native/('SDL/'+name if name=='libSDL3.so' else name)
        shutil.copy2(source,libs/name);run([strip,'--strip-unneeded',libs/name])
    libs64=stage/'app/src/main/jniLibs/arm64-v8a'
    images=stage/'app/src/main/assets'
    images.mkdir(parents=True,exist_ok=True)
    for old in list(images.glob('guest-*.macho'))+(list(libs64.glob('*.so')) if libs64.exists() else []):
        assert old.resolve().is_relative_to(stage.resolve())
        old.unlink()
    with64=not args.no_arm64
    if with64:
        libs64.mkdir(parents=True,exist_ok=True)
        for name,source in (('libSDL3.so',native64/'SDL/libSDL3.so'),('librekhcom64.so',native64/'librekhcom64.so')):
            shutil.copy2(source,libs64/name);run([strip,'--strip-unneeded',libs64/name])
        for region in args.regions:
            shutil.copy2(ROOT/f'build/android64/guest-{region}.macho',images/f'guest-{region}.macho')
    notices=stage/'app/src/main/assets/licenses';notices.mkdir(parents=True,exist_ok=True)
    shutil.copytree(ROOT/'port/assets/fonts',stage/'app/src/main/assets/fonts',dirs_exist_ok=True)
    # achievement badges: the game reads APK assets without the "assets/" prefix (PortLoadAsset)
    (stage/'app/src/main/assets/achievements').mkdir(parents=True,exist_ok=True)
    shutil.copy2(ROOT/'port/assets/achievements/badges.png',stage/'app/src/main/assets/achievements/badges.png')
    for name in ['FTL.TXT','GPLv2.TXT']:
        shutil.copy2(ROOT.parent/'toolchain/freetype-2.14.3/docs'/name,notices/('FreeType-'+name))
    for source,name in [(sdl/'LICENSE.txt','SDL3.txt'),(ROOT/'LICENSE.md','ReKHCoM.md'),(ROOT/'port/android/assets/NOTICE.md','touch.md')]:shutil.copy2(source,notices/name)
    run([stage/'gradlew.bat','--no-daemon','--max-workers=2','clean','assembleDebug'],cwd=stage)
    apk=stage/'app/build/outputs/apk/debug/app-debug.apk'
    import zipfile
    with zipfile.ZipFile(apk) as z:
        assert z.testzip() is None
        assert not any(Path(n).suffix.lower() in ['.gba','.sav','.khst','.ini'] for n in z.namelist())
        if with64:
            assert 'lib/arm64-v8a/librekhcom64.so' in z.namelist(), 'the 64-bit host is missing'
            assert all(f'assets/guest-{r}.macho' in z.namelist() for r in args.regions), 'a 64-bit game image is missing'
        for name in desired:
            member=f'lib/{args.abi}/{name}'
            assert member in z.namelist()
            assert hashlib.sha256(z.read(member)).digest()==hashlib.sha256((libs/name).read_bytes()).digest(), ('stale APK library',name)
    out=ROOT/'build/releases';out.mkdir(exist_ok=True)
    target=out/f"ReKHCoM-{release}-android-alpha{'-smoke' if args.smoke else ''}.apk"
    shutil.copy2(apk,target)
    digest=hashlib.sha256(target.read_bytes()).hexdigest()
    target.with_suffix('.apk.sha256').write_text(f'{digest}  {target.name}\n')
    (out/('android-build-manifest'+suffix+'.json')).write_text(json.dumps({'apk':target.name,'sha256':digest,'regions':args.regions,
        'abi':args.abi,'version':version_name,'version_code':version_code,'debug_signed':True,'test_only':args.smoke,'runtime_tested':False},indent=2))
    print('Built (runtime verification still required):',target,digest)


if __name__=='__main__':main()
