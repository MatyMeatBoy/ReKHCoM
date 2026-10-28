package com.rekhcom.android;

import android.content.Intent;
import android.net.Uri;
import java.io.*;
import java.util.Locale;
import org.libsdl.app.SDLActivity;

/** The app's first screen: the PC launcher itself (port/decompui, built for Android as liblauncher.so: LAUNCHER_ANDROID)
 *  on SDL. What it cannot do from native code goes through here (decompui/launcher_android.inc): the system's file
 *  pickers (a dump, a save in or out, the PC's hd folder), copied in the background with a status line it reads, and
 *  starting the game in its own process (GameActivity, :game). SAF copies are bounded, hashed (dumps) and atomic. */
public class LauncherActivity extends SDLActivity {
    private LifecycleTrace lifecycle;
    private void lifecycle(String event){if(lifecycle!=null)lifecycle.record(this,event,"launcher");}
    @Override protected void onCreate(android.os.Bundle state){
        try{lifecycle=LifecycleTrace.attach(this,"launcher",state!=null);}catch(Throwable ignored){}
        lifecycle("onCreate");super.onCreate(state);
    }
    @Override protected void onPause(){lifecycle("onPause");super.onPause();}
    @Override protected void onStop(){lifecycle("onStop");super.onStop();}
    @Override protected void onDestroy(){lifecycle("onDestroy");super.onDestroy();}
    private static final int PICK_ROM=10,PICK_SAVE=20,PICK_EXPORT=30,PICK_HD=40;
    @Override protected String[] getLibraries(){return new String[]{"SDL3","launcher"};}
    @Override protected String[] getArguments(){return new String[0];}

    /* the launcher's status line: busy|serial|error|text (nativeStatus); serial grows with every result */
    private static volatile boolean busy,failed;
    private static volatile int serial;
    private static volatile String message="";
    private int pickRegion,pickBank=1;
    private boolean english(){return !"es".equals(Locale.getDefault().getLanguage());}
    private String t(String es,String en){return english()?en:es;}
    private void progress(String text){message=text;}

