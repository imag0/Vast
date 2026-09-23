package com.ayomi.infinitecanvas;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.text.TextPaint;
import android.text.StaticLayout;
import android.text.Layout;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.GradientDrawable;
import android.content.res.ColorStateList;
import android.os.Build;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.text.Spanned;
import android.text.TextWatcher;
import android.view.ContextThemeWrapper;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import org.json.JSONArray;
import org.json.JSONObject;
import java.nio.charset.StandardCharsets;

/** Platform UI only. Document ownership, tools, persistence and rendering stay in C. */
public final class VastUi {
    private static final class BoundedScrollView extends ScrollView {
        int maxHeight;
        BoundedScrollView(Context c,int maximum){super(c);maxHeight=maximum;}
        @Override protected void onMeasure(int width,int height){super.onMeasure(width,View.MeasureSpec.makeMeasureSpec(maxHeight,View.MeasureSpec.AT_MOST));}
    }
    // Shared cross-axis size keeps the top pill and compact rail in proportion.
    private static final int COMPACT_CHROME_DP=60;
    private static AlertDialog editorDialog, panelDialog;
    private static EditText editor;
    private static Activity owner;
    private static Activity refreshOwner;
    private static float preferredRefresh;
    private static void requestHighRefresh(Activity a){
        if(refreshOwner==a)return;refreshOwner=a;preferredRefresh=0;
        android.view.Display display=a.getWindowManager().getDefaultDisplay();
        android.view.Display.Mode current=display.getMode();
        for(android.view.Display.Mode mode:display.getSupportedModes())if(mode.getPhysicalWidth()==current.getPhysicalWidth()&&mode.getPhysicalHeight()==current.getPhysicalHeight()&&mode.getRefreshRate()<=120.5f&&mode.getRefreshRate()>preferredRefresh)preferredRefresh=mode.getRefreshRate();
        if(preferredRefresh>60){WindowManager.LayoutParams lp=a.getWindow().getAttributes();lp.preferredRefreshRate=preferredRefresh;a.getWindow().setAttributes(lp);}
        /* A window preference, not a global override: Android retains control
           for battery, thermal and user refresh-rate policy. */
    }
    private static String panelKey = "";
    private static String panelTitle="";
    private static int panelBackground;
    private static ScrollView panelScroll;
    private static LinearLayout panelContainer;
    private static int panelSection=-1;
    private static final java.util.HashMap<Integer,Integer> sectionScroll=new java.util.HashMap<>();
    private static boolean closing;
    private static int editorMode;
    private static PopupWindow header, identity, rail, contextTools, toolPanel;
    private static String chromeKey="";
    private static int chromeBackground;
    private static native void nativeText(String value);
    private static native void nativeAction(int action);
    private static native void nativeCancel();
    private static native void nativeUnit(int unit);
    private static native void nativeColor(int color);
    private static native int[] nativeThumbnail(int index);
    private static android.os.ParcelFileDescriptor photoDescriptor;
    private static android.graphics.BitmapRegionDecoder photoDecoder;
    private static long photoDecoderId=-1;

    private static synchronized void closePhotoDecoder(){
        if(photoDecoder!=null){photoDecoder.recycle();photoDecoder=null;}
        if(photoDescriptor!=null){try{photoDescriptor.close();}catch(Exception ignored){}photoDescriptor=null;}
        photoDecoderId=-1;
    }

    /** Source dimensions and orientation for bounded, region-by-region import. */
    public static int[] photoInfo(Activity activity,long mediaId,int maxEdge){
        android.net.Uri uri=android.content.ContentUris.withAppendedId(android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI,mediaId);
        int width=0,height=0,orientation=0;
        try(android.os.ParcelFileDescriptor pfd=activity.getContentResolver().openFileDescriptor(uri,"r")){
            if(pfd==null)return null;android.graphics.BitmapFactory.Options options=new android.graphics.BitmapFactory.Options();options.inJustDecodeBounds=true;
            android.graphics.BitmapFactory.decodeFileDescriptor(pfd.getFileDescriptor(),null,options);width=options.outWidth;height=options.outHeight;
        }catch(Exception ex){android.util.Log.w("VastUi","Unable to inspect photo",ex);return null;}
        try(android.database.Cursor cursor=activity.getContentResolver().query(uri,new String[]{android.provider.MediaStore.Images.ImageColumns.ORIENTATION},null,null,null)){
            if(cursor!=null&&cursor.moveToFirst())orientation=((cursor.getInt(0)%360)+360)%360;
        }catch(Exception ignored){}
        if(orientation!=90&&orientation!=180&&orientation!=270)orientation=0;if(width<=0||height<=0)return null;
        int sample=1;while(Math.max((width+sample-1)/sample,(height+sample-1)/sample)>maxEdge&&sample<16)sample*=2;
        int sampledW=(width+sample-1)/sample,sampledH=(height+sample-1)/sample;
        int outputW=(orientation==90||orientation==270)?sampledH:sampledW,outputH=(orientation==90||orientation==270)?sampledW:sampledH;
        return new int[]{width,height,sample,orientation,outputW,outputH};
    }

    /** Open one reusable decoder session; native code consumes it a tile per frame. */
    public static synchronized int[] photoOpen(Activity activity,long mediaId,int maxEdge){
        closePhotoDecoder();
        int[] info=photoInfo(activity,mediaId,maxEdge);if(info==null)return null;
        android.net.Uri uri=android.content.ContentUris.withAppendedId(android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI,mediaId);
        try{
            photoDescriptor=activity.getContentResolver().openFileDescriptor(uri,"r");
            if(photoDescriptor==null)return null;
            photoDecoder=android.graphics.BitmapRegionDecoder.newInstance(photoDescriptor.getFileDescriptor(),false);
            if(photoDecoder==null){closePhotoDecoder();return null;}photoDecoderId=mediaId;return info;
        }catch(Exception ex){android.util.Log.w("VastUi","Unable to open photo decoder",ex);closePhotoDecoder();return null;}
    }

