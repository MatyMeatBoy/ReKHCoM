package com.rekhcom.android;
import android.content.*;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import java.io.*;

/** Variant only: immutable UUID snapshots, protected by exact read-only URI grants. */
public final class StartupReportProvider extends ContentProvider {
    @Override public boolean onCreate(){return true;}
    private File file(Uri uri)throws FileNotFoundException {
        if(getContext()==null||!"content".equals(uri.getScheme())||!(getContext().getPackageName()+".startup-report").equals(uri.getAuthority())||uri.getPath()==null||!uri.getPath().startsWith("/")||uri.getQuery()!=null||uri.getFragment()!=null)throw new FileNotFoundException("Unknown report URI");
        return ReportSnapshots.file(StartupExitCollector.sharedDirectory(getContext()),uri.getPath().substring(1));
    }
    @Override public ParcelFileDescriptor openFile(Uri uri,String mode)throws FileNotFoundException {if(!"r".equals(mode))throw new FileNotFoundException("Report is read only");return ParcelFileDescriptor.open(file(uri),ParcelFileDescriptor.MODE_READ_ONLY);}
    @Override public String getType(Uri uri){try{file(uri);return "application/zip";}catch(FileNotFoundException e){return null;}}
    @Override public Cursor query(Uri uri,String[] projection,String selection,String[] args,String order){
        try{File file=file(uri);String[] columns=projection==null?new String[]{OpenableColumns.DISPLAY_NAME,OpenableColumns.SIZE}:projection;MatrixCursor cursor=new MatrixCursor(columns,1);Object[] values=new Object[columns.length];for(int i=0;i<columns.length;i++){if(OpenableColumns.DISPLAY_NAME.equals(columns[i]))values[i]="ReKHCoM-startup-report.zip";else if(OpenableColumns.SIZE.equals(columns[i]))values[i]=file.length();}cursor.addRow(values);return cursor;}catch(FileNotFoundException e){return null;}
    }
    @Override public Uri insert(Uri u,ContentValues v){throw new UnsupportedOperationException("Read only");}
    @Override public int update(Uri u,ContentValues v,String s,String[] a){throw new UnsupportedOperationException("Read only");}
    @Override public int delete(Uri u,String s,String[] a){throw new UnsupportedOperationException("Read only");}
}
