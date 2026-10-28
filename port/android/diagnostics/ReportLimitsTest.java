package com.rekhcom.android;
import java.io.*;
import java.util.Arrays;
public final class ReportLimitsTest {
    public static void main(String[] args) throws Exception {
        byte[] binary = {0, (byte)255, 13, 10, (byte)128, 0};
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        if (ReportLimits.copy(new ByteArrayInputStream(binary), out, 6) != 6 || !Arrays.equals(binary,out.toByteArray())) throw new AssertionError("binary trace changed");
        try { ReportLimits.copy(new ByteArrayInputStream(binary),new ByteArrayOutputStream(),5); throw new AssertionError("oversized trace accepted"); }
        catch(ReportLimits.TooLarge expected) {}
        if (!ReportLimits.safeName("tombstone-123.pb") || ReportLimits.safeName("../data.sav") || ReportLimits.safeName("/absolute") || ReportLimits.safeName("a/b")) throw new AssertionError("unsafe name");
        if(ReportLimits.text("abcdefgh",5).length()>5 || !ReportLimits.text(null,5).equals("")) throw new AssertionError("text cap");
        if(!ReportLimits.utf8Text("\u20ac!",3).equals("\u20ac")||!ReportLimits.utf8Text("\u20ac!",2).isEmpty())throw new AssertionError("UTF8 byte cap");
        ByteArrayOutputStream empty = new ByteArrayOutputStream();
        if(ReportLimits.copy(new ByteArrayInputStream(new byte[0]),empty,0)!=0) throw new AssertionError("empty trace");
        System.out.println("PASS byte-exact binary/empty trace, oversize rejection, bounded text, safe export names");
    }
}
