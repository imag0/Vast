import android.os.SystemClock;
import android.view.InputEvent;
import android.view.MotionEvent;
import java.lang.reflect.Method;

/** Shell-only gesture/OCR smoke fixture. Never claims physical stylus coverage. */
public final class DeviceGestureReplay {
    static Object manager;
    static Method inject;
    static void event(long down,int action,float[] xy,boolean pen)throws Exception{
        int n=xy.length/2;
        MotionEvent.PointerProperties[] properties=new MotionEvent.PointerProperties[n];
        MotionEvent.PointerCoords[] points=new MotionEvent.PointerCoords[n];
        for(int i=0;i<n;i++){
            properties[i]=new MotionEvent.PointerProperties();properties[i].id=i;
            properties[i].toolType=pen?MotionEvent.TOOL_TYPE_STYLUS:MotionEvent.TOOL_TYPE_FINGER;
            points[i]=new MotionEvent.PointerCoords();points[i].x=xy[i*2];points[i].y=xy[i*2+1];points[i].pressure=.7f;points[i].size=.1f;
        }
        MotionEvent e=MotionEvent.obtain(down,SystemClock.uptimeMillis(),action,n,properties,points,0,0,1,1,0,0,pen?0x4002:0x1002,0);
        boolean accepted=(Boolean)inject.invoke(manager,e,2);e.recycle();
        if(!accepted)throw new IllegalStateException("Input rejected");Thread.sleep(8);
    }
    static void stroke(float[] vertices)throws Exception{
        long down=SystemClock.uptimeMillis();event(down,0,new float[]{vertices[0],vertices[1]},true);
        for(int j=2;j<vertices.length;j+=2)for(int k=1;k<=12;k++){
            float t=k/12f;event(down,2,new float[]{vertices[j-2]+(vertices[j]-vertices[j-2])*t,vertices[j-1]+(vertices[j+1]-vertices[j-1])*t},true);
        }
        event(down,1,new float[]{vertices[vertices.length-2],vertices[vertices.length-1]},true);
    }
    public static void main(String[] args)throws Exception{
        Class<?> cls=Class.forName("android.hardware.input.InputManagerGlobal");manager=cls.getMethod("getInstance").invoke(null);inject=cls.getMethod("injectInputEvent",InputEvent.class,int.class);
        if(args[0].equals("hello")){
            stroke(new float[]{800,450,800,610});stroke(new float[]{880,450,880,610});stroke(new float[]{800,530,880,530});
            stroke(new float[]{1000,450,930,450,930,610,1000,610});stroke(new float[]{930,530,990,530});
            stroke(new float[]{1050,450,1050,610,1120,610});stroke(new float[]{1170,450,1170,610,1240,610});
            float[] oval=new float[66];for(int i=0;i<=32;i++){oval[i*2]=1320+45*(float)Math.cos(i*Math.PI/16);oval[i*2+1]=530+80*(float)Math.sin(i*Math.PI/16);}stroke(oval);
        }else{
            long down=SystemClock.uptimeMillis();event(down,0,new float[]{1300,800},false);
            event(down,5|(1<<8),new float[]{1300,800,1700,1100},false);
            for(int i=1;i<=50;i++)event(down,2,new float[]{1300-i*2,800-i,1700+i*2,1100+i},false);
            if(args[0].equals("pinchhold")){System.out.println("HOLD pinch");Thread.sleep(6000);}
            event(down,6|(1<<8),new float[]{1200,750,1800,1150},false);event(down,1,new float[]{1200,750},false);
        }
        System.out.println("GESTURE_PASS "+args[0]);
    }
}
