typedef unsigned long size_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
void* malloc(size_t n){return 0;} void* calloc(size_t a,size_t b){return 0;} void* realloc(void*p,size_t n){return 0;} void free(void*p){} void* memset(void*p,int c,size_t n){return p;} void* memcpy(void*a,const void*b,size_t n){return a;} void* memmove(void*a,const void*b,size_t n){return a;}
int memcmp(const void*a,const void*b,size_t n){return 0;} size_t strlen(const char*s){return 0;} int strcmp(const char*a,const char*b){return 0;} int strncmp(const char*a,const char*b,size_t n){return 0;}
int snprintf(char*b,size_t n,const char*f,...){return 0;} void* fopen(const char*a,const char*b){return 0;} size_t fread(void*a,size_t b,size_t c,void*d){return 0;} size_t fwrite(const void*a,size_t b,size_t c,void*d){return c;} int ferror(void*f){return 0;} int fflush(void*f){return 0;} int fclose(void*a){return 0;} int fileno(void*f){return 0;} int fsync(int fd){return 0;} long syscall(long n,...){return 0;} int open(const char*p,int flags,...){return 0;} int rename(const char*a,const char*b){return 0;} int remove(const char*p){return 0;}
long read(int fd,void*b,size_t n){return 0;} long write(int fd,const void*b,size_t n){return 0;} int close(int fd){return 0;} int eventfd(unsigned int initval,int flags){return 1;} int timerfd_create(int c,int f){return 1;}
typedef long time_t; typedef struct { time_t tv_sec; long tv_nsec; } ts; typedef struct { ts it_interval; ts it_value; } its; int timerfd_settime(int fd,int flags,const its* n,its* o){return 0;}
int clock_gettime(int clock_id,ts*value){if(value){value->tv_sec=0;value->tv_nsec=0;}return 0;} int setpriority(int which,int who,int priority){return 0;}
typedef long pthread_t; typedef struct{int32_t v[10];} pthread_mutex_t; typedef struct{int32_t v[12];} pthread_cond_t;
int pthread_create(pthread_t*t,const void*a,void*(*start)(void*),void*arg){return 0;} int pthread_join(pthread_t t,void**result){return 0;}
int pthread_mutex_init(pthread_mutex_t*m,const void*a){return 0;} int pthread_mutex_destroy(pthread_mutex_t*m){return 0;} int pthread_mutex_lock(pthread_mutex_t*m){return 0;} int pthread_mutex_unlock(pthread_mutex_t*m){return 0;}
int pthread_cond_init(pthread_cond_t*c,const void*a){return 0;} int pthread_cond_destroy(pthread_cond_t*c){return 0;} int pthread_cond_wait(pthread_cond_t*c,pthread_mutex_t*m){return 0;} int pthread_cond_timedwait(pthread_cond_t*c,pthread_mutex_t*m,const ts*until){return 0;} int pthread_cond_signal(pthread_cond_t*c){return 0;} int pthread_cond_broadcast(pthread_cond_t*c){return 0;}
