"""Pure-JVM production importer checks; optional --rom uses an owned clean dump.

No APK, Android process or copyrighted fixture is generated in the repository.
"""
from pathlib import Path
import argparse,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
TOOLS=next(p/'toolchain' for p in ROOT.parents if (p/'toolchain').is_dir())
JDK=sorted((TOOLS/'android/java').glob('jdk-*/bin'))[-1]
HARNESS=r'''
import com.rekhcom.android.RomImport;
import java.io.*;import java.nio.file.*;import java.security.*;import java.util.*;import java.util.zip.*;
public class RomImportTest {
 static int checks;
 static void check(boolean yes){if(!yes)throw new AssertionError();checks++;}
 static void put(File f,byte[]b)throws Exception{f.getParentFile().mkdirs();try(OutputStream out=new FileOutputStream(f)){out.write(b);}}
 static String sha(File f)throws Exception{MessageDigest d=MessageDigest.getInstance("SHA-1");try(InputStream in=new FileInputStream(f)){byte[]b=new byte[65536];int n;while((n=in.read(b))>=0)if(n>0)d.update(b,0,n);}return hex(d.digest());}
 static String hex(byte[]b){StringBuilder s=new StringBuilder();for(byte v:b)s.append(String.format(Locale.ROOT,"%02x",v&255));return s.toString();}
 static Map<String,String> snapshot(File root)throws Exception{Map<String,String> m=new TreeMap<>();try(java.util.stream.Stream<Path>s=Files.walk(root.toPath())){Iterator<Path>i=s.iterator();while(i.hasNext()){Path p=i.next();if(Files.isRegularFile(p))m.put(root.toPath().relativize(p).toString(),sha(p.toFile()));}}return m;}
 static File app(File root,String name)throws Exception{File f=new File(root,name);f.mkdirs();for(String p:new String[]{"roms/B8CP.gba","roms/B8CE.gba","roms/B8CJ.gba","ReKHCoM-HD.zip","hd/sprites/old.png","kh_com.sav","kh_com.ini"})put(new File(f,p),("previous:"+p).getBytes("UTF-8"));return f;}
 static final RomImport.Replacer MOVE=(from,to)->{try{Files.move(from.toPath(),to.toPath(),StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);}catch(IOException e){throw e;}};
 static byte[] zip(String name,byte[]body)throws Exception{ByteArrayOutputStream bytes=new ByteArrayOutputStream();try(ZipOutputStream z=new ZipOutputStream(bytes)){z.putNextEntry(new ZipEntry(name));z.write(body);z.closeEntry();}return bytes.toByteArray();}
 static void reject(File root,String name,InputStream in)throws Exception{
  File f=app(root,name);Map<String,String> old=snapshot(f);
  try{RomImport.install(in,f,MOVE);throw new AssertionError("accepted "+name);}catch(IOException expected){checks++;}
  check(snapshot(f).equals(old));
 }
 static InputStream zeros(final long length){return new InputStream(){long left=length;public int read(){if(left==0)return -1;left--;return 0;}public int read(byte[]b,int o,int n){if(left==0)return -1;n=(int)Math.min(n,left);Arrays.fill(b,o,o+n,(byte)0);left-=n;return n;}};}
 static byte[] badSizeZip()throws Exception{byte[]b=zip("game.gba",new byte[16]);for(int i=0;i+46<b.length;i++)if(b[i]==0x50&&b[i+1]==0x4b&&b[i+2]==1&&b[i+3]==2){long n=32L*1024*1024+1;for(int k=0;k<4;k++)b[i+24+k]=(byte)(n>>(8*k));break;}return b;}
 static byte[] declaredZip(String[]names,long[]sizes)throws Exception{
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();try(ZipOutputStream z=new ZipOutputStream(bytes)){for(String name:names){z.putNextEntry(new ZipEntry(name));z.write(new byte[16]);z.closeEntry();}}
  byte[]b=bytes.toByteArray();int entry=0;for(int i=0;i+46<b.length;i++)if(b[i]==0x50&&b[i+1]==0x4b&&b[i+2]==1&&b[i+3]==2){for(int k=0;k<4;k++)b[i+24+k]=(byte)(sizes[entry]>>(8*k));entry++;}
  check(entry==names.length);return b;
 }
 public static void main(String[]args)throws Exception{
  File root=new File(args[0]);root.mkdirs();
  reject(root,"raw-short",new ByteArrayInputStream(new byte[16]));
  reject(root,"null-stream",null);
  reject(root,"raw-unrecognized",zeros(32L*1024*1024));
  reject(root,"raw-large",zeros(32L*1024*1024+1));
  reject(root,"failed-stream",new InputStream(){int n;public int read()throws IOException{if(n++==32)throw new IOException("fixture read failure");return 0;}});
  reject(root,"failed-close",new ByteArrayInputStream(new byte[16]){public void close()throws IOException{throw new IOException("fixture close failure");}});
  reject(root,"zip-truncated",new ByteArrayInputStream(new byte[]{80,75,3,4}));
  reject(root,"zip-small-rom",new ByteArrayInputStream(zip("game.gba",new byte[16])));
  reject(root,"zip-oversize-rom",new ByteArrayInputStream(badSizeZip()));
  reject(root,"zip-unknown",new ByteArrayInputStream(zip("readme.txt",new byte[16])));
  reject(root,"zip-uncompressed-limit",new ByteArrayInputStream(declaredZip(new String[]{"hd/music/a.ogg","hd/music/b.ogg","hd/music/c.ogg"},new long[]{0xfffffffeL,0xfffffffeL,0xfffffffeL})));
  reject(root,"zip-rom-work-limit",new ByteArrayInputStream(declaredZip(new String[]{"game.gba","readme.txt"},new long[]{32L<<20,33L<<20})));
  reject(root,"zip-ambiguous",new ByteArrayInputStream(declaredZip(new String[]{"one.gba","two.gba"},new long[]{32L<<20,32L<<20})));
  reject(root,"zip-mixed-rom-hd",new ByteArrayInputStream(declaredZip(new String[]{"game.gba","hd/music/Title.ogg"},new long[]{32L<<20,16})));
  for(String path:new String[]{"../game.gba","/game.gba","C:/game.gba","sub\\game.gba"})reject(root,"unsafe-"+checks,new ByteArrayInputStream(zip(path,new byte[16])));
  ByteArrayOutputStream many=new ByteArrayOutputStream();try(ZipOutputStream z=new ZipOutputStream(many)){for(int i=0;i<16385;i++){z.putNextEntry(new ZipEntry("d"+i+"/"));z.closeEntry();}}
  reject(root,"zip-entry-limit",new ByteArrayInputStream(many.toByteArray()));
  byte[] pack=zip("hd/music/Title.ogg",new byte[]{'O','g','g','S'});
  reject(root,"pack-close-failure",new ByteArrayInputStream(pack){public void close()throws IOException{throw new IOException("fixture close failure");}});
  File hd=app(root,"hd-pack");Map<String,String> before=snapshot(hd);
  boolean[]closed={false};RomImport.Result result=RomImport.install(new ByteArrayInputStream(pack){int reads;public synchronized int read(byte[]b,int o,int n){return super.read(b,o,reads++<4?Math.min(1,n):n);}public void close(){closed[0]=true;}},hd,(a,b)->{check(closed[0]);MOVE.replace(a,b);});
  check(result.kind==RomImport.Kind.HD_PACK&&Arrays.equals(Files.readAllBytes(new File(hd,"ReKHCoM-HD.zip").toPath()),pack));
  before.remove("ReKHCoM-HD.zip");Map<String,String> after=snapshot(hd);after.remove("ReKHCoM-HD.zip");check(before.equals(after));
  File fail=app(root,"failed-commit");before=snapshot(fail);
  try{RomImport.install(new ByteArrayInputStream(pack),fail,(a,b)->{throw new IOException("fixture rename failure");});throw new AssertionError();}catch(IOException expected){checks++;}
  check(before.equals(snapshot(fail)));
  if(args.length>1){
   File own=new File(args[1]);String expected=sha(own);File archive=new File(root,"owned-rom.zip");
   try(ZipOutputStream z=new ZipOutputStream(new FileOutputStream(archive));InputStream in=new FileInputStream(own)){z.putNextEntry(new ZipEntry("dump/game.gba"));byte[]b=new byte[65536];int n;while((n=in.read(b))!=-1)z.write(b,0,n);z.closeEntry();}
   byte[] badCrc=Files.readAllBytes(archive.toPath());
   for(int i=0;i+46<badCrc.length;i++)if(badCrc[i]==0x50&&badCrc[i+1]==0x4b&&badCrc[i+2]==1&&badCrc[i+3]==2){badCrc[i+16]^=1;break;}
   reject(root,"zip-wrong-crc",new ByteArrayInputStream(badCrc));
   for(int form=0;form<2;form++){
    File f=app(root,"valid-"+form);byte[] oldPack=Files.readAllBytes(new File(f,"ReKHCoM-HD.zip").toPath());
    Map<String,String> previous=snapshot(f);
    result=RomImport.install(new FileInputStream(form==0?own:archive),f,MOVE);
    check(result.kind==RomImport.Kind.ROM&&sha(new File(f,"roms/"+result.code+".gba")).equals(expected));
    check(Arrays.equals(oldPack,Files.readAllBytes(new File(f,"ReKHCoM-HD.zip").toPath())));
    check(Arrays.equals(Files.readAllBytes(new File(f,"kh_com.sav").toPath()),"previous:kh_com.sav".getBytes("UTF-8")));
    Map<String,String> current=snapshot(f);String replaced="roms"+File.separator+result.code+".gba";previous.remove(replaced);current.remove(replaced);check(previous.equals(current));
    System.out.println("OWNED_ROM_RESULT "+result.code+" "+new File(f,"roms/"+result.code+".gba").getAbsolutePath());
   }
  }
  System.out.println("PASS production Android ROM importer: "+checks+" checks; optional owned ROM "+(args.length>1?"verified":"not supplied"));
 }
}
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rom',type=Path);p.add_argument('--work',type=Path);a=p.parse_args()
 with tempfile.TemporaryDirectory(prefix='android-rom-jvm-') as tmp:
  out=Path(tmp);source=out/'RomImportTest.java';source.write_text(HARNESS,encoding='utf-8')
  subprocess.run([str(JDK/'javac.exe'),'-encoding','UTF-8','-d',str(out),str(ROOT/'port/android/app/src/main/java/com/rekhcom/android/RomImport.java'),str(source)],check=True)
  if a.work:a.work.mkdir(parents=True,exist_ok=False)
  subprocess.run([str(JDK/'java.exe'),'-Xmx192m','-cp',str(out),'RomImportTest',str(a.work.resolve() if a.work else out/'files'),*([str(a.rom.resolve())] if a.rom else [])],check=True,timeout=60)
if __name__=='__main__':main()
