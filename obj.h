#ifndef __OBJECT_H_
#define __OBJECT_H_

#include <vector>

#include "utils.h"

template<class T>
class _v2 {
    public:
        typedef T Telem;

        const static int len = 2;
        T x,y;

        _v2(T x, T y) : x(x), y(y) {}
        _v2(const _v2 &v) : x(v.x), y(v.y) {}
        _v2(T xy) : x(xy), y(xy) {}
        _v2() : x(0), y(0) {}

        auto operator+(const _v2 v) {
            return _v2(x+v.x, y+v.y);
        }

        auto operator+=(const _v2 v) {
            x += v.x;
            y += v.y;
            return *this;
        }

        auto operator-(const _v2 v) {
            return _v2(x-v.x, y-v.y);
        }

        auto operator*(const _v2 v) {
            return _v2(x*v.x, y*v.y);
        }

        auto operator*(const T s) {
            return _v2(x*s, y*s);
        }

        auto dp(const _v2 v) {
            return x*v.x + y*v.y;
        }

        auto vp(const _v2 v) {
            return x*v.y - y*v.x;
        }

        auto rqlen() {
            return T(1)/(dp(*this));
        }

        auto operator==(const _v2 v) {
            return v==v.x && y==v.y;
        }

        auto operator!=(const _v2 v) {
            return x!=v.x || y!=v.y;
        }

        auto min(const _v2 v) {
            return _v2(x<v.x ? x : v.x, y<v.y ? y : v.y);
        }

        auto max(const _v2 v) {
            return _v2(x>v.x ? x : v.x, y>v.y ? y : v.y);
        }
};
template<class Telem>
std::ostream &operator<<(std::ostream &os, const _v2<Telem> &v) {
    return os << "(" << v.x << ", " << v.y << ")";
}

typedef _v2<float> v2;
typedef _v2<int> v2i;
/*
class PtCont {
    std::vector<v2> pts;
    public:

    inline auto &get(int npt) {
        return pts[npt];
    }

    inline void put(v2 pt) {
        pts.push_back(pt);
    }
};

class DrawObj {
    public:

        virtual void render(PtCont &pt_cont) {
        }

        virtual void render_start() {
        }

        virtual void render_end() {
        }
};

class Sect : public DrawObj {
    int p1, p2;

    public:
        Sect(int p1, int p2) : p1(p1), p2(p2) {
        }

        void render(PtCont &pt_cont) { // TODO: speedup
            glBegin(GL_LINES);
            pt_cont.get(p1).put_vertex();
            pt_cont.get(p2).put_vertex();
            glEnd();
        }

};

class RefObj {
    v2 p;

public:
    RefObj(v2 &p)
        : p(p) {
    }

    // TODO: vector version
    virtual void accum_v(v2 s, v2 d, v2::Telem d) {
        auto dd = d - p;
        auto l = dd.rqlen();
        p += ;
    }
};

#define ITERATE_OBJECT_CLASSES() \
    IT_FUNC(Sect);

class ObjectContainer {
    public:
#define IT_FUNC(c) std::vector<c> CONCAT(cont_, c)
    ITERATE_OBJECT_CLASSES()
#undef IT_FUNC
    
    PtCont pt_cont;

        void render() {
#define IT_FUNC(c) for (auto &o : CONCAT(cont_, c)) o.render(pt_cont)
    ITERATE_OBJECT_CLASSES()
#undef IT_FUNC
        }
}; // */

#endif /* __OBJECT_H_ */