    /** Called by the launcher (any thread): the picker for a dump, a save into region/bank, a save out of it, the hd folder. */
    public void nativePick(final int what,final int region,final int bank){
        runOnUiThread(()->{
            if(busy)return;
            pickRegion=Math.max(0,Math.min(2,region));pickBank=Math.max(1,Math.min(5,bank));
            Intent i;
            if(what==PICK_HD){i=new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);}
            else if(what==PICK_EXPORT){i=new Intent(Intent.ACTION_CREATE_DOCUMENT);i.setType("application/octet-stream");i.addCategory(Intent.CATEGORY_OPENABLE);i.putExtra(Intent.EXTRA_TITLE,SaveImport.bankName(pickRegion,pickBank));}
            else{i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.setType("*/*");i.addCategory(Intent.CATEGORY_OPENABLE);i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);}
            try{startActivityForResult(i,what);}catch(Exception e){finish(t("No hay un selector de archivos: ","No file picker: ")+e.getMessage(),true);}
        });
    }
    /** Called by the launcher: the game for region (eu/us/jp) with the dump at rom; kh_com.ini and netlink.cfg are written. */
    public void nativeLaunch(final String region,final String rom){
        runOnUiThread(()->{Intent i=new Intent(this,GameActivity.class);i.putExtra("region",region);i.putExtra("rom",rom);startActivity(i);});
    }
    /** Read by the launcher every frame. */
    public String nativeStatus(){return (busy?1:0)+"|"+serial+"|"+(failed?1:0)+"|"+message;}
    private void finish(String text,boolean error){message=text;failed=error;busy=false;serial++;}

    @Override protected void onActivityResult(int request,int result,Intent data){
        if(request!=PICK_ROM&&request!=PICK_SAVE&&request!=PICK_EXPORT&&request!=PICK_HD){super.onActivityResult(request,result,data);return;}
        if(result!=RESULT_OK||data==null||data.getData()==null)return;
        final Uri uri=data.getData();
        final int region=pickRegion,bank=pickBank;
        busy=true;message=t("Copiando...","Copying...");
        new Thread(()->{
            try {
                String msg;
                if(request==PICK_ROM)msg=importRom(uri);
                else if(request==PICK_SAVE){importSave(uri,region,bank);msg=t("Partida importada. Copia previa conservada si existía.","Save imported. The previous copy was kept if there was one.");}
                else if(request==PICK_EXPORT){exportSave(uri,region,bank);msg=t("Partida exportada.","Save exported.");}
                else msg=importHd(uri);
                finish(msg,false);
            }catch(Exception e){finish(t("No se pudo completar: ","Could not complete: ")+e.getMessage(),true);}
        },"launcher-copy").start();
    }
    private static void copy(InputStream in,OutputStream out,long limit)throws Exception{
        byte[] buf=new byte[65536];long count=0;int n;
        while((n=in.read(buf))!=-1){if(count+n>limit)throw new IOException("Archivo demasiado grande");out.write(buf,0,n);count+=n;}
    }
    private String importRom(Uri uri)throws Exception{
        /* ROM ZIPs and HD packs share PK magic; inspect the bounded inventory
           before choosing a destination. Os.rename replaces atomically on the
           app's filesystem without first deleting a previous ROM or HD pack. */
        RomImport.Result result=RomImport.install(getContentResolver().openInputStream(uri),getFilesDir(),(pending,target)->{
            try{android.system.Os.rename(pending.getAbsolutePath(),target.getAbsolutePath());}
            catch(android.system.ErrnoException e){throw new IOException("Could not install the selected file",e);}
        });
        return result.kind==RomImport.Kind.HD_PACK?
            t("Mod HD copiado: se instala al abrir el juego.","HD mod copied: it installs when the game starts."):
            t("ROM instalada y verificada: ","ROM installed and verified: ")+result.code;
    }
    private void importSave(Uri uri,int selectedSaveRegion,int selectedBank)throws Exception{
        File temp=File.createTempFile("save-",".tmp",getFilesDir());
        try {
            try(InputStream in=getContentResolver().openInputStream(uri);FileOutputStream out=new FileOutputStream(temp)){if(in==null)throw new IOException("Archivo no accesible");
                ByteArrayOutputStream bytes=new ByteArrayOutputStream();copy(in,bytes,131072);
                out.write(SaveImport.normalize(bytes.toByteArray(),selectedSaveRegion));out.getFD().sync();}
            if(temp.length()!=65536)throw new IOException("Partida SRAM de 64 KiB requerida");
            File target=new File(getFilesDir(),SaveImport.bankName(selectedSaveRegion,selectedBank));
            if(target.exists())try(FileInputStream in=new FileInputStream(target);FileOutputStream out=new FileOutputStream(new File(getFilesDir(),target.getName()+".backup-"+System.currentTimeMillis()))){copy(in,out,65536);out.getFD().sync();}
            if(!temp.renameTo(target))throw new IOException("No se pudo guardar la partida");
        }finally{if(temp.exists())temp.delete();}
    }
    /* HD mod (port/progress/HD_MOD.md): the PC launcher's hd folder → files/hd: music (.ogg + .loop), movies and scenes (.mp4)
       their subtitles (.txt) and the HD cards/title/HUD pictures (sprites/*.png), whichever are there. Each file lands whole (temp + rename); one already there with
       the same size is kept (a second import only adds what is new). */
    private static final String[][] HD_PARTS={{"music","[A-Za-z0-9_]+[.](ogg|loop)"},{"movies","[A-Za-z0-9_]+[.]mp4"},{"scenes","[A-Za-z0-9_]+[.]mp4"},{"subs","[A-Za-z0-9_]+[.][a-z]{2}[.]txt"},{"sprites","[A-Za-z0-9_]+[.]png"}};
    private static native boolean commitHdMedia(String root,int mode,String pending);
    private static String hdFile(android.content.ContentResolver resolver,Uri tree,String parent,String wanted)throws IOException{
        try(android.database.Cursor c=resolver.query(android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(tree,parent),
                new String[]{android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID,android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME},null,null,null)){
            while(c!=null&&c.moveToNext())if(wanted.equals(c.getString(1)))return c.getString(0);
        }
        return null;
    }
    private static void hdGeneration(android.content.ContentResolver resolver,Uri tree,String directory,String relative,
                                     java.util.List<String[]> files,int depth,int[] entries)throws IOException{
        if(depth>2)throw new IOException("HD media depth");
        try(android.database.Cursor c=resolver.query(android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(tree,directory),
                new String[]{android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID,android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                             android.provider.DocumentsContract.Document.COLUMN_SIZE,android.provider.DocumentsContract.Document.COLUMN_MIME_TYPE},null,null,null)){
            while(c!=null&&c.moveToNext()){
                if(++entries[0]>4096)throw new IOException("HD media inventory limit");
                String name=c.getString(1);if(name==null||name.contains(".."))continue;
                if(android.provider.DocumentsContract.Document.MIME_TYPE_DIR.equals(c.getString(3))){
                    if(depth==0&&(name.equals("movies")||name.equals("scenes")||name.equals("subs")))hdGeneration(resolver,tree,c.getString(0),relative+"/"+name,files,depth+1,entries);
                }else{
                    String member=relative+"/"+name;
                    if(member.matches("media/(en|ja)/g-[a-z0-9-]+/(media[.]meta|(movies|scenes)/[A-Za-z0-9_.-]+[.](mp4|png)|subs/[A-Za-z0-9_]+[.][a-z]{2}[.]txt)"))
                        files.add(new String[]{c.getString(0),relative,name,Long.toString(c.getLong(2))});
                }
            }
        }
    }
    private static void hdVariants(android.content.ContentResolver resolver,Uri tree,String root,java.util.List<String[]> files)throws Exception{
        String media=child(resolver,tree,root,"media");if(media==null)return;
        int[] entries={0};
        for(String label:new String[]{"en","ja"}){
            String directory=child(resolver,tree,media,label);if(directory==null)continue;
            String active=hdFile(resolver,tree,directory,"active.txt");if(active==null)continue;
            java.io.ByteArrayOutputStream bytes=new java.io.ByteArrayOutputStream();
            try(InputStream in=resolver.openInputStream(android.provider.DocumentsContract.buildDocumentUriUsingTree(tree,active))){if(in==null)throw new IOException("HD active");copy(in,bytes,1024);}
            String text=new String(bytes.toByteArray(),java.nio.charset.StandardCharsets.UTF_8).replace("\r\n","\n");
            java.util.regex.Matcher match=java.util.regex.Pattern.compile("\\AREKHCOM_MEDIA_ACTIVE_V1\\nmovies (g-[a-z0-9-]+|-)\\nscenes (g-[a-z0-9-]+|-)\\n?\\z").matcher(text);
            if(!match.matches())throw new IOException("HD media active index");
            java.util.Set<String> generations=new java.util.HashSet<>();generations.add(match.group(1));generations.add(match.group(2));
            for(String generation:generations){
                if(generation.equals("-"))continue;
                String folder=child(resolver,tree,directory,generation);if(folder==null)throw new IOException("HD media generation missing");
                hdGeneration(resolver,tree,folder,"media/"+label+"/"+generation,files,0,entries);
            }
            files.add(new String[]{active,"media/"+label,"active.txt",Long.toString(bytes.size())});
        }
    }
    private String importHd(Uri tree)throws Exception{
        android.content.ContentResolver resolver=getContentResolver();
        String root=android.provider.DocumentsContract.getTreeDocumentId(tree);
        if(child(resolver,tree,root,"music")==null&&child(resolver,tree,root,"movies")==null&&child(resolver,tree,root,"scenes")==null&&child(resolver,tree,root,"sprites")==null&&child(resolver,tree,root,"media")==null){String hd=child(resolver,tree,root,"hd");if(hd!=null)root=hd;}
        java.util.List<String[]> files=new java.util.ArrayList<>(); // document id, folder, name, size
        long total=0,copied=0;
        for(String[] part:HD_PARTS){
            String folder=child(resolver,tree,root,part[0]);if(folder==null)continue;
            try(android.database.Cursor c=resolver.query(android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(tree,folder),
                    new String[]{android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID,android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME,android.provider.DocumentsContract.Document.COLUMN_SIZE},null,null,null)){
                while(c!=null&&c.moveToNext()){String name=c.getString(1);if(name!=null&&name.matches(part[1])){files.add(new String[]{c.getString(0),part[0],name,Long.toString(c.getLong(2))});total+=c.getLong(2);}}
            }
        }
        hdVariants(resolver,tree,root,files);
        files.sort((a,b)->Boolean.compare(a[2].equals("active.txt"),b[2].equals("active.txt")));
        total=0;for(String[] f:files)total+=Long.parseLong(f[3]);
        if(files.isEmpty())throw new IOException(t("no hay una carpeta hd con music, movies, scenes, subs o sprites dentro","there is no hd folder with music, movies, scenes, subs or sprites inside"));
        int tracks=0,movies=0;
        for(String[] f:files){
            File dir=new File(getFilesDir(),"hd/"+f[1]);if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("hd/"+f[1]);
            File target=new File(dir,f[2]);long size=Long.parseLong(f[3]);
            boolean variant=f[1].startsWith("media/"),active=variant&&f[2].equals("active.txt");
            if(f[2].endsWith(".ogg"))tracks++;else if(f[2].endsWith(".mp4"))movies++;
            progress(t("Copiando mod HD: ","Copying HD mod: ")+(copied>>20)+" / "+(total>>20)+" MB");
            copied+=size;
            if(variant&&!active&&target.isFile()){
                if(target.length()!=size)throw new IOException("Immutable HD generation differs: "+f[2]);
                continue;
            }
            if(!active&&target.isFile()&&target.length()==size&&size>0)continue;
            File temp=File.createTempFile("hd-",".tmp",dir);
            try{
                try(InputStream in=resolver.openInputStream(android.provider.DocumentsContract.buildDocumentUriUsingTree(tree,f[0]));FileOutputStream out=new FileOutputStream(temp)){
                    if(in==null)throw new IOException(f[2]);copy(in,out,4L*1024*1024*1024);out.getFD().sync();}
                if(active){
                    if(!commitHdMedia(getFilesDir().getAbsolutePath(),f[1].equals("media/en")?1:2,temp.getAbsolutePath()))throw new IOException("Incomplete HD media; previous variant preserved");
                }else{
                    if(!variant&&target.exists()&&!target.delete())throw new IOException(f[2]);
                    if(!temp.renameTo(target))throw new IOException(f[2]);
                }
            }finally{if(temp.exists())temp.delete();}
        }
        return t("Mod HD importado: ","HD mod imported: ")+tracks+t(" pistas, "," tracks, ")+movies+t(" videos. Se activa en las opciones del juego."," movies. It is turned on in the game's options.");
    }
    private static String child(android.content.ContentResolver resolver,Uri tree,String parent,String wanted){
        try(android.database.Cursor c=resolver.query(android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(tree,parent),
                new String[]{android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID,android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME,android.provider.DocumentsContract.Document.COLUMN_MIME_TYPE},null,null,null)){
            while(c!=null&&c.moveToNext())
                if(wanted.equalsIgnoreCase(c.getString(1))&&android.provider.DocumentsContract.Document.MIME_TYPE_DIR.equals(c.getString(2)))return c.getString(0);
        }
        return null;
    }
    private void exportSave(Uri uri,int selectedSaveRegion,int selectedBank)throws Exception{
        File source=new File(getFilesDir(),SaveImport.bankName(selectedSaveRegion,selectedBank));if(!source.isFile())throw new IOException("Todavía no hay una partida guardada");
        try(FileInputStream in=new FileInputStream(source);OutputStream out=getContentResolver().openOutputStream(uri,"w")){if(out==null)throw new IOException("Destino no accesible");copy(in,out,65536);}
    }
}
