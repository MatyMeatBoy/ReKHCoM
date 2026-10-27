package com.rekhcom.android;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
public final class LifecycleJournalTest {
    static void require(boolean value){if(!value)throw new AssertionError();}
    static String read(File file)throws Exception{return new String(Files.readAllBytes(file.toPath()),StandardCharsets.UTF_8);}
    public static void main(String[] args)throws Exception{
        File root=new File(args[0]);require(root.mkdirs());
        File old=new File(root,"lifecycle-game.log");Files.write(old.toPath(),"previous evidence".getBytes(StandardCharsets.UTF_8));
        LifecycleJournal journal=new LifecycleJournal(root,"game",123,768);
        require(journal.record(1,"onCreate","host64",false,0,false,false));journal.awaitForTests();
        require(read(new File(root,"lifecycle-game.log.1")).equals("previous evidence"));
        String first=read(old);require(first.contains("session="+journal.session()));require(first.contains("pid=123 instance=1 role=game event=onCreate runtime=host64"));
        for(int i=0;i<80;i++){require(journal.record(i+2,"onDestroy","host64",true,0x80000000,false,true));journal.awaitForTests();}
        for(File file:root.listFiles())require(file.isFile()&&file.length()<=768);
        require(root.listFiles().length==6);
        require(read(old).contains("config=0x80000000"));
        LifecycleJournal launcher=new LifecycleJournal(root,"launcher",124,768);
        require(launcher.record(1,"onPause","launcher",false,0,false,false));launcher.awaitForTests();
        require(read(old).contains("role=game"));
        require(read(new File(root,"lifecycle-launcher.log")).contains("role=launcher"));
        require(journal.record(99,"/secret/ROM.gba","token-password",false,0,false,false));journal.awaitForTests();
        String summary=LifecycleJournal.summary(root);
        require(summary.length()<=LifecycleJournal.SUMMARY_LIMIT);
        require(!summary.contains("ROM")&&!summary.contains("password")&&!summary.contains("previous evidence"));
        require(summary.contains("event=unknown runtime=unknown"));
        require(LifecycleJournal.clipShare(new String(new char[100000])).length()<=LifecycleJournal.SHARE_LIMIT);
        // An unexpected directory at a reserved log name must never be removed.
        File blocked=new File(args[0]+"-blocked");require(blocked.mkdirs());File keep=new File(blocked,"lifecycle-game.log.1");require(keep.mkdir());
        LifecycleJournal bad=new LifecycleJournal(blocked,"game",125,768);require(bad.record(1,"onDestroy","host64",true,0,false,false));bad.awaitForTests();
        require(bad.diskFailed()&&keep.isDirectory());
        // Queue capacity, even under a burst, is fixed; recording never waits for disk.
        for(int i=0;i<1000;i++)journal.record(i,"onStop","host64",false,0,false,false);
        require(journal.queuedForTests()<=LifecycleJournal.QUEUE_LIMIT);
        journal.closeForTests();launcher.closeForTests();bad.closeForTests();
        for(File file:root.listFiles())require(file.isFile()&&file.length()<=768);
        String prior=read(old);LifecycleJournal next=new LifecycleJournal(root,"game",126,768);
        require(!next.session().equals(journal.session()));
        require(next.record(1,"onCreate","pending",false,0,false,true));next.awaitForTests();
        require(read(new File(root,"lifecycle-game.log.1")).equals(prior));next.closeForTests();
        System.out.println("PASS lifecycle journal: session/instance, process isolation, actual+5, bytes/queue/share bounds, field allowlist, failure preservation");
    }
}
