package com.rekhcom.android;
import java.util.concurrent.atomic.AtomicInteger;
public final class StartupGameAttemptTest {
 static int checks;
 static void check(boolean ok){checks++;if(!ok)throw new AssertionError("check"+checks);}
 public static void main(String[]args)throws Exception{
  StartupGameAttempt.Registry registry=new StartupGameAttempt.Registry();AtomicInteger retired=new AtomicInteger();
  StartupGameAttempt a=registry.begin("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",10);
  check(!registry.retire(a,false,true,false,retired::incrementAndGet));
  check(!registry.retire(a,true,false,false,retired::incrementAndGet));
  check(!registry.retire(a,true,true,true,retired::incrementAndGet));
  StartupGameAttempt b=registry.begin("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",10);
  check(!registry.retire(a,true,true,false,retired::incrementAndGet));check(retired.get()==0);
  check(registry.retire(b,true,true,false,retired::incrementAndGet));check(retired.get()==1);
  check(!registry.retire(b,true,true,false,retired::incrementAndGet));
  a=registry.begin("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",11);
  b=registry.begin("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",11); // same nonce, new Activity
  check(b.generation>a.generation);
  check(!registry.retire(a,true,true,false,retired::incrementAndGet));
  check(registry.retire(b,true,true,false,retired::incrementAndGet));
  try{registry.begin("../wrong",11);throw new AssertionError();}catch(IllegalArgumentException expected){checks++;}
  try{registry.begin("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",0);throw new AssertionError();}catch(IllegalArgumentException expected){checks++;}
  java.io.File root=new java.io.File(args[0]);root.mkdirs();
  java.io.File save=new java.io.File(root,"kh_com.sav");java.nio.file.Files.write(save.toPath(),new byte[]{1,2,3});
  check(!StartupStageJournal.record(root,"../other","aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",10,"native_enter",0));
  check(!StartupStageJournal.record(root,"game","../wrong",10,"native_enter",0));
  check(!StartupStageJournal.record(root,"game","aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",10,"bad\nsecret",0));
  for(int n=0;n<320;n++)check(StartupStageJournal.record(root,"game","aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",10,"native_return",n));
  java.io.File log=new java.io.File(root,"startup-game.log");check(log.length()<=StartupStageJournal.LIMIT);
  check(new java.io.File(root,"startup-game.log.1").length()<=StartupStageJournal.LIMIT);
  String text=StartupStageJournal.summary(root);check(text.contains("code=319"));
  check(java.util.Arrays.equals(java.nio.file.Files.readAllBytes(save.toPath()),new byte[]{1,2,3}));
  try{RomImport.verifyInstalled(save);throw new AssertionError();}catch(java.io.IOException expected){checks++;}
  if(args.length>1)check("B8CP".equals(RomImport.verifyInstalled(new java.io.File(args[1]))));
  System.out.println("PASS attempt ownership/journal: "+checks+" checks; stale A cannot retire B; SAV unchanged");
 }
}
