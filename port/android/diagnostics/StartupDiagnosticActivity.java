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
    private Button open,copy,share,refresh,play,pick;
    private Spinner region;
    private final String nonce=java.util.UUID.randomUUID().toString().replace("-","");
    private static final int PICK_ROM=10,PLAY_GAME=60,OPEN_GRAPHICAL=61;
    private static final String[] CODES={"B8CP","B8CE","B8CJ"},REGIONS={"eu","us","jp"};
    private volatile boolean collecting,working;
    private boolean childPending;
    private boolean initialSelection=true;
    private boolean spanish;
    private String t(String es,String en){return spanish?es:en;}
    @Override public void onCreate(Bundle state){
        super.onCreate(state);spanish="es".equals(Locale.getDefault().getLanguage());
        childPending=state!=null&&state.getBoolean("startup_child_pending",false);working=childPending;
        LinearLayout layout=new LinearLayout(this);layout.setOrientation(LinearLayout.VERTICAL);layout.setPadding(24,24,24,24);
        TextView title=new TextView(this);title.setText(t("ReKHCoM · Arranque seguro de prueba","ReKHCoM · Safe startup test"));title.setTextSize(22);layout.addView(title);
        TextView help=new TextView(this);help.setText(t("Jugar inicia directamente el juego sin cargar el launcher gráfico. Puedes usar tu ROM ya instalada. Si vuelve a cerrarse, regresa aquí y comparte el informe. Esta prueba no garantiza corregir la causa en tu dispositivo.","Play starts the game directly without loading the graphical launcher. You can use your installed ROM. If it closes again, return here and share the report. This test does not establish a device-specific fix."));layout.addView(help);
        region=new Spinner(this);region.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{"EU","US","JP"}));layout.addView(region);
        play=button(layout,t("Jugar con ROM instalada","Play installed ROM"),()->launch(false));
        pick=button(layout,t("Seleccionar ROM","Select ROM"),this::pickRom);
        open=button(layout,t("Launcher gráfico (opcional)","Graphical launcher (optional)"),()->launch(true));
        copy=button(layout,t("Copiar resumen","Copy summary"),()->{ClipboardManager c=(ClipboardManager)getSystemService(CLIPBOARD_SERVICE);if(c!=null)c.setPrimaryClip(ClipData.newPlainText("ReKHCoM startup",report.getText()));});
        share=button(layout,t("Compartir informe","Share report"),this::share);
        refresh=button(layout,t("Actualizar informe","Refresh report"),this::collect);
        report=new TextView(this);report.setTextIsSelectable(true);layout.addView(report);
        ScrollView scroll=new ScrollView(this);scroll.addView(layout);setContentView(scroll);
        collect();
    }
    @Override protected void onResume(){super.onResume();if(report!=null)collect();}
    @Override protected void onSaveInstanceState(Bundle state){state.putBoolean("startup_child_pending",childPending);super.onSaveInstanceState(state);}
    private void controls(){if(play!=null){play.setEnabled(!working&&!collecting);pick.setEnabled(!working&&!collecting);open.setEnabled(!working&&!collecting);}}
    private void launch(boolean graphical){
        if(working||collecting)return;working=true;controls();
        final int selected=region.getSelectedItemPosition();
        final String attempt=java.util.UUID.randomUUID().toString().replace("-","");
        final Context context=getApplicationContext();
        new Thread(()->{
            try {
                File rom=new File(context.getFilesDir(),"roms/"+CODES[selected]+".gba");
                if(!graphical&&!CODES[selected].equals(RomImport.verifyInstalled(rom)))throw new java.io.IOException("Installed ROM region mismatch");
                String role=graphical?"launcher":"game";
                if(!StartupProcessGate.awaitAbsent(context,role))throw new java.io.IOException(t("La ventana anterior todavía está cerrándose. Intenta de nuevo o comparte el informe.","The previous window is still closing. Try again or share the report."));
                StartupStageJournal.record(context.getFilesDir(),"safe",attempt,android.os.Process.myPid(),graphical?"launcher_requested":"game_requested",selected);
                runOnUiThread(()->{
                    if(isFinishing()||isDestroyed())return;
                    Intent intent=new Intent().setClassName(getPackageName(),"com.rekhcom.android."+(graphical?"LauncherActivity":"GameActivity"));
                    intent.putExtra("startup_attempt",attempt).putExtra("region",REGIONS[selected]).putExtra("rom",rom.getAbsolutePath());
                    childPending=true;
                    try{startActivityForResult(intent,graphical?OPEN_GRAPHICAL:PLAY_GAME);}
                    catch(Exception error){childPending=false;working=false;controls();showFailure(error);}
                });
            }catch(Exception error){runOnUiThread(()->{working=false;if(isFinishing()||isDestroyed())return;controls();showFailure(error);});}
        },"safe-startup-prepare").start();
    }
    private void showFailure(Exception error){Toast.makeText(this,t("No se pudo iniciar: ","Could not start: ")+error.getMessage(),Toast.LENGTH_LONG).show();}
    private void pickRom(){
        if(working||collecting)return;
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        try{startActivityForResult(intent,PICK_ROM);}catch(ActivityNotFoundException error){showFailure(error);}
    }
    @Override protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);
        if(request==PLAY_GAME||request==OPEN_GRAPHICAL){childPending=false;working=false;controls();collect();return;}
        if(request!=PICK_ROM||result!=RESULT_OK||data==null||data.getData()==null)return;
        final Uri uri=data.getData();working=true;controls();final Context context=getApplicationContext();
        new Thread(()->{
            try {
                RomImport.Result imported=RomImport.install(context.getContentResolver().openInputStream(uri),context.getFilesDir(),(pending,target)->{
                    try{android.system.Os.rename(pending.getAbsolutePath(),target.getAbsolutePath());}
                    catch(android.system.ErrnoException error){throw new java.io.IOException("Atomic import failed",error);}
                });
                StartupStageJournal.record(context.getFilesDir(),"safe",nonce,android.os.Process.myPid(),"rom_import",imported.kind==RomImport.Kind.ROM?1:0);
                runOnUiThread(()->{working=false;if(isFinishing()||isDestroyed())return;for(int n=0;n<CODES.length;n++)if(CODES[n].equals(imported.code))region.setSelection(n);controls();Toast.makeText(this,t("Importación verificada.","Import verified."),Toast.LENGTH_LONG).show();collect();});
            }catch(Exception error){runOnUiThread(()->{working=false;if(isFinishing()||isDestroyed())return;controls();showFailure(error);});}
        },"safe-rom-import").start();
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
        if(collecting)return;collecting=true;controls();copy.setEnabled(false);share.setEnabled(false);refresh.setEnabled(false);report.setText(t("Leyendo información de cierre de esta app…","Reading this app's exit information…"));
        final Context context=getApplicationContext();
        new Thread(()->{
            String result;boolean ok;
            int first=-1,count=0;
            for(int n=0;n<CODES.length;n++){File rom=new File(context.getFilesDir(),"roms/"+CODES[n]+".gba");if(rom.isFile()&&rom.length()==33554432){first=n;count++;}}
            try{StartupStageJournal.record(context.getFilesDir(),"safe",nonce,android.os.Process.myPid(),"java_entry",0);result=StartupExitCollector.collect(context);ok=true;}catch(Exception e){result=t("No se pudo crear el informe (el previo se conserva): ","Could not create report (previous report retained): ")+e;ok=StartupExitCollector.report(context).isFile();}
            final String text=result;final boolean ready=ok;
            final int installed=first,installedCount=count;
            runOnUiThread(()->{collecting=false;if(isFinishing()||isDestroyed())return;if(initialSelection&&installedCount==1)region.setSelection(installed);initialSelection=false;controls();report.setText(text);copy.setEnabled(true);share.setEnabled(ready);refresh.setEnabled(true);});
        },"startup-exit-report").start();
    }
}
