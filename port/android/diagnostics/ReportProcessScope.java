package com.rekhcom.android;
/** Exact own-package allowlist, including the published legacy diagnostic child. */
final class ReportProcessScope {
    static boolean owns(String app,String process){
        return app!=null&&process!=null&&(app.equals(process)||(app+":game").equals(process)||
            (app+":launcher").equals(process)||(app+":startup_profile").equals(process));
    }
}
