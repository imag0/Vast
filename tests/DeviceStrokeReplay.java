import android.os.SystemClock;
import android.view.InputEvent;
import android.view.MotionEvent;
import java.lang.reflect.Method;

/** Shell-only synthetic input replay. Does not validate a physical digitizer. */
public final class DeviceStrokeReplay {
    public static void main(String[] args) throws Exception {
        int count=args.length>0?Integer.parseInt(args[0]):1200;
        Class<?> cls=Class.forName("android.hardware.input.InputManagerGlobal");
        Object manager=cls.getMethod("getInstance").invoke(null);
        Method inject=cls.getMethod("injectInputEvent",InputEvent.class,int.class);
        MotionEvent.PointerProperties prop=new MotionEvent.PointerProperties();
        prop.id=0;prop.toolType=MotionEvent.TOOL_TYPE_STYLUS;
        MotionEvent.PointerCoords coord=new MotionEvent.PointerCoords();
        coord.pressure=.6f;coord.size=.1f;
        long down=SystemClock.uptimeMillis(),totalNs=0,maxNs=0;
        int bucket=0;
        for(int i=0;i<=count;i++){
            coord.x=1500+650*(float)Math.cos(i*.035);
            coord.y=900+500*(float)Math.sin(i*.035);
            int action=i==0?MotionEvent.ACTION_DOWN:i==count?MotionEvent.ACTION_UP:MotionEvent.ACTION_MOVE;
            MotionEvent event=MotionEvent.obtain(down,SystemClock.uptimeMillis(),action,1,
                new MotionEvent.PointerProperties[]{prop},new MotionEvent.PointerCoords[]{coord},
                0,0,1,1,0,0,0x4002,0);
            long start=System.nanoTime();
            boolean accepted=(Boolean)inject.invoke(manager,event,2);
            long elapsed=System.nanoTime()-start;event.recycle();
            if(!accepted)throw new IllegalStateException("Injection rejected at "+i);
            totalNs+=elapsed;maxNs=Math.max(maxNs,elapsed);bucket++;
            if(i==20||i==200||i==600||i==2000||i==count){
                System.out.printf("events=%d bucket=%d synchronous_inject_mean_us=%.3f max_us=%.3f elapsed_ms=%d%n",
                    i,bucket,totalNs/1000.0/bucket,maxNs/1000.0,SystemClock.uptimeMillis()-down);
                totalNs=0;maxNs=0;bucket=0;
            }
            if(i<count)Thread.sleep(8);
        }
    }
}
