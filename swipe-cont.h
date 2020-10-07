#include <limits>

#include <swipe-buckets.h>

template<class T, class pos_t = int>
class SwipeCont {
    vector<SwipeBuckets<T, pos_t>> ranges;
    vector<float> range_sizes;
    float range_size;

    public:

    SwipeCont(float range_size, vector<float> &range_sizes)
        : range_sizes(range_sizes) {
        for (auto &r : range_sizes) {
            ranges.emplace_back(range_size, pos_t(range_size / r));
        }
        range_sizes.push_back(
            std::numeric_limits<int>::max()
        );
        ranges.emplace_back(range_size, 16);
    }

    template<class Tf>
    void iterate(Tf proc_f, float b, float e) {
        for (auto &r : ranges) {
            r.iterate(proc_f, b, e);
        }
    }

    void insert(T &v) {
        int p = 0;
        while (range_sizes[p] > v.x_pos()) p++;
        ranges[p].insert(v);
    }

    void optimize(float cur_y) {
        for (auto &r : ranges) {
            auto np = cur_y + r.get_bucket_size();
            r.optimize([&, np](auto &v) {
                return v.y_pos_end() < np;
            });
        }
    }
};

