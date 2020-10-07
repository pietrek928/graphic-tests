#include <iostream>
#include <tuple>

//#include "gl_obj.h"

template <class... aa>
class A {
    std::tuple<aa...> v;

    public:

    A() {}
};

template<int N, typename Tp, class... Targs>
inline void for_each(Tp &p, std::tuple<Targs...> &t) {
    if constexpr(N < sizeof...(Targs)) {
        p.process(std::get<N>(t));
        for_each<(int)(N+1)>(p, t);
    }
}

template<typename Tp, class... Targs>
void for_each(Tp &p, std::tuple<Targs...> &t) {
    for_each<0>(p, t);
}

struct PP {
    void process(auto &a) {
        std::cout << sizeof(a) << std::endl;
    }
};

int main() {
    //A<int, double> a();
    std::tuple<int, double> v;
    PP pp;
    for_each(pp, v);
    return 0;
}

