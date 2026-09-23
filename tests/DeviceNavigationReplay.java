import android.os.SystemClock;
import android.view.InputEvent;
import android.view.MotionEvent;
import java.lang.reflect.Method;

/** Synthetic navigation only; no claims about physical finger latency. */
public final class DeviceNavigationReplay {
    static Object manager;static Method inject;static long down,total,max;static int events;
    static void send(int action,float[]xy)throws Exception{
        int n=xy.length/2;MotionEvent.PointerProperties[]p=new MotionEvent.PointerProperties[n];MotionEvent.PointerCoords[]c=new MotionEvent.PointerCoords[n];
        for(int i=0;i<n;i++){p[i]=new MotionEvent.PointerProperties();p[i].id=i;p[i].toolType=MotionEvent.TOOL_TYPE_FINGER;c[i]=new MotionEvent.PointerCoords();c[i].x=xy[i*2];c[i].y=xy[i*2+1];c[i].pressure=1;c[i].size=.1f;}
        MotionEvent e=MotionEvent.obtain(down,SystemClock.uptimeMillis(),action,n,p,c,0,0,1,1,0,0,0x1002,0);long start=System.nanoTime();boolean ok=(Boolean)inject.invoke(manager,e,2);long ns=System.nanoTime()-start;e.recycle();if(!ok)throw new IllegalStateException("Rejected event");total+=ns;max=Math.max(max,ns);events++;Thread.sleep(8);
    }
    public static void main(String[]args)throws Exception{
        Class<?>cls=Class.forName("android.hardware.input.InputManagerGlobal");manager=cls.getMethod("getInstance").invoke(null);inject=cls.getMethod("injectInputEvent",InputEvent.class,int.class);
        String mode=args.length>0?args[0]:"pan";int repeats=args.length>1?Integer.parseInt(args[1]):6;
        for(int loop=0;loop<repeats;loop++){down=SystemClock.uptimeMillis();
            if(mode.equals("pan")){send(0,new float[]{2500,600});for(int k=1;k<=120;k++){float dx=300*(float)Math.sin(k*Math.PI/60);send(2,new float[]{2500+dx,600});}send(1,new float[]{2500,600});}
            else{float initial=mode.equals("out")?650:100,last=initial;send(0,new float[]{1500-initial,900});send(5|(1<<8),new float[]{1500-initial,900,1500+initial,900});for(int k=1;k<=120;k++){float t=k/120f;last=mode.equals("out")?650-550*t:100+550*t;send(2,new float[]{1500-last,900,1500+last,900});}send(6|(1<<8),new float[]{1500-last,900,1500+last,900});send(1,new float[]{1500-last,900});}
        }
        System.out.printf("NAV_PASS mode=%s events=%d dispatch_mean_us=%.3f max_us=%.3f%n",mode,events,total/1000.0/events,max/1000.0);
    }
}
