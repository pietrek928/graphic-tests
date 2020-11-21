#ifndef __TRANSLATE_H_
#define __TRANSLATE_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <stdlib.h>

class File2Number {
    std::unordered_map<std::string, int> file2num;
    std::unordered_map<int, std::string> num2file;

    int last_n = 0;

    int add_priv(std::string f) {
        while (num2file.find(last_n) != num2file.end()) {
            last_n ++;
        }

        auto n = last_n++;
        file2num[f] = n;
        num2file[n] = f;

        return n;
    }

    std::string simplify_path(std::string &p) {
        if (p.length() && p[0] == '!') {
            return p;
        }
        char simplified[PATH_MAX];
        ASSERT(realpath(p.c_str(), simplified) != NULL, "Simplifying path failed.");
        return simplified;
    }

    public:
    File2Number() {}

    void add(std::string v, int n) {
        v = simplify_path(v);
        auto fn = num2file.find(n);
        ASSERT(fn == num2file.end(), "Number already mapped", n, "->", fn->second);
        auto fv = file2num.find(v);
        ASSERT(fv == file2num.end(), "Path already mapped", v, "->", fv->second);
        file2num[v] = n;
        num2file[n] = v;
    }

    int get(std::string v) {
        v = simplify_path(v);
        auto f = file2num.find(v);
        if (f != file2num.end()) {
            return f->second;
        } else {
            return add_priv(v);
        }
    }

    auto get(std::vector<std::string> &v) {
        std::vector<int> r(v.size());
        std::transform(
            v.begin(), v.end(), r.begin(),
            [=](std::string &vv){return get(vv);}
        );
        return r;
    }

    std::string get(int n) {
        return num2file[n];
    }
};

#endif /* __TRANSLATE_H_ */

