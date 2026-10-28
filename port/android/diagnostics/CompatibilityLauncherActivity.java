package com.rekhcom.android;

import android.widget.Toast;
import org.libsdl.app.SDLActivity;

/** Nonexported diagnostic-only subclass; plain Java entry validates before launch. */
public final class CompatibilityLauncherActivity extends LauncherActivity {
 @Override protected String[] getArguments(){
  String actual=SDLActivity.nativeGetHint(StartupProfiles.KEY);
  if(!StartupProfileSession.verify(this,actual))runOnUiThread(()->Toast.makeText(this,"Startup profile did not apply; see diagnostic report",Toast.LENGTH_LONG).show());
  return super.getArguments();
 }
 @Override protected void onDestroy(){
  try{super.onDestroy();}finally{StartupProfileSession.restore(this);}
 }
}
