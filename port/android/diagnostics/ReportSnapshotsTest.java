package com.rekhcom.android;
import java.io.*;
import java.nio.file.*;
import java.util.Arrays;
public final class ReportSnapshotsTest {
    public static void main(String[] args)throws Exception {
        Path temp=Files.createTempDirectory("report-snapshots-");File directory=temp.resolve("shared").toFile(),source=temp.resolve("private.zip").toFile();
        byte[] first={0,1,(byte)255},second={9,8,7,6};
        Files.write(source.toPath(),first);File a=ReportSnapshots.create(source,directory);
        File preserved=ReportSnapshots.preservePrior(source);
        Files.write(source.toPath(),second);File b=ReportSnapshots.create(source,directory);
        if(!ReportSnapshots.preservePrior(source).equals(preserved)||!Arrays.equals(Files.readAllBytes(preserved.toPath()),first))throw new AssertionError("prior report overwritten");
        if(a.getName().equals(b.getName())||!Arrays.equals(Files.readAllBytes(a.toPath()),first)||!Arrays.equals(Files.readAllBytes(b.toPath()),second))throw new AssertionError("share snapshot changed/reused");
        if(!ReportSnapshots.file(directory,a.getName()).equals(a.getCanonicalFile()))throw new AssertionError("valid snapshot rejected");
        for(String bad:new String[]{"../private.zip","/private.zip","startup-../private.zip","startup-not-a-uuid.zip","startup-12345678-1234-1234-1234-123456789012.zip/extra"}){
            try{ReportSnapshots.file(directory,bad);throw new AssertionError("unsafe path accepted");}catch(FileNotFoundException expected){}
        }
        for(int i=0;i<12;i++)ReportSnapshots.create(source,directory);
        if(directory.listFiles().length!=8)throw new AssertionError("retention exceeded8");
        try{ReportSnapshots.file(directory,a.getName());throw new AssertionError("expired URI reused");}catch(FileNotFoundException expected){}
        System.out.println("PASS independent UUID shares, old bytes immutable, path rejection, retention8, expired URI not reused");
    }
}
