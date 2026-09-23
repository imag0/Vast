import android.os.SystemClock;
import android.view.InputEvent;
import android.view.MotionEvent;
import java.lang.reflect.Method;

/** Shell-only eraser/navigation and dense-ink replay; not physical-pen validation. */
public final class DeviceEraseReplay {
    static Object manager;static Method inject;static int tool=MotionEvent.TOOL_TYPE_STYLUS;
    static boolean send(long down,int action,float x,float y)throws Exception{
        MotionEvent.PointerProperties p=new MotionEvent.PointerProperties();p.id=7;p.toolType=tool;
        MotionEvent.PointerCoords q=new MotionEvent.PointerCoords();q.x=x;q.y=y;q.pressure=action==7?0:.7f;q.size=.1f;
        MotionEvent e=MotionEvent.obtain(down,SystemClock.uptimeMillis(),action,1,new MotionEvent.PointerProperties[]{p},new MotionEvent.PointerCoords[]{q},0,0,1,1,0,0,tool==1?0x1002:0x4002,0);
        boolean ok=(Boolean)inject.invoke(manager,e,2);e.recycle();return ok;
    }
    public static void main(String[]args)throws Exception{
        Class<?>cls=Class.forName("android.hardware.input.InputManagerGlobal");manager=cls.getMethod("getInstance").invoke(null);inject=cls.getMethod("injectInputEvent",InputEvent.class,int.class);
        String mode=args[0];int count=args.length>1?Integer.parseInt(args[1]):600;
        if(mode.equals("dense")){
            for(int i=0;i<count;i++){long down=SystemClock.uptimeMillis();float x=500+(i%80)*24,y=400+(i/80%50)*20;
                if(!send(down,0,x,y)||!send(down,2,x+9,y+5)||!send(down,1,x+14,y-2))throw new IllegalStateException("Rejected "+i);
                if(i%200==199)System.out.println("created="+(i+1));}
        }else if(mode.equals("finger")||mode.equals("contact")||mode.equals("hover")||mode.equals("cancel")){
            tool=mode.equals("finger")?1:2;long down=SystemClock.uptimeMillis();boolean hover=mode.equals("hover");
            send(down,hover?9:0,1100,900);send(down,hover?7:2,1200,950);System.out.println("HOLD "+mode);Thread.sleep(args.length>1?count:2500);
            send(down,hover?10:mode.equals("cancel")?3:1,1200,950);System.out.println("RELEASE "+mode);
        }else{
            long down=SystemClock.uptimeMillis(),sum=0,max=0;int bucket=0;
            for(int i=0;i<=count;i++){float x=mode.equals("empty")?200:600+(i%160)*10,y=mode.equals("empty")?170:400+(i/160%40)*20;long start=System.nanoTime();
                if(!send(down,i==0?0:i==count?1:2,x,y))throw new IllegalStateException("Rejected "+i);
                long elapsed=System.nanoTime()-start;sum+=elapsed;max=Math.max(max,elapsed);bucket++;
                if(i==20||i==200||i==count){System.out.printf("erase mode=%s events=%d mean_us=%.3f max_us=%.3f%n",mode,i,sum/1000.0/bucket,max/1000.0);sum=max=0;bucket=0;}
                Thread.sleep(8);
            }
        }
    }
}
