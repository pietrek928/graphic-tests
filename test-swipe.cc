#include <vector>
#include <iostream>

#include "swipe-buckets.h"

class bucket_node {
    float x;
    public:

    bucket_node(float x) : x(x) {
    }

    auto x_pos() {
        return x;
    }
};
SwipeBuckets<bucket_node> b(100, 100);

int main() {
    auto nn = bucket_node(5.0);
    b.insert(nn);
    b.insert(nn);
    b.iterate(
        [](auto &v){std::cout << v.x_pos() << std::endl;},
        3.0, 20.0
    );
    b.optimize([](auto &v){return true;});
    b.iterate(
        [](auto &v){std::cout << v.x_pos() << std::endl;},
        3.0, 20.0
    );
    return 0;
}

