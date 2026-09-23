/* Causal, componentwise monotone Hermite pieces. A finished piece never depends
 * on a future sample. Geometry is identical during contact, after UP and in
 * exports; subdivision depends on world curvature, never the camera. */
static float ink_tangent(float previous,float next){
    if(previous*next<=0)return 0;
    return 2*previous*next/(previous+next);
}
static Point ink_curve_point(Point previous,Point a,Point b,float t,int first){
    float dx=b.x-a.x,dy=b.y-a.y;
    float mx=first?dx:ink_tangent(a.x-previous.x,dx);
    float my=first?dy:ink_tangent(a.y-previous.y,dy);
    float u=t*(1-t)*(1-t);
    return (Point){a.x+dx*t+(mx-dx)*u,a.y+dy*t+(my-dy)*u,a.p+(b.p-a.p)*t};
}
static int ink_curve_steps(Point previous,Point a,Point b,int first){
    if(first)return 1;
    float dx=b.x-a.x,dy=b.y-a.y;
    float ex=ink_tangent(a.x-previous.x,dx)-dx;
    float ey=ink_tangent(a.y-previous.y,dy)-dy;
    /* |C''| <= 4*|tangent-secant|. Chord error <= |C''|/(8*n*n).
     * .01 world units is .2 pixels at the unchanged maximum 20x zoom. */
    float error=sqrtf(ex*ex+ey*ey);
    return maxi(1,mini(64,(int)ceilf(sqrtf(error*50.0f))));
}
