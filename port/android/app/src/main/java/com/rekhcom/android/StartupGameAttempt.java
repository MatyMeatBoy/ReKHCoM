package com.rekhcom.android;

/** Per-process ownership, not a process-wide "last Activity finished" flag. */
final class StartupGameAttempt {
    final String nonce;
    final int pid;
    final long generation;
    private StartupGameAttempt(String nonce,int pid,long generation){this.nonce=nonce;this.pid=pid;this.generation=generation;}
    static boolean valid(String nonce){return nonce!=null&&nonce.matches("[a-f0-9]{32}");}
    static final class Registry {
        private StartupGameAttempt current;
        private long generation;
        synchronized StartupGameAttempt begin(String nonce,int pid){
            if(!valid(nonce)||pid<=0)throw new IllegalArgumentException("attempt identity");
            current=new StartupGameAttempt(nonce,pid,++generation);return current;
        }
        synchronized boolean retire(StartupGameAttempt attempt,boolean threadTerminal,
                                    boolean finishing,boolean changing,Runnable action){
            if(attempt==null||current!=attempt||!threadTerminal||!finishing||changing)return false;
            // The action must target only attempt.pid. Holding this short lock
            // prevents an already-claimed B from being retired by A's cleanup.
            action.run();current=null;return true;
        }
    }
}
