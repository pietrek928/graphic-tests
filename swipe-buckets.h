#include <cstdio>
#include <vector>

template<class Ti>
class _it_range {
    Ti b, e;

    public:

        _it_range(Ti &b, Ti &e)
            : b(b), e(e) {
        }

        inline auto begin() {
            return b;
        }

        inline auto end() {
            return e;
        }
};

template<class Te, class pos_t = int>
inline auto range(std::vector<Te> &v, pos_t b, pos_t e) {
    auto vb = v.begin();
    auto bb = vb + b;
    auto ee = vb + e;
    return _it_range(bb, ee);
}

template<class T, class pos_t = int>
class SwipeBuckets {
    class node_t {
        public:
            T v;
            pos_t nxt;

            node_t(T &v)
                : v(v), nxt(-1) {
            }
    };

    std::vector<pos_t> vbuckets;
    std::vector<node_t> vnodes;
    float bucket_size;

    pos_t bucket_n(float pos) {
        return std::min((pos_t)(pos / bucket_size), (pos_t)vbuckets.size());
    }

    template<class Tf>
    void reinit_nodes(Tf filter_func, pos_t nbuckets) {
        auto old_nodes = std::move(vnodes);
        vbuckets.assign(nbuckets, -1);
        auto bucket_ends = vbuckets;
        for (auto &n : old_nodes) {
            if (filter_func(n.v)) {
                pos_t p = vnodes.size();
                vnodes.emplace_back(n.v);

                auto bucket = bucket_n(n.v.x_pos());
                auto ep = bucket_ends[bucket];
                if (ep == -1) {
                    vbuckets[bucket] = p;
                } else {
                    vnodes[ep].nxt = p;
                }
                bucket_ends[bucket] = p;
            }
        }
    }

public: 

    template<class Tf>
    void iterate(Tf proc_f, float b, float e) {
        auto start_bucket = bucket_n(b);
        auto end_bucket = bucket_n(e + bucket_size);
        //std::cout << start_bucket << " " << end_bucket << std::endl;
        for (auto &pos : range(vbuckets, start_bucket, end_bucket)) {
            if (pos != -1) {
                do {
                    auto &n = vnodes[pos];
                    proc_f(n.v);
                    pos = n.nxt;
                } while (pos != -1);
            }
        }
    }

    void insert(T &v) {
        pos_t new_pos = vnodes.size();
        vnodes.emplace_back(v);
        auto bucket = bucket_n(v.x_pos());
        pos_t p = vbuckets[bucket];
        if (p == -1) {
            vbuckets[bucket] = new_pos;
            return;
        } else {
            node_t *n;
            do {
                n = & vnodes[p];
                p = n->nxt;
            } while (p != -1);
            n->nxt = new_pos;
            return;
        }
    }

    template<class Tf>
    void optimize(Tf filter_func, pos_t nbuckets) {
        vbuckets.resize(nbuckets);
        bucket_size = range_size / nbuckets;

        reinit_nodes(filter_func, nbuckets);
    }

    template<class Tf>
    void optimize(Tf filter_func) {
        reinit_nodes(filter_func, vbuckets.size());
    }

    SwipeBuckets(float bucket_size, pos_t nbuckets)
        : bucket_size(bucket_size) {
        optimize([](auto &v){return true;}, nbuckets);
    }

    auto get_bucket_size() {
        return bucket_size;
    }
};

