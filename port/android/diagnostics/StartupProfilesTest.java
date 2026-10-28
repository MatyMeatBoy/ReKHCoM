package com.rekhcom.android;
public final class StartupProfilesTest {
 public static void main(String[] args){
  for(String original:new String[]{null,"","0","1","custom"}){
   if(!java.util.Objects.equals(original,StartupProfiles.value("default",original)))throw new AssertionError("default");
   if(!"0".equals(StartupProfiles.value("hidapi-off",original)))throw new AssertionError("off");
   if(!java.util.Objects.equals(original,StartupProfiles.value("default",original)))throw new AssertionError("restoration");
  }
  for(String bad:new String[]{null,"","audio","HIDAPI-OFF","default ","../"}){
   try{StartupProfiles.value(bad,null);throw new AssertionError("unknown accepted");}catch(IllegalArgumentException expected){}
  }
  System.out.println("PASS profile defaults/off/restoration/unknown contracts");
 }
}
