// Compile-only probe for POSIX/Bionic APIs used by the Android engine host.
#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

static void *probe_thread(void *unused) {
    (void)unused;
    pthread_setname_np(pthread_self(), "gta.engine.main");
    return 0;
}
int main(void) {
    Dl_info info = {0};
    (void)dladdr((void *)&main, &info);
    long page = sysconf(_SC_PAGESIZE);
    if (page <= 0) return 1;
    void *memory = mmap(0, (size_t)page, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) return 2;
    unsigned char resident = 0;
    (void)mincore(memory, (size_t)page, &resident);
    (void)munmap(memory, (size_t)page);
    pthread_t thread;
    if (pthread_create(&thread, 0, probe_thread, 0) == 0)
        (void)pthread_join(thread, 0);
    return 0;
}
