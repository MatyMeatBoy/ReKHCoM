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
        open=button(layout,t("Abrir launcher","Open launcher"),()->startActivity(new Intent().setClassName(getPackageName(),"com.rekhcom.android.LauncherActivity")));
        copy=button(layout,t("Copiar resumen","Copy summary"),()->{ClipboardManager c=(ClipboardManager)getSystemService(CLIPBOARD_SERVICE);if(c!=null)c.setPrimaryClip(ClipData.newPlainText("ReKHCoM startup",report.getText()));});
        share=button(layout,t("Compartir informe","Share report"),this::share);
        refresh=button(layout,t("Actualizar informe","Refresh report"),this::collect);
        ScrollView scroll=new ScrollView(this);report=new TextView(this);report.setTextIsSelectable(true);scroll.addView(report);layout.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));setContentView(layout);
        collect();
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
