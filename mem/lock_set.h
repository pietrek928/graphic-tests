#ifndef __LOCK_SET_OBJ_H_
#define __LOCK_SET_OBJ_H_

#include <unordered_set>
#include "lock.h"

template<class To>
class SetLock : LockObject {
    unordered_set<To> lock_set;

    public:
        void lock_c(To &v) {
            auto l = LockObject::lock();
            while (lock_set.find(v) != lock_set.end()) {
                wait();
            }
        }

        void unlock_c(To &v) {
            { // scope for lock
                auto l = LockObject::lock();
                lock_set.erase(v);
            }
            notifyAll(); // All ?
        }

        inline auto lock(To &v) {
            return ScopeLockArg(*this, v);
        }
};

template<class To, int N>
class LockConstantSet : LockObject {
    int n = 0;
    To L[N];

    public:
        void lock_c(To &v) {
            auto l = LockObject::lock();
            while (lock_set.find(v) != lock_set.end()) {
                wait();
            }
        }

        void unlock_c(To &v) {
            auto l = LockObject::lock();
            lock_set.erase(v);
            notifyAll(); // All ?
        }

        inline auto lock(To &v) {
            return ScopeLockArg(*this, v);
        }
};

#endif /* __LOCK_SET_OBJ_H_ */

