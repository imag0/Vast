void *dlopen(const char *filename, int flags) {
    (void)filename;
    (void)flags;
    return 0;
}

void *dlsym(void *handle, const char *symbol) {
    (void)handle;
    (void)symbol;
    return 0;
}

int dlclose(void *handle) {
    (void)handle;
    return 0;
}

const char *dlerror(void) {
    return 0;
}
