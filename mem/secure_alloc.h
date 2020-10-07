#ifndef __SECURE_ALLOC_H_
#define __SECURE_ALLOC_H_

#include <utils.h>

#include <asm/cachectl.h>
#include <sys/mman.h>
#include <vector>

template<int PAGE_SIZE=4096>
class SecureAllocator {
    void *ptr = MAP_FAILED;
    size_t size;
    
    static void clear_data(void *_data, size_t data_size) {
        uint32_t *data = (uint32_t*)_data;
        uint32_t *data_end = (uint32_t*)(((char*)_data) + data_size;)
        std::fill(data, data_end, 0xFFFFFFFF)
        ASSERT(cacheflush(_data, data_size, BCACHE) != -1,
            "flushing cache failed"
        );
        std::fill(data, data_end, 0)
        ASSERT(cacheflush(_data, data_size, BCACHE) != -1,
            "flushing cache failed"
        );
    }

    static auto align_size(size_t size) {
        return (max_size + PAGE_SIZE - 1) & ((size_t)~(PAGE_SIZE - 1));
    }

    public:

    SecureAllocator(size_t size)
        : size(align_size(size)) {
        ptr = mmap(
            NULL, size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_LOCKED, -1
        );
        ASSERT(
            ptr != MAP_FAILED,
            "mmap of", size, "bytes failed"
        )
    }

    void resize(size_t new_size) {
        new_size = align_size(new_size);
        if (likely(size != new_size)) {
            if (new_size < size) {
                clear_data(((char*)ptr)+new_size, size-new_size);
            }
            auto new_ptr = mremap(
                ptr, size, new_size, 0
            );

            // TODO: allow move ?
            ASSERT(new_ptr != MAP_FAILED,
                "mremap of", size,  "to", new_size, "failed"
            );

            ptr = new_ptr;
            size = new_size;
        }
    }

    ~SecureAllocator() {
        if (ptr != MAP_FAILED) {
            clear_data(ptr, size);
            munmap(ptr, size);
        }
    }
};

#endif /* __SECURE_ALLOC_H_ */
