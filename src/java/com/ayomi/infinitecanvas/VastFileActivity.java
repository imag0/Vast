package com.ayomi.infinitecanvas;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.widget.Toast;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

/** Isolated Storage Access Framework bridge for portable, offline Vast files. */
public final class VastFileActivity extends Activity {
    private static final int PICK=9040;
    private int mode;
    private String source,pending;

    public static void launch(Activity host,int mode,String source,String name,String pending){
        Intent i=new Intent(host,VastFileActivity.class);i.putExtra("mode",mode);i.putExtra("source",source);i.putExtra("name",name);i.putExtra("pending",pending);host.startActivity(i);
    }
    @Override public void onCreate(Bundle state){
        super.onCreate(state);mode=getIntent().getIntExtra("mode",0);source=getIntent().getStringExtra("source");pending=getIntent().getStringExtra("pending");
        Intent pick;if(mode==1){pick=new Intent(Intent.ACTION_CREATE_DOCUMENT);pick.setType("application/vnd.vast.project");pick.putExtra(Intent.EXTRA_TITLE,getIntent().getStringExtra("name"));}
        else{pick=new Intent(Intent.ACTION_OPEN_DOCUMENT);pick.setType("*/*");pick.addCategory(Intent.CATEGORY_OPENABLE);}startActivityForResult(pick,PICK);
    }
    private static void copy(InputStream in,OutputStream out)throws Exception{byte[] b=new byte[65536];for(int n;(n=in.read(b))>=0;)if(n>0)out.write(b,0,n);out.flush();}
    @Override protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);if(request!=PICK||result!=RESULT_OK||data==null||data.getData()==null){if(mode==1&&source!=null)new File(source).delete();finish();return;}Uri uri=data.getData();
        try{if(mode==1){try(InputStream in=new FileInputStream(new File(source));OutputStream out=getContentResolver().openOutputStream(uri,"w")){if(out==null)throw new IllegalStateException("No export stream");copy(in,out);}}
            else{File dst=new File(pending),tmp=new File(pending+".tmp");try(InputStream in=getContentResolver().openInputStream(uri);OutputStream out=new FileOutputStream(tmp)){if(in==null)throw new IllegalStateException("No import stream");copy(in,out);((FileOutputStream)out).getFD().sync();}if(dst.exists()&&!dst.delete())throw new IllegalStateException("Cannot replace import");if(!tmp.renameTo(dst))throw new IllegalStateException("Cannot commit import");}}
        catch(Exception ex){android.util.Log.e("VastFiles","Transfer failed",ex);if(mode==2&&pending!=null)try{new FileOutputStream(new File(pending+".error")).close();}catch(Exception ignored){}Toast.makeText(this,"Vast could not transfer this project",Toast.LENGTH_LONG).show();}
        finally{if(mode==1&&source!=null)new File(source).delete();}finish();
    }
}
