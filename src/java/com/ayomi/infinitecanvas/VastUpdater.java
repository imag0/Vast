package com.ayomi.infinitecanvas;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import android.view.ContextThemeWrapper;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.MessageDigest;
import org.json.JSONArray;
import org.json.JSONObject;

/** Small, signed, user-confirmed updater backed by the public Vast repository. */
final class VastUpdater {
    static final String AUTHORITY="com.ayomi.infinitecanvas.updates";
    private static final String MANIFEST_URL="https://raw.githubusercontent.com/imag0/Vast/main/updates/latest.json";
    private static final long CHECK_INTERVAL_MS=12L*60L*60L*1000L;
    private static boolean checking;

    private VastUpdater(){}

    static void maybeCheck(Activity activity,int background){
        long last=activity.getSharedPreferences("vast_updates",Context.MODE_PRIVATE).getLong("last_check",0);
        if(System.currentTimeMillis()-last>=CHECK_INTERVAL_MS)check(activity,background,false);
    }

    static void check(Activity activity,int background,boolean manual){
        synchronized(VastUpdater.class){if(checking)return;checking=true;}
        if(!manual)activity.getSharedPreferences("vast_updates",Context.MODE_PRIVATE).edit().putLong("last_check",System.currentTimeMillis()).apply();
        new Thread(()->{
            try{
                JSONObject manifest=readJson(MANIFEST_URL);int remoteCode=manifest.getInt("versionCode");String remoteName=manifest.getString("versionName");
                long localCode=localVersionCode(activity);activity.getSharedPreferences("vast_updates",Context.MODE_PRIVATE).edit().putLong("last_check",System.currentTimeMillis()).apply();
                if(remoteCode<=localCode){if(manual)activity.runOnUiThread(()->message(activity,background,"Vast is up to date","You already have the newest published version."));return;}
                String apkUrl=manifest.getString("apkUrl"),sha256=manifest.getString("sha256");
                activity.runOnUiThread(()->new AlertDialog.Builder(themed(activity,background)).setCustomTitle(VastStyle.title(themed(activity,background),"Vast "+remoteName+" is available",background))
                    .setMessage("Download the signed update from github.com/imag0/Vast? Android will ask you to confirm installation; your canvases stay on this device.")
                    .setNegativeButton("Later",null).setPositiveButton("Download",(d,w)->download(activity,background,apkUrl,sha256)).show());
            }catch(Exception ex){android.util.Log.w("VastUpdater","Update check failed",ex);if(manual)activity.runOnUiThread(()->message(activity,background,"Could not check for updates","Check your connection and try again."));}
            finally{synchronized(VastUpdater.class){checking=false;}}
        },"Vast update check").start();
    }

    private static void download(Activity activity,int background,String apkUrl,String expectedSha){
        new Thread(()->{
            File dir=new File(activity.getCacheDir(),"updates"),target=new File(dir,"vast-update.apk"),part=new File(dir,"vast-update.part");
            try{if(!dir.exists()&&!dir.mkdirs())throw new IllegalStateException("Cannot create update directory");
                HttpURLConnection connection=open(apkUrl);MessageDigest digest=MessageDigest.getInstance("SHA-256");
                try(InputStream in=connection.getInputStream();FileOutputStream out=new FileOutputStream(part)){byte[] buffer=new byte[65536];for(int n;(n=in.read(buffer))>=0;){if(n==0)continue;out.write(buffer,0,n);digest.update(buffer,0,n);}out.getFD().sync();}finally{connection.disconnect();}
                String actual=hex(digest.digest());if(!actual.equalsIgnoreCase(expectedSha))throw new SecurityException("Update checksum mismatch");
                if(target.exists()&&!target.delete())throw new IllegalStateException("Cannot replace old update");if(!part.renameTo(target))throw new IllegalStateException("Cannot finish update download");
                if(!sameSigner(activity,target))throw new SecurityException("Update signing certificate does not match this installation");
                activity.runOnUiThread(()->install(activity,background,target));
            }catch(Exception ex){part.delete();android.util.Log.w("VastUpdater","Update download failed",ex);activity.runOnUiThread(()->message(activity,background,"Update was not installed","The download or signature check failed. Your current version is unchanged."));}
        },"Vast update download").start();
    }

    private static void install(Activity activity,int background,File apk){
        if(Build.VERSION.SDK_INT>=26&&!activity.getPackageManager().canRequestPackageInstalls()){
            message(activity,background,"Allow Vast to install updates","Enable “Allow from this source”, then return to Vast and tap Check for updates again.");
            activity.startActivity(new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,Uri.parse("package:"+activity.getPackageName())));return;
        }
        Uri uri=Uri.parse("content://"+AUTHORITY+"/vast-update.apk");Intent intent=new Intent(Intent.ACTION_VIEW).setDataAndType(uri,"application/vnd.android.package-archive").addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_ACTIVITY_NEW_TASK);activity.startActivity(intent);
    }

    private static boolean sameSigner(Activity activity,File apk)throws Exception{
        PackageManager pm=activity.getPackageManager();int flags=Build.VERSION.SDK_INT>=28?PackageManager.GET_SIGNING_CERTIFICATES:PackageManager.GET_SIGNATURES;
        PackageInfo installed=pm.getPackageInfo(activity.getPackageName(),flags),archive=pm.getPackageArchiveInfo(apk.getAbsolutePath(),flags);if(archive==null||!activity.getPackageName().equals(archive.packageName))return false;
        Signature[] a=signatures(installed),b=signatures(archive);return a.length==1&&b.length==1&&MessageDigest.isEqual(a[0].toByteArray(),b[0].toByteArray());
    }

    private static Signature[] signatures(PackageInfo info){if(Build.VERSION.SDK_INT>=28&&info.signingInfo!=null)return info.signingInfo.getApkContentsSigners();return info.signatures==null?new Signature[0]:info.signatures;}
    private static long localVersionCode(Activity a)throws Exception{PackageInfo p=a.getPackageManager().getPackageInfo(a.getPackageName(),0);return Build.VERSION.SDK_INT>=28?p.getLongVersionCode():p.versionCode;}
    private static JSONObject readJson(String url)throws Exception{HttpURLConnection c=open(url);try(InputStream in=c.getInputStream()){byte[] buffer=new byte[8192];StringBuilder text=new StringBuilder();for(int n;(n=in.read(buffer))>=0;)if(n>0)text.append(new String(buffer,0,n,"UTF-8"));return new JSONObject(text.toString());}finally{c.disconnect();}}
    private static HttpURLConnection open(String value)throws Exception{HttpURLConnection c=(HttpURLConnection)new URL(value).openConnection();c.setConnectTimeout(10000);c.setReadTimeout(30000);c.setInstanceFollowRedirects(true);c.setRequestProperty("User-Agent","Vast-Android-Updater");int status=c.getResponseCode();if(status<200||status>=300)throw new IllegalStateException("HTTP "+status);return c;}
    private static String hex(byte[] value){StringBuilder b=new StringBuilder(value.length*2);for(byte v:value)b.append(String.format(java.util.Locale.ROOT,"%02x",v&255));return b.toString();}
    private static Context themed(Activity a,int background){return new ContextThemeWrapper(a,VastStyle.light(background)?android.R.style.Theme_Material_Light_Dialog_Alert:android.R.style.Theme_Material_Dialog_Alert);}
    private static void message(Activity a,int background,String title,String body){Context c=themed(a,background);new AlertDialog.Builder(c).setCustomTitle(VastStyle.title(c,title,background)).setMessage(body).setPositiveButton("OK",null).show();}
}
