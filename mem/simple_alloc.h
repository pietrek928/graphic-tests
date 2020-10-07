#ifndef __SIMPLE_ALLOC_H_
#define __SIMPLE_ALLOC_H_

#include <utils.h>

#include <sys/mman.h>
#include <vector>

class SimpleAllocator {
    constexpr int PAGESIZE = 4096;

    void *ptr = MAP_FAILED;
    size_t size;

    static auto align_size(size_t size) {
        return (max_size + PAGE_SIZE - 1) & ((size_t)~(PAGE_SIZE - 1));
    }

    public:

    SimpleAllocator(size_t size=size_t(1e9+9))
        : size(align_size(size)) {
        ptr = mmap(
            NULL, size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1
        );
        ASSERT(
            new_ptr != MAP_FAILED,
            "mmap of", size, "bytes failed"
        );
    }

    void resize(size_t new_size) {
        new_size = align_size(new_size);
        if (likely(size != new_size)) {
            auto new_ptr = mremap(
                ptr, size, new_size, 0
            );

            // TODO: allow move ?
            ASSERT(
                new_ptr != MAP_FAILED,
                "mremap of", size, "to", new_size,  "failed"
            );

            ptr = new_ptr;
            size = new_size;
        }
    }

    ~SimpleAllocator() {
        if (ptr != MAP_FAILED) {
            munmap(ptr, size);
        }
    }
};

#endif /* __SIMPLE_ALLOC_H_ */

