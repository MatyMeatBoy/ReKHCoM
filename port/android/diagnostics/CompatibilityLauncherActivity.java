package com.rekhcom.android;

import android.os.Bundle;
import android.widget.Toast;
import org.libsdl.app.SDLActivity;

/** Nonexported isolated diagnostic process; normal launcher/game processes untouched. */
public final class CompatibilityLauncherActivity extends LauncherActivity {
 @Override protected void onCreate(Bundle state){
  String requested=getIntent().getStringExtra("startup_profile");
  try{StartupProfileSession.prepare(this,requested);}
  catch(Exception e){
   StartupProfileSession.restore(this);
   StartupProfileSession.rejected(this);
   Toast.makeText(this,"Profile rejected; continuing with original default",Toast.LENGTH_LONG).show();
  }
  super.onCreate(state);
 }
 @Override protected String[] getArguments(){
  String actual=SDLActivity.nativeGetHint(StartupProfiles.KEY);
  if(!StartupProfileSession.verify(this,actual))runOnUiThread(()->Toast.makeText(this,"Startup profile did not apply; see diagnostic report",Toast.LENGTH_LONG).show());
  return super.getArguments();
 }
 @Override protected void onDestroy(){
  try{super.onDestroy();}finally{
   StartupProfileSession.restore(this);
   if(isFinishing()&&StartupProfileSession.isOwnProfileProcess(this))android.os.Process.killProcess(android.os.Process.myPid());
  }
 }
}
