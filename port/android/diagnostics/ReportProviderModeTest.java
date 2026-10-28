package com.rekhcom.android;
import java.io.FileNotFoundException;
import java.lang.reflect.Field;
import sun.misc.Unsafe;

/** Host-only: bypass the Android SDK stub constructor, then run the real guard. */
public final class ReportProviderModeTest {
    public static void main(String[] args)throws Exception {
        Field field=Unsafe.class.getDeclaredField("theUnsafe");field.setAccessible(true);
        StartupReportProvider provider=(StartupReportProvider)((Unsafe)field.get(null)).allocateInstance(StartupReportProvider.class);
        for(String mode:new String[]{null,"w","rw","wt","rwt","wa","R",""}){
            try{provider.openFile(null,mode);throw new AssertionError("write/nonread mode accepted");}
            catch(FileNotFoundException expected){if(!"Report is read only".equals(expected.getMessage()))throw expected;}
        }
        System.out.println("PASS actual provider rejects8nonread/write modes before any URI/context access");
    }
}
