#ifndef __POOL_LOCK_H_
#define __POOL_LOCK_H_

#include "lock.h"

template<int N>
class PoolLock {
    LockObject L[N];

    template<class To>
    inline int hash(To &v) {
        return (v * 74675675667) % N;
    }

    public:
        template<class To>
        auto lock(To &v) {
            auto h = hash(v);
            return ScopeLock(L[h]);
        }
};

#endif /* __POOL_LOCK_H_ */

