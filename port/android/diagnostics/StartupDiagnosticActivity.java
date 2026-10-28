package com.rekhcom.android;

import android.app.Activity;
import android.content.*;
import android.net.Uri;
import android.os.Bundle;
import android.widget.*;
import java.util.Locale;
import java.io.File;

/** Optional variant entry: no SDL inheritance, native calls or library loads. */
public final class StartupDiagnosticActivity extends Activity {
    private TextView report;
    private Button open,copy,share,refresh;
    private volatile boolean collecting;
    private boolean spanish;
    private String t(String es,String en){return spanish?es:en;}
    @Override public void onCreate(Bundle state){
        super.onCreate(state);spanish="es".equals(Locale.getDefault().getLanguage());
        LinearLayout layout=new LinearLayout(this);layout.setOrientation(LinearLayout.VERTICAL);layout.setPadding(24,24,24,24);
        TextView title=new TextView(this);title.setText(t("ReKHCoM · Diagnóstico de arranque","ReKHCoM · Startup diagnostic"));title.setTextSize(22);layout.addView(title);
        TextView help=new TextView(this);help.setText(t("Prueba opcional: no corrige el cierre. Esta pantalla no carga SDL. Abre el launcher una vez; si se cierra, vuelve aquí y comparte el informe. Red desactivada en esta variante.","Optional test, not a crash fix. This screen does not load SDL. Open the launcher once; if it closes, return here and share the report. Network is disabled in this variant."));layout.addView(help);
        boolean options=false;
        try{getPackageManager().getActivityInfo(new ComponentName(getPackageName(),"com.rekhcom.android.CompatibilityLauncherActivity"),0);options=true;}catch(android.content.pm.PackageManager.NameNotFoundException ignored){}
        final boolean compat=options;
        open=button(layout,t("Abrir launcher (predeterminado)","Open launcher (default)"),()->openLauncher(compat,"default"));
        if(compat){
            button(layout,t("Compatibilidad de mandos (sin HIDAPI)","Controller compatibility (HIDAPI off)"),()->openLauncher(true,"hidapi-off"));
            TextView info=new TextView(this);info.setText(t("Sólo esta prueba; mandos Android y touch siguen activos. No afecta al juego, que usa otro proceso.","This launcher test only; Android controllers and touch remain active. Does not affect the game, which uses another process."));layout.addView(info);
        }
        copy=button(layout,t("Copiar resumen","Copy summary"),()->{ClipboardManager c=(ClipboardManager)getSystemService(CLIPBOARD_SERVICE);if(c!=null)c.setPrimaryClip(ClipData.newPlainText("ReKHCoM startup",report.getText()));});
        share=button(layout,t("Compartir informe","Share report"),this::share);
        refresh=button(layout,t("Actualizar informe","Refresh report"),this::collect);
        ScrollView scroll=new ScrollView(this);report=new TextView(this);report.setTextIsSelectable(true);scroll.addView(report);layout.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));setContentView(layout);
        collect();
    }
    private void openLauncher(boolean compat,String mode){
        Intent intent=new Intent().setClassName(getPackageName(),compat?"com.rekhcom.android.CompatibilityLauncherActivity":"com.rekhcom.android.LauncherActivity");
        if(compat){
            try{
                // Reflection keeps the original diagnostic build free of optional profile classes.
                Class<?> gate=Class.forName("com.rekhcom.android.StartupProfileSession");
                java.lang.reflect.Method prepare=gate.getDeclaredMethod("prepare",Context.class,String.class);prepare.setAccessible(true);prepare.invoke(null,this,mode);
            }catch(Exception e){Toast.makeText(this,t("Perfil rechazado; launcher no abierto.","Profile rejected; launcher was not opened."),Toast.LENGTH_LONG).show();return;}
        }
        try{startActivity(intent);}catch(Exception e){
            if(compat)try{Class<?> gate=Class.forName("com.rekhcom.android.StartupProfileSession");java.lang.reflect.Method restore=gate.getDeclaredMethod("restore",Context.class);restore.setAccessible(true);restore.invoke(null,this);}catch(Exception ignored){}
            Toast.makeText(this,t("No se pudo abrir el launcher.","Could not open launcher."),Toast.LENGTH_LONG).show();
        }
    }
    private Button button(LinearLayout parent,String label,Runnable action){Button b=new Button(this);b.setText(label);b.setOnClickListener(v->action.run());parent.addView(b);return b;}
    private void share(){
        share.setEnabled(false);final Context context=getApplicationContext();
        new Thread(()->{
            try {
                File snapshot=StartupExitCollector.shareSnapshot(context);
                final Uri uri=Uri.parse("content://"+getPackageName()+".startup-report/"+snapshot.getName());
                runOnUiThread(()->{
                    if(isFinishing()||isDestroyed())return;share.setEnabled(true);
                    Intent intent=new Intent(Intent.ACTION_SEND).setType("application/zip").putExtra(Intent.EXTRA_STREAM,uri).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    intent.setClipData(ClipData.newUri(getContentResolver(),"ReKHCoM startup report",uri));
                    try{startActivity(Intent.createChooser(intent,t("Compartir informe","Share report")));}catch(ActivityNotFoundException e){Toast.makeText(this,t("No hay una app para compartir. Puedes copiar el resumen.","No sharing app available. You can copy the summary."),Toast.LENGTH_LONG).show();}
                });
            }catch(Exception e){runOnUiThread(()->{if(isFinishing()||isDestroyed())return;share.setEnabled(true);Toast.makeText(this,t("No se pudo compartir: ","Could not share: ")+e.getMessage(),Toast.LENGTH_LONG).show();});}
        },"startup-share-snapshot").start();
    }
    private void collect(){
        if(collecting)return;collecting=true;copy.setEnabled(false);share.setEnabled(false);refresh.setEnabled(false);report.setText(t("Leyendo información de cierre de esta app…","Reading this app's exit information…"));
        final Context context=getApplicationContext();
        new Thread(()->{
            String result;boolean ok;
            try{result=StartupExitCollector.collect(context);ok=true;}catch(Exception e){result=t("No se pudo crear el informe: ","Could not create report: ")+e;ok=false;}
            final String text=result;final boolean ready=ok;
            runOnUiThread(()->{collecting=false;if(isFinishing()||isDestroyed())return;report.setText(text);copy.setEnabled(true);share.setEnabled(ready);refresh.setEnabled(true);});
        },"startup-exit-report").start();
    }
}
