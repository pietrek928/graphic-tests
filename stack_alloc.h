#ifndef __STACK_ALLOC_H_
#define __STACK_ALLOC_H_

#include <utils.h>

#include <sys/mman.h>
#include <vector>

/*template<class T>
class StackObjectHolder {
    T* ptr = NULL;

    public:
    StackObjectHolder(T *ptr)
        : ptr(ptr) {}

    operator T*
}*/

class StackAllocator {
    void *ptr = MAP_FAILED;
    size_t size;

    constexpr static int REGION_USED = 1;

    class AllocInfo {
        void *obj_ptr;
        int flags;
    };

    public:

    StackAllocator(size_t max_size=size_t(1e9+9))
        : size(max_size & ((size_t)~0xFFFF)) {
        ptr = mmap(
            , size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_NORESERVE, -1
        );
        if (unlikely(ptr == MAP_FAILED)) {
            throw std::runtime_error(((std::string)"mmap of ") + std::to_string(size) + " bytes failed")
        }
    }

    void alloc

    ~StackAllocator() {
        if (ptr != MAP_FAILED) {
            munmap(ptr, size);
        }
    }
};

#endif /* __STACK_ALLOC_H_ */

