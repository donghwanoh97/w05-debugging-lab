#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define ARENA_SIZE 4096
static unsigned char *arena = NULL;
static size_t arena_capacity = ARENA_SIZE;
static size_t arena_off = 0;

static size_t page_size = 0;
static size_t total_mapping_size = 0;

static void init_arena(size_t size) {
    page_size = sysconf(_SC_PAGE_SIZE);

    total_mapping_size = size + page_size;

    arena = mmap(NULL, total_mapping_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    if (mprotect(arena + size, page_size, PROT_NONE) != 0) {
        perror("mprotect failed");
        munmap(arena, total_mapping_size);
        exit(1);
    }

    arena_capacity = size;
    arena_off = 0;
    fprintf(stderr, "Guard page arena initialized. Addr: %p, Capacity: %zu, Guard at: %p\n", (void *)arena, arena_capacity, (void *)(arena + size));
    fprintf(stderr, "mmap arena address: %p\n", (void *)arena);
}

static void free_arena(void) {
    if (arena && arena != MAP_FAILED) {
        munmap(arena, total_mapping_size);
        arena = NULL;
    }
}

static void *arena_alloc(size_t n) {
    if (arena_off + n > arena_capacity) {
        return NULL;
    }

    void *p = arena + arena_off;
    arena_off += n;
    fprintf(stderr, "alloc n=%zu off=%zu cap=%zu\n", n, arena_off, arena_capacity);
    return p;
}

static char *intern(const char *s) {
    size_t n = strlen(s) + 1;
    
    char *dst = arena_alloc(n);

    if (dst == NULL) {
        return NULL;
    }

    memcpy(dst, s, n);                     
    return dst;
}

int main(void) {
    init_arena(ARENA_SIZE);

    const char *words[] = {
        "insert", "delete", "search", "traverse", "balance",
        "rotate", "rehash", "compact", "serialize", "checkpoint",
    };
    int nwords = (int)(sizeof(words) / sizeof(words[0]));

    char *last = NULL;
    long total = 0;
    for (int i = 0; i < 100000; i++) {
        char buf[32];
        snprintf(buf, sizeof buf, "%s-%d", words[i % nwords], i);
        last = intern(buf);
        
        if (last == NULL) {
            fprintf(stderr, "Arena is full.");
            return -1;
        }

        total += (long)strlen(last);
    }

    printf("interned, last=%s total_len=%ld\n", last, total);

    free_arena();
    return 0;
}
