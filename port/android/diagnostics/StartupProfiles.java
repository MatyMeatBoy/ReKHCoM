package com.rekhcom.android;
/** Diagnostic-only allowlist. Null original means absent, not an empty value. */
final class StartupProfiles {
 static final String KEY="SDL_JOYSTICK_HIDAPI";
 static String value(String mode,String original){
  if("default".equals(mode))return original;
  if("hidapi-off".equals(mode))return "0";
  throw new IllegalArgumentException("Unknown startup profile");
 }
}
