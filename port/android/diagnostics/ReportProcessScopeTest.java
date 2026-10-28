package com.rekhcom.android;
public final class ReportProcessScopeTest {
 public static void main(String[]args){
  String app="com.rekhcom.android";int own=0,legacy=0;
  String[] records={app+":startup_profile",app+":startup_profile",app+":startup_profile",app+":startup_profile",app,app+":game",app+":launcher","other.app:startup_profile"};
  for(String process:records)if(ReportProcessScope.owns(app,process)){own++;if(process.endsWith(":startup_profile"))legacy++;}
  if(own!=7||legacy!=4)throw new AssertionError("lost own legacy crash records");
  for(String process:new String[]{null,"com.rekhcom.android.other",app+":unrelated",app+":startup-profile"})
   if(ReportProcessScope.owns(app,process))throw new AssertionError("unexpected process accepted");
  System.out.println("PASS exact own-process scope: mixed8 retains4legacy+game/launcher/main, rejects foreign/null");
 }
}
