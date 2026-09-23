float sqrtf(float x){return x;} float floorf(float x){return x;} float ceilf(float x){return x;} float fabsf(float x){return x<0?-x:x;}
float expf(float x){return x;}
 float atan2f(float a,float b){return 0;}

float sinf(float x){return x;} float cosf(float x){return 1.0f;} void sincosf(float x,float*s,float*c){if(s)*s=x;if(c)*c=1.0f;}
