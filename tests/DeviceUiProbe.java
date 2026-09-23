import android.app.UiAutomation;
import android.os.Bundle;
import android.os.HandlerThread;
import android.view.accessibility.AccessibilityNodeInfo;
import java.lang.reflect.Constructor;
/** Shell-only native View QA. No document access and no app test backdoor. */
public final class DeviceUiProbe {
    static AccessibilityNodeInfo lookup(UiAutomation ui,String query){
        AccessibilityNodeInfo node=find(ui.getRootInActiveWindow(),query);
        if(node==null)for(android.view.accessibility.AccessibilityWindowInfo window:ui.getWindows()){node=find(window.getRoot(),query);if(node!=null)break;}
        return node;
    }
    static void click(UiAutomation ui,String query)throws Exception{
        long until=android.os.SystemClock.uptimeMillis()+1800;AccessibilityNodeInfo node;
        do{node=lookup(ui,query);if(node!=null&&node.isVisibleToUser()&&node.performAction(AccessibilityNodeInfo.ACTION_CLICK))return;Thread.sleep(10);}while(android.os.SystemClock.uptimeMillis()<until);
        throw new IllegalStateException("Rapid click failed: "+query);
    }
    static void tap(UiAutomation ui,float x,float y){tap(ui,x,y,android.view.MotionEvent.TOOL_TYPE_FINGER);}
    static void tap(UiAutomation ui,float x,float y,int tool){
        long t=android.os.SystemClock.uptimeMillis();
        android.view.MotionEvent.PointerProperties p=new android.view.MotionEvent.PointerProperties();p.id=0;p.toolType=tool;
        android.view.MotionEvent.PointerCoords c=new android.view.MotionEvent.PointerCoords();c.x=x;c.y=y;c.pressure=1;
        for(int action:new int[]{0,1}){android.view.MotionEvent e=android.view.MotionEvent.obtain(t,android.os.SystemClock.uptimeMillis(),action,1,new android.view.MotionEvent.PointerProperties[]{p},new android.view.MotionEvent.PointerCoords[]{c},0,0,1,1,0,0,tool==android.view.MotionEvent.TOOL_TYPE_STYLUS?0x4002:0x1002,0);if(!ui.injectInputEvent(e,true))throw new IllegalStateException("Tap rejected");e.recycle();}
    }
    static AccessibilityNodeInfo find(AccessibilityNodeInfo root,String query){
        if(root==null)return null;
        if(query.equals("@editor")&&"android.widget.EditText".contentEquals(root.getClassName()))return root;
        if((root.getText()!=null&&query.contentEquals(root.getText()))||(root.getContentDescription()!=null&&query.contentEquals(root.getContentDescription())))return root;
        for(int i=0;i<root.getChildCount();i++){AccessibilityNodeInfo result=find(root.getChild(i),query);if(result!=null)return result;}return null;
    }
    public static void main(String[] args)throws Exception{
        android.os.Looper.prepareMainLooper();
        HandlerThread thread=new HandlerThread("Vast UI QA");thread.start();
        Class<?> connection=Class.forName("android.app.UiAutomationConnection");Object binder=connection.getDeclaredConstructor().newInstance();
        Constructor<?> ctor=UiAutomation.class.getDeclaredConstructor(android.os.Looper.class,Class.forName("android.app.IUiAutomationConnection"));
        UiAutomation ui=(UiAutomation)ctor.newInstance(thread.getLooper(),binder);UiAutomation.class.getMethod("connect").invoke(ui);
        int exit=0;try{android.accessibilityservice.AccessibilityServiceInfo info=ui.getServiceInfo();info.flags|=android.accessibilityservice.AccessibilityServiceInfo.FLAG_RETRIEVE_INTERACTIVE_WINDOWS;ui.setServiceInfo(info);
            Thread.sleep(300);String operation=args[0],query=args.length>1?args[1]:"@editor";
            if(operation.equals("chrome")){
                if(lookup(ui,"Collapse")!=null){click(ui,"Collapse");Thread.sleep(250);}
                android.graphics.Rect nav=null,rail=null;
                for(android.view.accessibility.AccessibilityWindowInfo window:ui.getWindows()){
                    AccessibilityNodeInfo root=window.getRoot();android.graphics.Rect bounds=new android.graphics.Rect();
                    if(root==null)continue;root.getBoundsInScreen(bounds);
                    if(find(root,"Projects")!=null&&find(root,"Undo")!=null)nav=bounds;
                    if(find(root,"Settings")!=null&&find(root,"Expand")!=null)rail=bounds;
                }
                if(nav==null||rail==null||nav.height()!=rail.width())throw new IllegalStateException("Chrome dimension mismatch: nav="+nav+" rail="+rail);
                System.out.println("CHROME_SIZE_PASS nav_height_px="+nav.height()+" compact_rail_width_px="+rail.width());
                if(args.length>1){android.graphics.Bitmap shot=ui.takeScreenshot();try(java.io.FileOutputStream f=new java.io.FileOutputStream(args[1])){shot.compress(android.graphics.Bitmap.CompressFormat.PNG,100,f);}shot.recycle();}
                click(ui,"Search");Thread.sleep(200);click(ui,"Close");System.out.println("CHROME_SEARCH_PASS");return;
            }
            if(operation.equals("tap")||operation.equals("pen")){tap(ui,Float.parseFloat(args[1]),Float.parseFloat(args[2]),operation.equals("pen")?android.view.MotionEvent.TOOL_TYPE_STYLUS:android.view.MotionEvent.TOOL_TYPE_FINGER);System.out.println("TAP_PASS "+operation);return;}
            if(operation.equals("rapid")){
                long start=android.os.SystemClock.uptimeMillis();
                for(int i=0;i<12;i++){
                    if(lookup(ui,"Settings")==null)tap(ui,22,960);click(ui,"Settings");Thread.sleep(40);
                    for(String category:new String[]{"Drawing","Canvas","Handwriting Search","Appearance","Stylus button"}){click(ui,category);Thread.sleep(35);}
                    click(ui,"Close");Thread.sleep(25);
                }
                Thread.sleep(250);if(lookup(ui,"Close")!=null)throw new IllegalStateException("Stale dialog after dismiss");
                System.out.println("RAPID_PASS cycles=12 category_changes=60 requested_gap_ms=25-40 elapsed_ms="+(android.os.SystemClock.uptimeMillis()-start));
                return;
            }
            AccessibilityNodeInfo node=find(ui.getRootInActiveWindow(),query);
            if(node==null)for(android.view.accessibility.AccessibilityWindowInfo window:ui.getWindows()){node=find(window.getRoot(),query);if(node!=null)break;}
            if(node==null)throw new IllegalStateException("No native View: "+query);
            if(operation.equals("text")||operation.equals("long")){Bundle b=new Bundle();String value=args.length>2?args[2]:"Hello 世界\nLive Android text";
                if(operation.equals("long")){StringBuilder paragraph=new StringBuilder();for(int i=1;i<=6;i++)paragraph.append("Paragraph ").append(i).append(": English and 中文 remain visible, wrap naturally, and preserve every line. 😀\n");value=paragraph.toString();}
                b.putCharSequence(AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE,value);if(!node.performAction(AccessibilityNodeInfo.ACTION_SET_TEXT,b))throw new IllegalStateException("Set text failed");Thread.sleep(200);node.refresh();if(!value.contentEquals(node.getText()))throw new IllegalStateException("Text not immediately visible: "+node.getText());System.out.println("LIVE_TEXT_PASS chars="+value.length()+" value="+node.getText());}
            else if(operation.equals("click")){if(!node.performAction(AccessibilityNodeInfo.ACTION_CLICK))throw new IllegalStateException("Click failed");System.out.println("CLICK_PASS "+query);}
            else if(operation.equals("select")){Bundle b=new Bundle();b.putInt(AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_START_INT,0);b.putInt(AccessibilityNodeInfo.ACTION_ARGUMENT_SELECTION_END_INT,Math.min(5,node.getText().length()));if(!node.performAction(AccessibilityNodeInfo.ACTION_SET_SELECTION,b))throw new IllegalStateException("Selection failed");node.refresh();System.out.println("SELECTION_PASS start="+node.getTextSelectionStart()+" end="+node.getTextSelectionEnd());}
            else System.out.println("VIEW class="+node.getClassName()+" text="+node.getText()+" visible="+node.isVisibleToUser()+" editable="+node.isEditable());
        }catch(Exception ex){System.out.println("QA_FAIL "+ex);exit=1;}finally{UiAutomation.class.getMethod("disconnect").invoke(ui);thread.quitSafely();System.exit(exit);}
    }
}
