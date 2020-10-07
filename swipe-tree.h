#include <vector>

template<class T>
class Point {
public:
    Point(T x, T y)
            : x(x), y(y) {
    }

    T x, y;

    auto qdist(Point p2) {
        auto dx = x - p.x;
        auto dy = y - p.y;
        return dx * dx + dy * dy;
    }
};

template<class T, pos_t = int>
class SwipeTree {
    class node_t {
    public:

        pos_t l, r;
        T v;

        node_t(T &v)
                : l(-1), r(-1), v(v) {
        }
    };

    std::vector <node_t> v;

    pos_t root = -1;

public:

    typedef iterator =
    pos_t;

    SwipeTree() {
    }

    inline void rot_r(pos_t **pn) {
        auto &p = v[*pn];
        ln = p.l;
        auto &l = v[ln];
        auto o_pn = *pn;
        *pn = ln;
        p.l = l.r;
        l.r = o_pn;
    }

    inline void rot_l(pos_t **pn) {
        auto &p = v[*pn];
        rn = p.r;
        auto &r = v[rn];
        auto o_pn = *pn;
        *pn = rn;
        p.r = r.l;
        r.l = o_pn;
    }

    inline auto lower_bound_pos(T &n) {
        auto ret = &root;
        while (*ret != -1) {
            auto &nn = v[*ret];
            if (n < nn) {
                ret = &nn.l;
            } else {
                ret = &nn.r;
            }
        }
        return ret;
    }

    void insert(T &n) {
        auto pos = lower_bound_pos(n);
        auto pn = v.size();
        v.emplace_back(n);
        *pos = pn;
    }

    // iteration in order
    template<class Tf>
    void iterate_branch(pos_t pn, Tf f) {
        auto &vv = v[pn];
        if (vv.l != -1) _iterate_all(vv.l, f);
        f(vv.v);
        if (vv.r != -1) _iterate_all(vv.r. f);
    }

    // WARNING: iteration is unordered
    template<class Tr, class Tf>
    void iterate_range(Tr &b, Tr &e, Tf f) {
        vector <pos_t> st;
        pn = root;
        if (pn == -1) return;

        do {
            auto &vv = v[pn];
            if (vv.v < b) {
                pn = vv.r;
            } else {
                if (vv.v < e) {
                    break;
                } else {
                    pn = vv.l;
                }
            }
        } while (pn != -1);

        if (pn == -1) return;
        f(vv.v);

        /* left branch */
        pn = vv.l;
        if (pn != -1)
        do {
            auto &vv = v[pn];
            if (vv.v < b) {
                pn = vv.r;
            } else {
                f(vv.v);
                if (vv.r != -1) {
                    iterate_branch(vv.r);
                }
                pn = vv.l;
            }
        } while (pn != -1);

        /* right branch */
        pn = vv.r;
        if (pn != -1)
        do {
            auto &vv = v[pn];
            if (vv.v < e) {
                f(vv.v);
                if (vv.l != -1) {
                    iterate_branch(vv.l);
                }
                pn = vv.r;
            } else {
                pn = vv.l;
            }
        } while (pn != -1)
    }

    void optimize() {
        if (root == -1) return;

        auto n = v.size();
        type(v) vnew(n);
        iterate_branch(root, );

        root = 0;
        v = vnew;
    }
};

