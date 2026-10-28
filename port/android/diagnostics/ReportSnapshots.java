package com.rekhcom.android;
import java.io.*;
import java.util.*;

/** Immutable per-user-action exports. Only this dedicated cache folder is managed. */
final class ReportSnapshots {
    private static final int RETAIN=8;
    /** Preserve once before safe-entry auto-collection replaces the old report.
     * This cache backup is separate from rolling share snapshots; never rewrite it. */
    static synchronized File preservePrior(File source)throws IOException {
        File target=new File(source.getParentFile(),"pre-safe-entry-report.zip");
        if(target.exists()){if(!target.isFile())throw new IOException("Prior report backup is not a file");return target;}
        if(!source.isFile())return null;
        File pending=File.createTempFile("pre-safe-entry-",".pending",source.getParentFile());
        try{
            try(InputStream in=new FileInputStream(source);FileOutputStream out=new FileOutputStream(pending)){
                ReportLimits.copy(in,out,ReportLimits.TRACE_LIMIT+ReportLimits.TEXT_LIMIT+65536);out.getFD().sync();
            }
            if(target.exists())return target;
            if(!pending.renameTo(target))throw new IOException("Cannot preserve prior report");
            return target;
        }finally{pending.delete();}
    }
    static boolean name(String value){return value!=null&&value.matches("startup-[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\\.zip");}
    static File file(File directory,String name)throws FileNotFoundException {
        try {
            if(!name(name))throw new FileNotFoundException("Unknown snapshot");
            File base=directory.getCanonicalFile(),file=new File(base,name).getCanonicalFile();
            if(!base.equals(file.getParentFile())||!file.isFile())throw new FileNotFoundException("Snapshot unavailable or expired");
            return file;
        }catch(IOException e){throw new FileNotFoundException(e.getMessage());}
    }
    static synchronized File create(File source,File directory)throws IOException {
        if(!directory.isDirectory()&&!directory.mkdirs())throw new IOException("Cannot create share folder");
        File base=directory.getCanonicalFile();File[] files=base.listFiles();
        if(files==null)throw new IOException("Cannot list share snapshots");
        ArrayList<File> snapshots=new ArrayList<>();
        for(File file:files){
            if(!base.equals(file.getCanonicalFile().getParentFile()))continue;
            if(name(file.getName())&&file.isFile())snapshots.add(file);
            else if(file.getName().matches("pending-share-[0-9]+\\.tmp")&&file.isFile()&&!file.delete())throw new IOException("Cannot remove incomplete share");
        }
        Collections.sort(snapshots,(a,b)->{int time=Long.compare(a.lastModified(),b.lastModified());return time!=0?time:a.getName().compareTo(b.getName());});
        while(snapshots.size()>=RETAIN){File old=snapshots.remove(0);if(!old.delete())throw new IOException("Cannot enforce snapshot retention");}
        File target=new File(base,"startup-"+UUID.randomUUID().toString()+".zip"),pending=File.createTempFile("pending-share-",".tmp",base);
        try {
            try(InputStream input=new FileInputStream(source);FileOutputStream output=new FileOutputStream(pending)){
                ReportLimits.copy(input,output,ReportLimits.TRACE_LIMIT+ReportLimits.TEXT_LIMIT+65536);output.getFD().sync();
            }
            if(target.exists()||!pending.renameTo(target))throw new IOException("Cannot finalize share snapshot");
            return target;
        }finally{pending.delete();}
    }
}
