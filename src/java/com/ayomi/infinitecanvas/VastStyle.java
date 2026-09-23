package com.ayomi.infinitecanvas;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.*;
import android.graphics.drawable.*;
import android.view.Gravity;
import android.widget.*;

/** Shared native View materials. No backdrop capture, blur, or bitmap assets. */
final class VastStyle {
    static int dp(Context c,int n){return Math.round(n*c.getResources().getDisplayMetrics().density);}
    static boolean light(int bg){return Color.luminance(bg|0xff000000)>.45;}
    static int ink(int bg){return light(bg)?0xff292c30:0xfff3f0eb;}
    static int muted(int bg){return light(bg)?0xff676b70:0xffb9bec5;}
    static int accent(int bg){return light(bg)?0xff536e74:0xffb7cbd0;}
    static int alpha(int color,int a){return (color&0xffffff)|(a<<24);}
    static ColorStateList textColors(int bg){return new ColorStateList(new int[][]{new int[]{-android.R.attr.state_enabled},new int[]{}},new int[]{alpha(ink(bg),92),ink(bg)});}
    static GradientDrawable rounded(Context c,int fill,int radius){GradientDrawable d=new GradientDrawable();d.setColor(fill);d.setCornerRadius(dp(c,radius));return d;}
    static Drawable glass(Context c,int bg,int radius,boolean panel){
        boolean l=light(bg);int[] colors=l?new int[]{panel?0xf5faf8f4:0xe8faf8f4,panel?0xf0e8e7e3:0xdbe8e7e3}:new int[]{panel?0xf1393e45:0xe3393e45,panel?0xf0262a30:0xdc262a30};
        GradientDrawable base=new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM,colors);base.setCornerRadius(dp(c,radius));
        base.setStroke(1,l?0x65ffffff:0x35ffffff);
        return base;
    }
    static Drawable interaction(Context c,int bg,int radius){
        StateListDrawable states=new StateListDrawable();
        GradientDrawable active=rounded(c,light(bg)?0xc9ffffff:0x32ffffff,radius);active.setStroke(1,light(bg)?0x80ffffff:0x36ffffff);
        states.addState(new int[]{android.R.attr.state_selected},active);
        states.addState(new int[]{android.R.attr.state_focused},rounded(c,alpha(accent(bg),48),radius));
        states.addState(new int[]{android.R.attr.state_hovered},rounded(c,alpha(ink(bg),18),radius));
        states.addState(new int[]{},new ColorDrawable(Color.TRANSPARENT));
        return new RippleDrawable(ColorStateList.valueOf(alpha(ink(bg),30)),states,rounded(c,Color.WHITE,radius));
    }
    static void button(Button b,int bg){
        b.setAllCaps(false);b.setElevation(0);b.setStateListAnimator(null);b.setTextColor(textColors(bg));b.setTextSize(14);
        b.setTypeface(Typeface.create("sans-serif-medium",Typeface.NORMAL));b.setLetterSpacing(.01f);b.setIncludeFontPadding(false);
        b.setBackground(interaction(b.getContext(),bg,16));b.setMinWidth(0);b.setMinimumWidth(0);b.setMinHeight(dp(b.getContext(),48));
        b.setPadding(dp(b.getContext(),14),dp(b.getContext(),8),dp(b.getContext(),14),dp(b.getContext(),8));
    }
    static void toggle(Switch s,int bg){
        s.setTextColor(textColors(bg));s.setTextSize(15);s.setSwitchPadding(dp(s.getContext(),20));
        s.setThumbTintList(new ColorStateList(new int[][]{new int[]{android.R.attr.state_checked},new int[]{}},new int[]{light(bg)?0xfff9faf8:0xfff1f0eb,light(bg)?0xfff2f2ee:0xffc1c5c9}));
        s.setTrackTintList(new ColorStateList(new int[][]{new int[]{android.R.attr.state_checked},new int[]{}},new int[]{accent(bg),alpha(ink(bg),48)}));
    }
    static void field(EditText e,int bg){
        Context c=e.getContext();e.setTextColor(ink(bg));e.setHintTextColor(muted(bg));e.setHighlightColor(alpha(accent(bg),90));
        GradientDrawable fill=rounded(c,light(bg)?0xc9ffffff:0x700e1319,18);fill.setStroke(1,alpha(ink(bg),55));e.setBackground(fill);
        e.setPadding(dp(c,18),dp(c,16),dp(c,18),dp(c,16));e.setLineSpacing(dp(c,3),1);
        if(android.os.Build.VERSION.SDK_INT>=29){GradientDrawable cursor=rounded(c,accent(bg),1);cursor.setSize(dp(c,2),dp(c,24));e.setTextCursorDrawable(cursor);}
    }
    static TextView title(Context c,String value,int bg){TextView t=new TextView(c);t.setText(value);t.setTextSize(23);t.setTextColor(ink(bg));t.setTypeface(Typeface.create("sans-serif-medium",0));t.setPadding(dp(c,28),dp(c,24),dp(c,28),dp(c,18));return t;}
    static String iconName(String title){
        String t=title.toLowerCase(java.util.Locale.ROOT);
        if(t.contains("search"))return "search";if(t.equals("undo"))return "undo";if(t.equals("redo"))return "redo";
        if(t.contains("project")||t.contains("library"))return "projects";if(t.equals("focus")||t.contains("select"))return "focus";
        if(t.equals("add")||t.startsWith("new"))return "add";if(t.contains("frame")||t.contains("rectangle"))return "frame";
        if(t.contains("photo"))return "photo";if(t.contains("measure")||t.contains("dimension")||t.contains("calibrat"))return "measure";
        if(t.contains("layer"))return "layers";if(t.equals("map")||t.contains("place"))return "map";
        if(t.contains("setting")||t.contains("advanced"))return "settings";if(t.contains("draw")||t.equals("pen")||t.equals("edit"))return "pen";
        if(t.contains("appearance")||t.contains("color"))return "color";if(t.contains("canvas"))return "frame";
        if(t.contains("stylus"))return "pen";if(t.contains("duplicate"))return "copy";if(t.contains("delete")||t.contains("remove"))return "delete";
        if(t.contains("text")||t.contains("note"))return "text";if(t.equals("done")||t.equals("save")||t.equals("apply"))return "check";
        if(t.equals("close")||t.equals("cancel"))return "close";if(t.equals("collapse"))return "left";if(t.equals("expand"))return "right";
        if(t.contains("lock"))return "lock";if(t.equals("more"))return "more";if(t.equals("ellipse"))return "ellipse";
        if(t.equals("line"))return "line";if(t.equals("arrow"))return "arrow";if(t.equals("shape"))return "frame";
        return "";
    }
    static Drawable icon(Context c,String title,int bg,int size){String name=iconName(title);if(name.isEmpty())return null;Icon d=new Icon(name,textColors(bg));d.setBounds(0,0,dp(c,size),dp(c,size));return d;}
    static final class Icon extends Drawable {
        final Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);final Path path=new Path();final String name;final ColorStateList colors;int opacity=255;
        Icon(String name,ColorStateList colors){this.name=name;this.colors=colors;}
        @Override public boolean isStateful(){return true;}
        @Override protected boolean onStateChange(int[] state){invalidateSelf();return true;}
        void line(Canvas c,float... xy){path.reset();path.moveTo(xy[0],xy[1]);for(int i=2;i<xy.length;i+=2)path.lineTo(xy[i],xy[i+1]);c.drawPath(path,p);}
        void box(Canvas c,float l,float t,float r,float b,float rad){c.drawRoundRect(l,t,r,b,rad,rad,p);}
        @Override public void draw(Canvas c){Rect b=getBounds();c.save();c.translate(b.left,b.top);c.scale(b.width()/24f,b.height()/24f);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(1.65f);p.setStrokeCap(Paint.Cap.ROUND);p.setStrokeJoin(Paint.Join.ROUND);p.setColor(colors.getColorForState(getState(),colors.getDefaultColor()));p.setAlpha(Color.alpha(p.getColor())*opacity/255);
            switch(name){
                case "search":c.drawCircle(10.5f,10.5f,6.5f,p);line(c,15.5f,15.5f,21,21);break;
                case "undo":case "redo":if(name.equals("redo")){c.translate(24,0);c.scale(-1,1);}line(c,8,5,3,10,8,15);path.reset();path.moveTo(3,10);path.cubicTo(20,5,24,16,16,20);c.drawPath(path,p);break;
                case "projects":box(c,3,8,21,21,4);line(c,5,5,19,5);line(c,8,2,16,2);break;
                case "focus":line(c,8,3,3,3,3,8);line(c,16,3,21,3,21,8);line(c,3,16,3,21,8,21);line(c,21,16,21,21,16,21);break;
                case "add":line(c,12,4,12,20);line(c,4,12,20,12);break;
                case "frame":box(c,4,4,20,20,3);break;
                case "photo":box(c,3,4,21,20,3);c.drawCircle(8,9,1.5f,p);line(c,4,18,10,12,14,16,17,13,21,17);break;
                case "measure":c.save();c.rotate(-40,12,12);box(c,2,7,22,17,2);line(c,7,7,7,11);line(c,12,7,12,13);line(c,17,7,17,11);c.restore();break;
                case "layers":line(c,12,3,22,9,12,15,2,9,12,3);line(c,3,14,12,20,21,14);break;
                case "map":line(c,3,6,9,3,15,6,21,3,21,18,15,21,9,18,3,21,3,6);line(c,9,3,9,18);line(c,15,6,15,21);break;
                case "settings":c.drawCircle(12,12,6,p);c.drawCircle(12,12,2,p);for(int i=0;i<8;i++){c.save();c.rotate(i*45,12,12);line(c,12,3,12,5);c.restore();}break;
                case "pen":line(c,4,20,6,14,17,3,21,7,10,18,4,20);line(c,14,6,18,10);break;
                case "color":c.drawCircle(12,12,9,p);p.setStyle(Paint.Style.FILL);for(int i=0;i<3;i++)c.drawCircle(7+i*5,10+(i==1?-3:2),1.5f,p);break;
                case "copy":box(c,8,7,21,21,3);line(c,15,4,15,3,3,3,3,16,5,16);break;
                case "delete":line(c,3,6,21,6);line(c,9,3,15,3);line(c,6,6,7,21,17,21,18,6);line(c,10,10,10,17);line(c,14,10,14,17);break;
                case "text":line(c,4,5,20,5);line(c,12,5,12,20);line(c,8,20,16,20);break;
                case "check":line(c,4,12,9,17,20,6);break;
                case "close":line(c,6,6,18,18);line(c,6,18,18,6);break;
                case "left":line(c,14,6,8,12,14,18);break;
                case "right":line(c,10,6,16,12,10,18);break;
                case "lock":box(c,5,10,19,21,3);c.drawArc(8,2,16,16,180,180,false,p);break;
                case "more":p.setStyle(Paint.Style.FILL);for(int i=0;i<3;i++)c.drawCircle(5+i*7,12,1.5f,p);break;
                case "ellipse":c.drawOval(3,5,21,19,p);break;
                case "line":line(c,4,20,20,4);break;
                case "arrow":line(c,4,20,20,4,10,4);line(c,20,4,20,14);break;
            }c.restore();
        }
        @Override public void setAlpha(int a){opacity=a;invalidateSelf();}
        @Override public void setColorFilter(ColorFilter f){p.setColorFilter(f);}
        @Override public int getOpacity(){return PixelFormat.TRANSLUCENT;}
    }
}