    /** Decode only one source region so an 8K import never needs a second 8K Java bitmap. */
    public static synchronized int[] photoTile(Activity activity,long mediaId,int left,int top,int right,int bottom,int sample){
        android.graphics.Bitmap bitmap=null;
        try{
            if(photoDecoder==null||photoDecoderId!=mediaId){if(photoOpen(activity,mediaId,8192)==null)return null;}
            android.graphics.BitmapFactory.Options options=new android.graphics.BitmapFactory.Options();options.inSampleSize=Math.max(1,sample);options.inPreferredConfig=android.graphics.Bitmap.Config.ARGB_8888;
            bitmap=photoDecoder.decodeRegion(new android.graphics.Rect(left,top,right,bottom),options);if(bitmap==null)return null;int w=bitmap.getWidth(),h=bitmap.getHeight();
            int[] result=new int[2+w*h];result[0]=w;result[1]=h;bitmap.getPixels(result,2,w,0,0,w,h);return result;
        }catch(Exception ex){android.util.Log.w("VastUi","Unable to decode photo region",ex);return null;}
        finally{if(bitmap!=null)bitmap.recycle();}
    }

    public static synchronized void photoClose(){closePhotoDecoder();}

    private static Context themed(Activity activity, int background) {
        boolean light = Color.luminance(background | 0xff000000) > .45;
        return new ContextThemeWrapper(activity, light ?
            android.R.style.Theme_Material_Light_Dialog_Alert : android.R.style.Theme_Material_Dialog_Alert);
    }
    private static int dp(Context c, int n) { return Math.round(n*c.getResources().getDisplayMetrics().density); }
    private static TextView label(Context c,String value,int size) {
        TextView t=new TextView(c);t.setText(value);t.setTextSize(size);
        t.setPadding(0,dp(c,8),0,dp(c,8));return t;
    }
    private static int surface(int background){return VastStyle.light(background)?0xeef6f4ef:0xec30353c;}
    private static int foreground(int background){return VastStyle.ink(background);}
    private static GradientDrawable surfaceDrawable(Context c,int color){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(c,14));d.setStroke(Math.max(1,dp(c,1)/2),(Color.luminance(color|0xff000000)>.45?0x1830363a:0x20e8e4dc));return d;}
    private static final int MOTION_UP=1,MOTION_DOWN=2,MOTION_LEFT=3,MOTION_RIGHT=4;
    private static float motionX(View view,int motion){int d=dp(view.getContext(),18);return motion==MOTION_LEFT?-d:motion==MOTION_RIGHT?d:0;}
    private static float motionY(View view,int motion){int d=dp(view.getContext(),14);return motion==MOTION_UP?-d:motion==MOTION_DOWN?d:0;}
    private static void enter(View view,int motion){
        view.animate().cancel();view.setAlpha(.25f);view.setTranslationX(motionX(view,motion));view.setTranslationY(motionY(view,motion));
        view.animate().alpha(1).translationX(0).translationY(0).setDuration(180).setInterpolator(new android.view.animation.DecelerateInterpolator()).start();
    }
    private static void exit(PopupWindow popup,int motion){
        if(popup==null)return;View view=popup.getContentView();popup.setTouchable(false);view.animate().cancel();
        view.animate().alpha(0).translationX(motionX(view,motion)).translationY(motionY(view,motion)).setDuration(150)
            .setInterpolator(new android.view.animation.AccelerateInterpolator()).withEndAction(()->{try{popup.dismiss();}catch(Exception ignored){}}).start();
    }
    private static void rowAppearance(Button b,JSONObject row,int background){
        b.setContentDescription(row.optString("text"));
        b.setSelected(row.optBoolean("checked"));
        if(row.optBoolean("checked")){b.setTypeface(android.graphics.Typeface.create("sans-serif-medium",0));}
        if(row.optString("kind").equals("danger"))b.setTextColor(Color.luminance(background|0xff000000)>.45?0xffa52b35:0xffff9299);
    }
    private static void performAction(Context c,JSONObject row,int background){
        int id=row.optInt("id");
        if(id==5400&&owner!=null){VastUpdater.check(owner,background,true);return;}
        if(!row.optString("kind").equals("danger")){nativeAction(id);return;}
        AlertDialog confirm=new AlertDialog.Builder(c).setCustomTitle(VastStyle.title(c,row.optString("text")+"?",background))
            .setMessage("This action cannot be undone.").setNegativeButton("Cancel",null)
            .setPositiveButton("Delete",(d,which)->nativeAction(id)).create();
        confirm.show();if(owner!=null)sizeDialog(confirm,owner,420,background);
    }
    private static void styleButton(Button b,int background){VastStyle.button(b,background);}
    private static void dismissChrome(){PopupWindow h=header,i=identity,r=rail,c=contextTools,t=toolPanel;header=identity=rail=contextTools=toolPanel=null;exit(h,MOTION_UP);exit(i,MOTION_UP);exit(r,MOTION_LEFT);exit(c,MOTION_DOWN);exit(t,MOTION_RIGHT);}
    private static LinearLayout actionBar(Context c,JSONArray rows,int background,boolean vertical,boolean compact,boolean shortBar)throws Exception{
        LinearLayout bar=new LinearLayout(c);bar.setOrientation(vertical?LinearLayout.VERTICAL:LinearLayout.HORIZONTAL);bar.setGravity(Gravity.CENTER_VERTICAL);bar.setPadding(dp(c,6),dp(c,6),dp(c,6),dp(c,6));bar.setBackground(VastStyle.glass(c,background,vertical?26:38,false));
        for(int i=0;i<rows.length();i++){JSONObject row=rows.getJSONObject(i);int id=row.getInt("id");String title=row.getString("text");
            if(id==0){TextView label=label(c,title,14);label.setPadding(dp(c,14),dp(c,12),dp(c,14),dp(c,12));label.setTextColor(VastStyle.muted(background));bar.addView(label);continue;}
            Button b=new Button(c);b.setText(compact?"":title);styleButton(b,background);rowAppearance(b,row,background);b.setEnabled(row.optBoolean("enabled",true));b.setOnClickListener(v->performAction(c,row,background));
            android.graphics.drawable.Drawable icon=VastStyle.icon(c,title,background,vertical?21:shortBar?20:23);b.setCompoundDrawablePadding(dp(c,vertical?12:shortBar?2:5));
            if(vertical){b.setGravity(compact?Gravity.CENTER:Gravity.START|Gravity.CENTER_VERTICAL);if(compact){b.setPadding(dp(c,12),dp(c,12),dp(c,12),dp(c,12));b.setCompoundDrawables(null,icon,null,null);}else b.setCompoundDrawables(icon,null,null,null);}
            else{b.setTextSize(12);b.setGravity(Gravity.CENTER);b.setPadding(dp(c,4),dp(c,shortBar?2:8),dp(c,4),dp(c,shortBar?2:6));b.setCompoundDrawables(null,icon,null,null);}
            b.setTooltipText(title);bar.addView(b,new LinearLayout.LayoutParams(vertical?-1:0,dp(c,vertical?50:shortBar?COMPACT_CHROME_DP-12:62),vertical?0:1));}
        return bar;
    }
    private static PopupWindow popup(Activity a,View view,int width,int height,int gravity,int x,int y,int motion){
        PopupWindow p=new PopupWindow(view,width,height,false);p.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));p.setOutsideTouchable(false);p.setTouchable(true);p.setElevation(dp(a,4));p.setClippingEnabled(true);
        p.setAnimationStyle(0);p.setOnDismissListener(()->view.animate().cancel());p.showAtLocation(a.getWindow().getDecorView(),gravity,x,y);enter(view,motion);return p;
    }
    private static PopupWindow syncBar(Activity a,PopupWindow old,JSONArray rows,int background,boolean vertical,int width,int height,int gravity,int x,int y,boolean scrolling,int motion)throws Exception{
        if(rows==null||rows.length()==0){exit(old,motion);return null;}
        String key=rows.toString()+":"+background+":"+width+":"+height;
        if(old!=null&&key.equals(old.getContentView().getTag()))return old;
        Context c=themed(a,background);LinearLayout bar=actionBar(c,rows,background,vertical,width<=dp(c,64),!vertical&&height==dp(c,COMPACT_CHROME_DP));
        if(old!=null){
            View root=old.getContentView();LinearLayout retained=(LinearLayout)(root instanceof ScrollView?((ScrollView)root).getChildAt(0):root);
            retained.removeAllViews();while(bar.getChildCount()>0){View child=bar.getChildAt(0);bar.removeView(child);retained.addView(child);}
            retained.setBackground(bar.getBackground());root.setTag(key);boolean expanded=old.getWidth()!=width;old.update(x,y,width,height);if(expanded){retained.setPivotX(0);retained.setScaleX(.94f);retained.animate().scaleX(1).setDuration(180).setInterpolator(new android.view.animation.DecelerateInterpolator()).start();}return old;
        }
        View root=bar;if(scrolling){ScrollView scroll=new ScrollView(c);scroll.setVerticalScrollBarEnabled(false);scroll.setFillViewport(false);scroll.addView(bar);root=scroll;}
        root.setTag(key);return popup(a,root,width,height,gravity,x,y,motion);
    }
    public static void showChrome(Activity a,String json,int background){
        requestHighRefresh(a);
        VastUpdater.maybeCheck(a,background);
        if(json.equals(chromeKey)&&background==chromeBackground)return;chromeKey=json;chromeBackground=background;
        if(json.isEmpty()){dismissChrome();return;}
        try{
            JSONObject state=new JSONObject(json);Context c=themed(a,background);owner=a;
            if(state.optBoolean("hidden")){dismissChrome();return;}
            // Keep project context, not canvas branding. This View survives
            // pen DOWN/UP and undo-state updates; its entrance never restarts.
            if(identity==null){TextView project=label(c,state.getString("project"),12);project.setPadding(dp(c,8),0,dp(c,8),0);project.setTextColor(foreground(background));identity=popup(a,project,dp(c,160),dp(c,32),Gravity.TOP|Gravity.LEFT,dp(c,16),dp(c,16),MOTION_UP);}
            else{TextView project=(TextView)identity.getContentView();project.setText(state.getString("project"));project.setTextColor(foreground(background));}
            boolean zen=state.optBoolean("zen");
            int screenWidth=a.getResources().getDisplayMetrics().widthPixels;
            header=syncBar(a,header,zen?null:state.optJSONArray("header"),background,false,Math.min(dp(c,420),screenWidth-dp(c,32)),dp(c,COMPACT_CHROME_DP),Gravity.TOP|Gravity.RIGHT,dp(c,16),dp(c,12),false,MOTION_UP);
            JSONArray context=state.optJSONArray("context");
            contextTools=syncBar(a,contextTools,zen?null:context,background,false,Math.min(dp(c,12+84*(context==null?0:context.length())),screenWidth-dp(c,32)),dp(c,74),Gravity.BOTTOM|Gravity.CENTER_HORIZONTAL,0,dp(c,24),false,MOTION_DOWN);
            toolPanel=syncBar(a,toolPanel,zen?null:state.optJSONArray("tool"),background,true,Math.min(dp(c,248),screenWidth-dp(c,32)),-2,Gravity.TOP|Gravity.RIGHT,dp(c,16),dp(c,12+COMPACT_CHROME_DP+16),false,MOTION_RIGHT);
            boolean expanded=state.optBoolean("rail");JSONArray tray=new JSONArray(state.optJSONArray("tools").toString());JSONObject handle=new JSONObject();handle.put("id",5300);handle.put("text",expanded?"Collapse":"Expand");tray.put(handle);
            rail=syncBar(a,rail,zen?null:tray,background,true,dp(c,expanded?152:COMPACT_CHROME_DP),Math.min(dp(c,462),a.getResources().getDisplayMetrics().heightPixels-dp(c,170)),Gravity.CENTER_VERTICAL|Gravity.LEFT,dp(c,12),0,true,MOTION_LEFT);
        }catch(Exception ex){android.util.Log.e("VastUi","Unable to show canvas controls",ex);dismissChrome();}
    }
    private static PopupWindow colorPopup;
    private static LinearLayout colorBody;
    private static TextView colorReadout;
    private static Button colorPreview;
    private static android.widget.SeekBar valueBar;
    private static ColorWheel colorWheel;
    private static final float[] colorHsv={0,0,1};
    private static int colorCloseId,colorBackground;
    private static boolean colorClosing,colorUpdating;
    private static void colorChanged(){
        int color=Color.HSVToColor(colorHsv);
        if(colorPreview!=null)colorPreview.setBackground(surfaceDrawable(colorPreview.getContext(),color));
        if(colorReadout!=null)colorReadout.setText(String.format(java.util.Locale.ROOT,"#%06X  ·  %d%%",color&0xffffff,Math.round(colorHsv[2]*100)));
        if(colorWheel!=null)colorWheel.invalidate();
        if(!colorUpdating)nativeColor(color);
    }
    private static void setPickerColor(int color){
        colorUpdating=true;Color.colorToHSV(color|0xff000000,colorHsv);if(valueBar!=null)valueBar.setProgress(Math.round(colorHsv[2]*1000));colorChanged();colorUpdating=false;
    }
    private static void closeColor(boolean notify,boolean immediate){
        if(colorPopup==null)return;
        PopupWindow closingPopup=colorPopup;View body=colorBody!=null?colorBody:closingPopup.getContentView();
        if(colorClosing&&!immediate)return;colorClosing=true;body.animate().cancel();
        // Release the input window immediately; fading pixels cannot block ink.
        closingPopup.setTouchable(false);closingPopup.setFocusable(false);closingPopup.update();
        Runnable finish=()->{closingPopup.setOnDismissListener(null);closingPopup.dismiss();if(colorPopup==closingPopup){colorPopup=null;colorBody=null;colorWheel=null;valueBar=null;colorPreview=null;colorReadout=null;colorClosing=false;}};
        if(immediate)finish.run();else body.animate().alpha(0).translationY(dp(body.getContext(),12)).scaleX(.96f).scaleY(.96f).setDuration(140).withEndAction(finish).start();
        if(notify)nativeAction(colorCloseId);
    }
    private static final class ColorWheel extends View{
        final android.graphics.Paint paint=new android.graphics.Paint(android.graphics.Paint.ANTI_ALIAS_FLAG);
        android.graphics.Shader shader;float radius,cx,cy;boolean tracking;
        ColorWheel(Context c){super(c);setFocusable(true);setClickable(true);setContentDescription("Hue and saturation. Arrow left and right change hue; up and down change saturation.");}
        @Override protected void onSizeChanged(int w,int h,int ow,int oh){
            cx=w*.5f;cy=h*.5f;radius=Math.min(w,h)*.5f-dp(getContext(),12);
            android.graphics.SweepGradient hue=new android.graphics.SweepGradient(cx,cy,new int[]{Color.RED,Color.MAGENTA,Color.BLUE,Color.CYAN,Color.GREEN,Color.YELLOW,Color.RED},null);
            android.graphics.RadialGradient sat=new android.graphics.RadialGradient(cx,cy,radius,Color.WHITE,0x00ffffff,android.graphics.Shader.TileMode.CLAMP);
            shader=new android.graphics.ComposeShader(hue,sat,android.graphics.PorterDuff.Mode.SRC_OVER);
        }
        @Override protected void onDraw(Canvas c){
            paint.setStyle(android.graphics.Paint.Style.FILL);paint.setShader(shader);c.drawCircle(cx,cy,radius,paint);paint.setShader(null);
            double angle=-colorHsv[0]*Math.PI/180;float r=radius*colorHsv[1],x=cx+(float)Math.cos(angle)*r,y=cy+(float)Math.sin(angle)*r;
            paint.setStyle(android.graphics.Paint.Style.STROKE);paint.setStrokeWidth(dp(getContext(),3));paint.setColor(0xb0000000);c.drawCircle(x,y,dp(getContext(),7),paint);
            paint.setStrokeWidth(dp(getContext(),2));paint.setColor(Color.WHITE);c.drawCircle(x,y,dp(getContext(),6),paint);
        }
        @Override public boolean onTouchEvent(android.view.MotionEvent e){
            int action=e.getActionMasked();if(action==android.view.MotionEvent.ACTION_DOWN){tracking=true;getParent().requestDisallowInterceptTouchEvent(true);}
            if(!tracking)return false;
            if(action==android.view.MotionEvent.ACTION_CANCEL){tracking=false;return true;}
            float dx=e.getX()-cx,dy=e.getY()-cy;colorHsv[0]=(float)((Math.atan2(-dy,dx)*180/Math.PI+360)%360);colorHsv[1]=Math.min(1,(float)Math.hypot(dx,dy)/Math.max(1,radius));colorChanged();
            if(action==android.view.MotionEvent.ACTION_UP){tracking=false;performClick();}return true;
        }
        @Override public boolean performClick(){super.performClick();return true;}
        @Override public boolean onKeyDown(int key,android.view.KeyEvent e){
            if(key==android.view.KeyEvent.KEYCODE_DPAD_LEFT)colorHsv[0]=(colorHsv[0]+355)%360;
            else if(key==android.view.KeyEvent.KEYCODE_DPAD_RIGHT)colorHsv[0]=(colorHsv[0]+5)%360;
            else if(key==android.view.KeyEvent.KEYCODE_DPAD_UP)colorHsv[1]=Math.min(1,colorHsv[1]+.025f);
            else if(key==android.view.KeyEvent.KEYCODE_DPAD_DOWN)colorHsv[1]=Math.max(0,colorHsv[1]-.025f);
            else return super.onKeyDown(key,e);colorChanged();return true;
        }
        @Override public boolean performAccessibilityAction(int action,android.os.Bundle args){
            if(action==android.view.accessibility.AccessibilityNodeInfo.ACTION_SCROLL_FORWARD||action==android.view.accessibility.AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD){colorHsv[0]=(colorHsv[0]+(action==4096?5:355))%360;colorChanged();announceForAccessibility("Hue "+Math.round(colorHsv[0])+" degrees");return true;}return super.performAccessibilityAction(action,args);
        }
        @Override public void onInitializeAccessibilityNodeInfo(android.view.accessibility.AccessibilityNodeInfo info){super.onInitializeAccessibilityNodeInfo(info);info.addAction(4096);info.addAction(8192);}
    }
    public static void showColorPicker(Activity a,String json,int background){
        try{
            JSONObject state=new JSONObject(json);
            if(!state.optBoolean("open")){closeColor(false,false);return;}
            colorCloseId=state.getInt("close");colorBackground=background;
            if(colorPopup!=null){if((Color.HSVToColor(colorHsv)&0xffffff)!=(state.getInt("color")&0xffffff))setPickerColor(state.getInt("color"));if(colorClosing){colorClosing=false;colorBody.animate().cancel();colorPopup.setTouchable(true);colorPopup.setFocusable(true);colorPopup.update();colorBody.animate().alpha(1).translationY(0).scaleX(1).scaleY(1).setDuration(180).start();}return;}
            Context c=themed(a,background);colorBody=new LinearLayout(c);colorBody.setOrientation(LinearLayout.VERTICAL);colorBody.setPadding(dp(c,20),dp(c,14),dp(c,20),dp(c,20));colorBody.setBackground(VastStyle.glass(c,background,28,true));
            LinearLayout heading=new LinearLayout(c);heading.setGravity(Gravity.CENTER_VERTICAL);TextView title=label(c,"Color",18);title.setTextColor(foreground(background));heading.addView(title,new LinearLayout.LayoutParams(0,dp(c,48),1));
            Button done=new Button(c);done.setText("Done");styleButton(done,background);done.setOnClickListener(v->closeColor(true,false));heading.addView(done,new LinearLayout.LayoutParams(dp(c,76),dp(c,48)));colorBody.addView(heading);
            colorWheel=new ColorWheel(c);colorBody.addView(colorWheel,new LinearLayout.LayoutParams(-1,dp(c,240)));
            TextView value=label(c,"Brightness",12);value.setTextColor(foreground(background));colorBody.addView(value);
            valueBar=new android.widget.SeekBar(c);valueBar.setMax(1000);valueBar.setContentDescription("Brightness");valueBar.setProgressTintList(ColorStateList.valueOf(0xff9eb6bc));valueBar.setThumbTintList(ColorStateList.valueOf(0xff9eb6bc));colorBody.addView(valueBar,new LinearLayout.LayoutParams(-1,dp(c,40)));
            LinearLayout current=new LinearLayout(c);current.setGravity(Gravity.CENTER_VERTICAL);colorPreview=new Button(c);colorPreview.setContentDescription("Current color");colorPreview.setEnabled(false);current.addView(colorPreview,new LinearLayout.LayoutParams(dp(c,48),dp(c,40)));
            colorReadout=label(c,"",13);colorReadout.setTextColor(foreground(background));colorReadout.setPadding(dp(c,12),0,0,0);current.addView(colorReadout);colorBody.addView(current);
            LinearLayout recents=new LinearLayout(c);JSONArray colors=state.optJSONArray("recent");int count=colors==null?0:Math.min(5,colors.length());
            for(int i=-1;i<count;i++){int color=i<0?state.getInt("previous"):colors.getInt(i);Button swatch=new Button(c);swatch.setMinWidth(0);swatch.setContentDescription(i<0?"Previous color":String.format(java.util.Locale.ROOT,"Recent color #%06X",color&0xffffff));swatch.setBackground(surfaceDrawable(c,color|0xff000000));swatch.setOnClickListener(v->{setPickerColor(color);nativeColor(color);});LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,dp(c,44),1);lp.setMargins(dp(c,3),dp(c,12),dp(c,3),0);recents.addView(swatch,lp);}colorBody.addView(recents);
            setPickerColor(state.getInt("color"));valueBar.setOnSeekBarChangeListener(new android.widget.SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(android.widget.SeekBar b,int v,boolean user){if(user){colorHsv[2]=v/1000f;colorChanged();}}public void onStartTrackingTouch(android.widget.SeekBar b){}public void onStopTrackingTouch(android.widget.SeekBar b){}});
            Rect available=new Rect();a.getWindow().getDecorView().getWindowVisibleDisplayFrame(available);
            int width=Math.min(dp(c,316),available.width()-dp(c,24));
            BoundedScrollView scroll=new BoundedScrollView(c,Math.max(dp(c,160),available.height()-dp(c,100)));scroll.setFillViewport(false);scroll.addView(colorBody);
            colorPopup=new PopupWindow(scroll,width,-2,true);colorPopup.setInputMethodMode(PopupWindow.INPUT_METHOD_NOT_NEEDED);colorPopup.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);colorPopup.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));colorPopup.setElevation(dp(c,3));colorPopup.setClippingEnabled(true);colorPopup.setOutsideTouchable(true);colorPopup.setAnimationStyle(0);
            colorPopup.setTouchInterceptor((v,e)->{if(e.getActionMasked()==android.view.MotionEvent.ACTION_OUTSIDE){closeColor(true,false);return true;}return false;});
            scroll.setFocusableInTouchMode(true);scroll.setOnKeyListener((v,key,event)->{if(key==android.view.KeyEvent.KEYCODE_BACK){if(event.getAction()==android.view.KeyEvent.ACTION_UP)closeColor(true,false);return true;}return false;});
            colorPopup.setOnDismissListener(()->{boolean notify=!colorClosing;colorPopup=null;colorBody=null;colorWheel=null;valueBar=null;colorPreview=null;colorReadout=null;colorClosing=false;if(notify)nativeAction(colorCloseId);});
            int bottomInset=Math.max(0,a.getResources().getDisplayMetrics().heightPixels-available.bottom);
            colorPopup.showAtLocation(a.getWindow().getDecorView(),Gravity.BOTTOM|Gravity.RIGHT,dp(c,14),bottomInset+dp(c,66));
            scroll.requestFocus();colorBody.setPivotX(width);colorBody.setPivotY(dp(c,500));colorBody.setAlpha(0);colorBody.setTranslationY(dp(c,22));colorBody.setScaleX(.94f);colorBody.setScaleY(.94f);colorBody.animate().alpha(1).translationY(0).scaleX(1).scaleY(1).setDuration(190).setInterpolator(new android.view.animation.DecelerateInterpolator()).start();
        }catch(Exception ex){android.util.Log.e("VastUi","Unable to show color picker",ex);closeColor(false,true);}
    }
    private static void sizeDialog(AlertDialog d, Activity a, int widthDp,int background) {
        Window w=d.getWindow(); if(w==null)return;
        Rect bounds=new Rect();a.getWindow().getDecorView().getWindowVisibleDisplayFrame(bounds);
        int width=Math.min(dp(a,widthDp),Math.max(dp(a,240),bounds.width()-dp(a,32)));
        w.setLayout(width,WindowManager.LayoutParams.WRAP_CONTENT);
        w.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        w.setDimAmount(.16f);
        if(preferredRefresh>60){WindowManager.LayoutParams lp=w.getAttributes();lp.preferredRefreshRate=preferredRefresh;w.setAttributes(lp);}
        w.setBackgroundDrawable(VastStyle.glass(a,background,30,true));
        w.getDecorView().setElevation(dp(a,8));
        for(int id:new int[]{AlertDialog.BUTTON_POSITIVE,AlertDialog.BUTTON_NEGATIVE,AlertDialog.BUTTON_NEUTRAL}){Button b=d.getButton(id);if(b!=null){styleButton(b,background);b.setSelected(id==AlertDialog.BUTTON_POSITIVE);}}
        w.setWindowAnimations(0);enter(w.getDecorView(),MOTION_DOWN);
    }
    private static void closePanel() {
        if(panelDialog!=null){panelDialog.getWindow().getDecorView().animate().cancel();panelDialog.setOnCancelListener(null);panelDialog.dismiss();panelDialog=null;}
        panelKey="";panelTitle="";panelScroll=null;panelContainer=null;panelSection=-1;
    }
    public static void closeEditor() {
        closing=true;
        if(editor!=null){InputMethodManager imm=(InputMethodManager)editor.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            if(imm!=null)imm.hideSoftInputFromWindow(editor.getWindowToken(),0);}
        if(editorDialog!=null){editorDialog.getWindow().getDecorView().animate().cancel();editorDialog.setOnCancelListener(null);editorDialog.dismiss();}
        editorDialog=null;editor=null;editorMode=0;closing=false;
    }
    public static String editorText(){return editor==null?"":editor.getText().toString();}
    public static boolean editorShowing(){return editorDialog!=null&&editorDialog.isShowing();}
    /** Identical measurement and raster layout for the non-editable canvas object. */
    public static int[] rasterText(String text) {
        TextPaint paint=new TextPaint(android.graphics.Paint.ANTI_ALIAS_FLAG);
        paint.setTextSize(18);paint.setColor(Color.WHITE);
        int width=Math.max(24,Math.min(480,(int)Math.ceil(Layout.getDesiredWidth(text,paint))));
        StaticLayout layout=StaticLayout.Builder.obtain(text,0,text.length(),paint,width)
            .setIncludePad(true).setLineSpacing(4,1).setBreakStrategy(Layout.BREAK_STRATEGY_SIMPLE).build();
        int height=Math.max(24,layout.getHeight());
        Bitmap bitmap=Bitmap.createBitmap(width,height,Bitmap.Config.ARGB_8888);
        layout.draw(new Canvas(bitmap));
        int[] result=new int[2+width*height];result[0]=width;result[1]=height;
        bitmap.getPixels(result,2,width,0,0,width,height);bitmap.recycle();return result;
    }
    public static boolean showEditor(Activity a,String initial,int mode,int byteLimit,int unit,int background) {
        closeEditor();closePanel();owner=a;editorMode=mode;
        Context c=themed(a,background);
        LinearLayout body=new LinearLayout(c);body.setOrientation(LinearLayout.VERTICAL);
        body.setPadding(dp(c,24),dp(c,8),dp(c,24),dp(c,8));
        editor=new EditText(c);
        VastStyle.field(editor,background);
        editor.setTextSize(18);editor.setSelectAllOnFocus(true);
        editor.setInputType(mode==6?InputType.TYPE_CLASS_NUMBER|InputType.TYPE_NUMBER_FLAG_DECIMAL:
            InputType.TYPE_CLASS_TEXT|(mode==1?InputType.TYPE_TEXT_FLAG_MULTI_LINE|InputType.TYPE_TEXT_FLAG_CAP_SENTENCES:0));
        editor.setSingleLine(mode!=1);editor.setGravity(Gravity.TOP|Gravity.START);
        editor.setMinLines(mode==1?3:1);editor.setMaxLines(mode==1?8:1);
        editor.setHorizontallyScrolling(mode!=1);
        editor.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI|(mode==1?EditorInfo.IME_FLAG_NO_ENTER_ACTION:
            mode==3?EditorInfo.IME_ACTION_SEARCH:EditorInfo.IME_ACTION_DONE));
        String[] titles={"","Edit text","Rename frame","Search canvas","Rename project","Rename place","Calibrate dimension"};
        editor.setHint(mode==3?"Search notes, places and handwriting":mode==6?"Real length":mode==1?"Write a note":"Name");
        editor.setContentDescription(titles[mode]);
        // Respect the existing UTF-8 document capacity without breaking surrogate pairs or IME spans.
        editor.setFilters(new InputFilter[]{(source,start,end,dest,dstart,dend)->{
            int remaining=byteLimit-(dest.subSequence(0,dstart).toString()+dest.subSequence(dend,dest.length())).getBytes(StandardCharsets.UTF_8).length;
            int accepted=start;
            while(accepted<end){int cp=Character.codePointAt(source,accepted);int chars=Character.charCount(cp);
                int bytes=cp<0x80?1:cp<0x800?2:cp<0x10000?3:4;
                if(bytes>remaining||accepted+chars>end)break;remaining-=bytes;accepted+=chars;}
            if(accepted==end)return null;
            return source.subSequence(start,accepted);
        }});
        editor.setText(initial);body.addView(editor,new LinearLayout.LayoutParams(-1,-2));
        if(mode==6){Spinner units=new Spinner(c);
            units.setAdapter(new ArrayAdapter<String>(c,android.R.layout.simple_spinner_dropdown_item,new String[]{"mm","cm","m","in","ft"}){
                private View styled(int position,View reused,android.view.ViewGroup parent){TextView t=reused instanceof TextView?(TextView)reused:new TextView(c);t.setText(getItem(position));t.setTextSize(16);t.setTextColor(foreground(background));t.setPadding(dp(c,18),dp(c,14),dp(c,18),dp(c,14));t.setMinHeight(dp(c,48));return t;}
                @Override public View getView(int position,View reused,android.view.ViewGroup parent){return styled(position,reused,parent);}
                @Override public View getDropDownView(int position,View reused,android.view.ViewGroup parent){return styled(position,reused,parent);}
            });
            units.setBackground(VastStyle.interaction(c,background,18));units.setPopupBackgroundDrawable(VastStyle.glass(c,background,18,true));
            units.setSelection(unit);units.setContentDescription("Length unit");
            units.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener(){
                public void onNothingSelected(android.widget.AdapterView<?> parent){}
                public void onItemSelected(android.widget.AdapterView<?> parent,View view,int position,long id){nativeUnit(position);}
            });body.addView(units);}
        ScrollView scroll=new ScrollView(c);scroll.setFillViewport(true);scroll.addView(body);
        editorDialog=new AlertDialog.Builder(c).setCustomTitle(VastStyle.title(c,titles[mode],background)).setView(scroll)
            .setNegativeButton("Cancel",(d,which)->nativeCancel())
            .setPositiveButton(mode==3?"Search":mode==6?"Apply":"Save",null).create();
        editorDialog.setCanceledOnTouchOutside(false);
        editorDialog.setOnCancelListener(d->{if(!closing)nativeCancel();});
        editorDialog.show();sizeDialog(editorDialog,a,560,background);
        Runnable commit=()->{
            if(editor==null)return;
            if(mode==6){try{String number=editorText().replace(',','.');if(!number.matches("(?:[0-9]+(?:\\.[0-9]*)?|\\.[0-9]+)"))throw new NumberFormatException();double value=Double.parseDouble(number);
                if(!Double.isFinite(value)||value<=.000001||value>1e20)throw new NumberFormatException();
            }catch(NumberFormatException ex){editor.setError("Enter a positive length");return;}}
            nativeText(mode==6?editorText().replace(',','.'):editorText());nativeAction(mode==6?302:401);
        };
        editorDialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->commit.run());
        editor.setOnEditorActionListener((view,action,event)->{if(mode!=1&&(action==EditorInfo.IME_ACTION_DONE||action==EditorInfo.IME_ACTION_SEARCH)){
            commit.run();return true;}return false;});
        editor.addTextChangedListener(new TextWatcher(){public void beforeTextChanged(CharSequence s,int start,int count,int after){}
            public void onTextChanged(CharSequence s,int start,int before,int count){if(!closing)nativeText(s.toString());}
            public void afterTextChanged(Editable e){}
        });
        editor.requestFocus();editor.selectAll();
        editorDialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE|WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        editor.post(()->{if(editor!=null){InputMethodManager imm=(InputMethodManager)a.getSystemService(Context.INPUT_METHOD_SERVICE);if(imm!=null)imm.showSoftInput(editor,InputMethodManager.SHOW_IMPLICIT);}});
        return true;
    }

    /** A small data-driven View adapter, fed only when the native UI state changes. */
    public static void showPanel(Activity a,String json,int background) {
        if(editorShowing())return;
        if(json.isEmpty()){closePanel();return;}
        if(json.equals(panelKey)&&panelDialog!=null&&panelDialog.isShowing())return;
        int oldScroll=0;
        if(panelDialog!=null){View old=panelDialog.findViewById(9001);if(old!=null)oldScroll=old.getScrollY();}
        try{
            JSONObject model=new JSONObject(json);Context c=themed(a,background);
            boolean reuse=panelDialog!=null&&panelDialog.isShowing()&&panelTitle.equals(model.getString("title"))&&panelBackground==background;
            if(!reuse){closePanel();oldScroll=0;}owner=a;panelKey=json;panelTitle=model.getString("title");panelBackground=background;
            LinearLayout body=new LinearLayout(c);body.setOrientation(LinearLayout.VERTICAL);
            body.setPadding(dp(c,20),dp(c,8),dp(c,20),dp(c,8));
            JSONArray rows=model.getJSONArray("rows");
            boolean narrow=a.getResources().getDisplayMetrics().widthPixels<dp(c,640);
            LinearLayout navigation=new LinearLayout(c);navigation.setOrientation(LinearLayout.VERTICAL);navigation.setPadding(dp(c,8),dp(c,8),dp(c,8),dp(c,8));navigation.setBackground(VastStyle.rounded(c,VastStyle.alpha(foreground(background),7),22));
            int nextSection=-1;for(int i=0;i<rows.length();i++){JSONObject row=rows.getJSONObject(i);if(row.optString("kind").equals("nav")&&row.optBoolean("checked"))nextSection=row.optInt("id");}
            if(reuse&&panelSection!=nextSection){sectionScroll.put(panelSection,oldScroll);oldScroll=sectionScroll.containsKey(nextSection)?sectionScroll.get(nextSection):0;}
            boolean sectionChanged=reuse&&panelSection!=nextSection;panelSection=nextSection;
            LinearLayout photoRow=null,swatchRow=null,group=body;int photoCount=0,swatchCount=0;
            for(int i=0;i<rows.length();i++){
                JSONObject row=rows.getJSONObject(i);String kind=row.optString("kind","button");String title=row.optString("text");
                int action=row.optInt("id");boolean enabled=row.optBoolean("enabled",true);
                if(kind.equals("swatch")){if(swatchCount++%6==0){swatchRow=new LinearLayout(c);body.addView(swatchRow,new LinearLayout.LayoutParams(-1,dp(c,52)));}Button swatch=new Button(c);swatch.setContentDescription("Color "+title);swatch.setBackgroundTintList(ColorStateList.valueOf(Color.parseColor(title)));swatch.setMinWidth(0);swatch.setOnClickListener(v->nativeAction(action));LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,dp(c,48),1);lp.setMargins(dp(c,3),dp(c,2),dp(c,3),dp(c,2));swatchRow.addView(swatch,lp);continue;}
                if(kind.equals("nav")){Button b=new Button(c);b.setText(title);styleButton(b,background);b.setTextSize(13);rowAppearance(b,row,background);b.setGravity(Gravity.START|Gravity.CENTER_VERTICAL);b.setCompoundDrawables(VastStyle.icon(c,title,background,19),null,null,null);b.setCompoundDrawablePadding(dp(c,10));b.setOnClickListener(v->nativeAction(action));LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,dp(c,58));lp.bottomMargin=dp(c,4);navigation.addView(b,lp);continue;}
                if(kind.equals("layer")){LinearLayout line=new LinearLayout(c);line.setGravity(Gravity.CENTER_VERTICAL);
                    Switch visible=new Switch(c);visible.setText(title);VastStyle.toggle(visible,background);visible.setContentDescription(title+" visibility");visible.setChecked(row.optBoolean("checked"));visible.setMinHeight(dp(c,52));visible.setOnCheckedChangeListener((v,on)->nativeAction(action));line.addView(visible,new LinearLayout.LayoutParams(0,dp(c,52),1));
                    int lockId=row.optInt("lockId");Button lock=new Button(c);boolean locked=row.optBoolean("locked");lock.setText(locked?"Locked":"Unlocked");lock.setContentDescription(title+(locked?" locked":" unlocked"));styleButton(lock,background);lock.setOnClickListener(v->nativeAction(lockId));LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(dp(c,110),dp(c,48));lp.setMargins(dp(c,24),0,0,0);line.addView(lock,lp);body.addView(line);continue;}
                if(kind.equals("photo")){
                    if(photoCount%3==0){photoRow=new LinearLayout(c);body.addView(photoRow,new LinearLayout.LayoutParams(-1,dp(c,160)));}
                    android.widget.ImageButton photo=new android.widget.ImageButton(c);photo.setContentDescription(title);photo.setScaleType(android.widget.ImageView.ScaleType.CENTER_INSIDE);photo.setBackgroundTintList(ColorStateList.valueOf(surface(background)));
                    int[] data=nativeThumbnail(photoCount++);if(data!=null&&data.length==2+data[0]*data[1])photo.setImageBitmap(Bitmap.createBitmap(data,2,data[0],data[0],data[1],Bitmap.Config.ARGB_8888));
                    photo.setOnClickListener(v->nativeAction(action));LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,-1,1);lp.setMargins(dp(c,4),dp(c,4),dp(c,4),dp(c,4));photoRow.addView(photo,lp);continue;
                }
                if(kind.equals("text")||kind.equals("heading")){TextView t=label(c,title,kind.equals("heading")?14:14);t.setTextColor(VastStyle.muted(background));if(kind.equals("heading")){t.setTypeface(android.graphics.Typeface.create("sans-serif-medium",0));t.setLetterSpacing(.035f);t.setPadding(dp(c,12),dp(c,18),0,dp(c,10));body.addView(t);group=new LinearLayout(c);group.setOrientation(LinearLayout.VERTICAL);group.setPadding(dp(c,10),dp(c,4),dp(c,10),dp(c,4));group.setBackground(VastStyle.rounded(c,VastStyle.alpha(foreground(background),7),20));body.addView(group,new LinearLayout.LayoutParams(-1,-2));}else{t.setLineSpacing(dp(c,4),1);t.setPadding(dp(c,12),dp(c,12),dp(c,12),dp(c,12));group.addView(t);}continue;}
                if(kind.equals("toggle")){Switch sw=new Switch(c);sw.setText(title);VastStyle.toggle(sw,background);sw.setMinHeight(dp(c,56));
                    sw.setChecked(row.optBoolean("checked"));sw.setEnabled(enabled);sw.setPadding(dp(c,12),dp(c,8),dp(c,12),dp(c,8));
                    sw.setOnCheckedChangeListener((button,checked)->nativeAction(action));group.addView(sw);continue;}
                if(kind.equals("step")){LinearLayout line=new LinearLayout(c);line.setGravity(Gravity.CENTER_VERTICAL);
                    line.setPadding(dp(c,12),dp(c,4),dp(c,4),dp(c,4));TextView t=label(c,title,15);t.setTextColor(foreground(background));line.addView(t,new LinearLayout.LayoutParams(0,-2,1));
                    TextView value=label(c,row.optString("value"),13);value.setTextColor(VastStyle.muted(background));value.setGravity(Gravity.END|Gravity.CENTER_VERTICAL);value.setPadding(dp(c,8),0,dp(c,12),0);line.addView(value,new LinearLayout.LayoutParams(dp(c,96),-2));
                    int minus=row.getInt("minus"),plus=row.getInt("plus");
                    Button back=new Button(c);back.setText("−");back.setContentDescription("Decrease "+title);back.setMinWidth(dp(c,48));back.setOnClickListener(v->nativeAction(minus));
                    Button next=new Button(c);next.setText("+");next.setContentDescription("Increase "+title);next.setMinWidth(dp(c,48));next.setOnClickListener(v->nativeAction(plus));
                    styleButton(back,background);styleButton(next,background);back.setBackground(VastStyle.interaction(c,background,24));next.setBackground(VastStyle.interaction(c,background,24));line.addView(back,new LinearLayout.LayoutParams(dp(c,48),dp(c,48)));line.addView(next,new LinearLayout.LayoutParams(dp(c,48),dp(c,48)));group.addView(line);continue;}
                Button button=new Button(c);button.setText(title);button.setAllCaps(false);button.setTextSize(16);button.setMinHeight(dp(c,48));button.setEnabled(enabled);
                styleButton(button,background);rowAppearance(button,row,background);button.setGravity(Gravity.START|Gravity.CENTER_VERTICAL);button.setMinHeight(dp(c,54));button.setCompoundDrawables(VastStyle.icon(c,title,background,20),null,null,null);button.setCompoundDrawablePadding(dp(c,14));button.setMaxLines(2);button.setEllipsize(android.text.TextUtils.TruncateAt.END);button.setOnClickListener(v->performAction(c,row,background));LinearLayout.LayoutParams blp=new LinearLayout.LayoutParams(-1,-2);blp.setMargins(0,dp(c,2),0,dp(c,2));group.addView(button,blp);
            }
            int maxHeight=(int)(a.getResources().getDisplayMetrics().heightPixels*.65f);
            ScrollView scroll=reuse?panelScroll:new BoundedScrollView(c,Math.min(dp(c,520),maxHeight));scroll.setId(9001);scroll.removeAllViews();scroll.addView(body);panelScroll=scroll;
            // Scroll content, not the dialog's fixed title and Close action.
            if(!reuse)panelContainer=new LinearLayout(c);
            panelContainer.setMinimumHeight(panelTitle.equals("Settings")?Math.min(dp(c,400),maxHeight):0);
            panelContainer.removeAllViews();panelContainer.setOrientation(narrow?LinearLayout.VERTICAL:LinearLayout.HORIZONTAL);
            if(navigation.getChildCount()>0){
                if(narrow){navigation.setOrientation(LinearLayout.HORIZONTAL);for(int i=0;i<navigation.getChildCount();i++)navigation.getChildAt(i).setLayoutParams(new LinearLayout.LayoutParams(dp(c,168),dp(c,58)));android.widget.HorizontalScrollView navScroll=new android.widget.HorizontalScrollView(c);navScroll.setHorizontalScrollBarEnabled(false);navScroll.addView(navigation);panelContainer.addView(navScroll,new LinearLayout.LayoutParams(-1,dp(c,78)));}
                else{LinearLayout.LayoutParams navLp=new LinearLayout.LayoutParams(dp(c,188),-2);navLp.setMargins(dp(c,16),dp(c,8),0,dp(c,12));panelContainer.addView(navigation,navLp);}
            }
            panelContainer.addView(scroll,narrow?new LinearLayout.LayoutParams(-1,-2):new LinearLayout.LayoutParams(0,-2,1));
            if(!reuse){
            int close=model.getInt("close");
            panelDialog=new AlertDialog.Builder(c).setCustomTitle(VastStyle.title(c,model.getString("title"),background)).setView(panelContainer)
                .setNegativeButton("Close",(d,which)->nativeAction(close)).create();
            panelDialog.setOnCancelListener(d->nativeAction(close));panelDialog.show();sizeDialog(panelDialog,a,navigation.getChildCount()>0?800:560,background);}
            final int restore=oldScroll;scroll.post(()->scroll.scrollTo(0,restore));
            if(sectionChanged)enter(body,MOTION_RIGHT);
        }catch(Exception ex){android.util.Log.e("VastUi","Unable to show panel",ex);closePanel();}
    }
    public static void destroy(){closeColor(false,true);closeEditor();closePanel();dismissChrome();chromeKey="";owner=refreshOwner=null;}
}
