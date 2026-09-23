import android.app.UiAutomation;
import android.os.HandlerThread;
import android.os.SystemClock;
import android.graphics.Bitmap;
import android.view.MotionEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import java.io.File;
import java.io.FileOutputStream;
import java.lang.reflect.Constructor;

/** Shell-only screenshot review through real native Views and injected input. */
public final class DeviceVisualReview {
    static UiAutomation ui;static String dir;static long down;
    static void click(String title)throws Exception{DeviceUiProbe.click(ui,title);Thread.sleep(240);}
    static void capture(String name)throws Exception{Thread.sleep(220);Bitmap b=ui.takeScreenshot();if(b==null)throw new IllegalStateException("No screenshot");try(FileOutputStream f=new FileOutputStream(new File(dir,name+".png"))){b.compress(Bitmap.CompressFormat.PNG,100,f);}b.recycle();System.out.println("CAPTURE "+name);}
    static void contact(int action,float x,float y)throws Exception{
        if(action==0)down=SystemClock.uptimeMillis();MotionEvent.PointerProperties p=new MotionEvent.PointerProperties();p.id=0;p.toolType=MotionEvent.TOOL_TYPE_FINGER;MotionEvent.PointerCoords c=new MotionEvent.PointerCoords();c.x=x;c.y=y;c.pressure=1;c.size=.1f;MotionEvent e=MotionEvent.obtain(down,SystemClock.uptimeMillis(),action,1,new MotionEvent.PointerProperties[]{p},new MotionEvent.PointerCoords[]{c},0,0,1,1,0,0,0x1002,0);if(!ui.injectInputEvent(e,true))throw new IllegalStateException("Injection failed");e.recycle();
    }
    public static void main(String[] args)throws Exception{
        android.os.Looper.prepareMainLooper();HandlerThread thread=new HandlerThread("Vast glass review");thread.start();
        Class<?> connection=Class.forName("android.app.UiAutomationConnection");Object binder=connection.getDeclaredConstructor().newInstance();Constructor<?> ctor=UiAutomation.class.getDeclaredConstructor(android.os.Looper.class,Class.forName("android.app.IUiAutomationConnection"));ui=(UiAutomation)ctor.newInstance(thread.getLooper(),binder);UiAutomation.class.getMethod("connect").invoke(ui);
        int code=0;try{android.accessibilityservice.AccessibilityServiceInfo info=ui.getServiceInfo();info.flags|=android.accessibilityservice.AccessibilityServiceInfo.FLAG_RETRIEVE_INTERACTIVE_WINDOWS;ui.setServiceInfo(info);Thread.sleep(400);
            dir=args.length>0?args[0]:"/data/local/tmp/vast-glass-review";new File(dir).mkdirs();
            if(DeviceUiProbe.lookup(ui,"Expand")!=null)click("Expand");
            capture("01-nav-and-expanded-tray");click("Collapse");capture("02-collapsed-tray");
            click("Settings");click("Drawing");capture("03-settings-drawing");click("Canvas");capture("04-settings-canvas");click("Handwriting Search");capture("05-settings-handwriting");click("Close");
            click("Search");capture("06-search");
            // The search query is a native editor; enter a harmless query and cancel.
            AccessibilityNodeInfo search=DeviceUiProbe.lookup(ui,"Search canvas");if(search!=null){click("Search canvas");AccessibilityNodeInfo editor=DeviceUiProbe.lookup(ui,"@editor");if(editor==null)throw new IllegalStateException("Missing EditText");android.os.Bundle b=new android.os.Bundle();b.putCharSequence(AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE,"Vast · 中文 · visible input");editor.performAction(AccessibilityNodeInfo.ACTION_SET_TEXT,b);Thread.sleep(300);capture("07-search-editor-ime");editor.refresh();if(!editor.getText().toString().contains("中文"))throw new IllegalStateException("Text not visible");click("Cancel");}
            if(DeviceUiProbe.lookup(ui,"Close")!=null)click("Close");
            click("Add");capture("08-add-panel");click("Shape");capture("09-tool-tray");click("Done");
            DeviceUiProbe.tap(ui,2960,1880);Thread.sleep(250);capture("10-color-expanded");click("Done");capture("11-color-collapsed");
            contact(0,1500,960);Thread.sleep(2100);capture("12-radial");contact(2,1500,860);Thread.sleep(150);capture("13-radial-active");contact(1,1500,860);Thread.sleep(200);
            click("Settings");click("Appearance");click("Paper");capture("14-settings-paper");click("Close");capture("15-paper-chrome");
            click("Settings");click("Appearance");click("Dark");click("Close");
            click("Search");click("Note  -  Hello 世界\nLive Android text");capture("16-selection-controls");
            click("More");capture("17-selection-context");click("Deselect");if(DeviceUiProbe.lookup(ui,"Close")!=null)click("Close");
            click("Settings");capture("18-settings-over-content");click("Close");
            System.out.println("VISUAL_REVIEW_PASS");
        }catch(Exception ex){ex.printStackTrace();code=1;}finally{UiAutomation.class.getMethod("disconnect").invoke(ui);thread.quitSafely();System.exit(code);}
    }
}
