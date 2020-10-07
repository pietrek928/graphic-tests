#include "lock.h"
#include "test.h"

#include <iostream>
#include <vector>
#include <thread>

using namespace std;


class SyncInt : public LockObject {
    public:
    volatile int v = 0;
};

template<class Tl>
void many_inc_thread(Tl *v, int nit) {
    for (int i=0; i<nit; i++) {
        auto l = v->lock();
        //ScopeLock l(*v);
        //v->lock_c();
        v->v++;
        //v->v++;
        //v->v++;
        //v->v++;
        //v->v++;
        //v->v++;
        //v->v++;
        //v->unlock_c();
    }
}

void test_many_inc(int nth, int nit) {
    SyncInt v;
    vector<thread> T;
    for (int i = 0; i < nth; i++) {
        T.emplace_back(many_inc_thread<decltype(v)>, &v, nit);
    }
    for (auto &t : T) {
        t.join();
    }
    ASSERT(v.v == nth * nit, "Invalid count", v.v);
}

int main() {
    DBG_STREAM(test) << "Object size: " << sizeof(LockObject) << DBG_ENDL;
    TEST(test_many_inc).benchmark(50, 10, 1e6);
    return 0;
}

