#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <unordered_set>
#include <set>
#include <ranges>
#include <thread>

#include "a a.h"
#include "ooooooooooooooooooo.h"
#include "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb.h"
#include <utils.h>
#include <lock.h>
#include <dependency_tracker.h>
#include <change_monitor.h>
#include <targets.h>


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

template<class Ttargets_tuple>
class TargetManager : LockObject {
    CompileDependencyTracker dependency_tracker;
    Ttargets_tuple tgs;
    File2Number ff;

    template<class T>
    auto translate(T v) {
        auto l = lock();
        return ff.get(v);
    }

    public:
    TargetManager(Ttargets_tuple tgs)
        : tgs(tgs) {
        std::unordered_map<int, std::vector<int>> deps;
        int i = 0;
        for_each([&](auto &t) {
            ff.add(t.target, i);
            i++;
        }, tgs);

        for_each([&](auto &t) {
            auto obj_n = translate(t.target);
            auto str_deps = t.find_deps();
            deps[obj_n] = translate(str_deps);
            dependency_tracker.update_deps(obj_n, {}, deps[obj_n]);
        }, tgs);
    }

    void run_compiler() {
        while (true) { // TODO: stop condition ?
            auto compile_n = dependency_tracker.compile_pop();
            for_one([&](auto &t) {
                try {
                    t.compile();
                    dependency_tracker.compile_success(compile_n);
                } catch (compile_error e) {
                    dependency_tracker.compile_fail(compile_n);
                }
            }, tgs, compile_n);
        }
    }

    void run_watcher(std::vector<std::string> &watched_dirs) {
        ChangeMonitor watcher;

        for (auto &d : watched_dirs) {
            watcher.add(d);
        }

        DBG(files) << "Change monitor started." << LOG_ENDL;
        watcher.listen([&](const std::string &obj_name) {
            DBG(files) << "Object changed " << obj_name << DBG_ENDL;
            auto obj_n = translate(obj_name);
            dependency_tracker.changed(obj_n);
        });
    }
};

template<class Ttargets_tuple>
auto init_targets(Ttargets_tuple tgs) {
    return TargetManager<Ttargets_tuple>(tgs);
}

void restart(char **argv) {
    INFO(compile) << "Restarting program..." << DBG_ENDL;
    //ASSERT_SYS(execv(argv[0], argv), "Restarting failed.");
}

int main(int argc, char **argv) {
    std::vector<std::string> compile_opts = {"-std=c++17", "-O2", "-s"};
    std::vector<std::string> common_include_dirs = {".", "..", "../mem"};
    auto mgr = init_targets(std::make_tuple(
        FromSourceTarget("npm.e", "npm.cc", compile_opts, common_include_dirs, {"pthread"}),
        ActionTarget("npm.e", "!compiler", [&](){
            restart(argv);
        })
    ));

    std::thread worker1([&](){
        mgr.run_compiler();
    });

    mgr.run_watcher(common_include_dirs);

    

    /*tt.update_deps(1, {}, {0, });
    tt.update_deps(1, {0, }, {});
    tt.update_deps(1, {}, {0, });
    DBG(files) << tt.compile_pop() << LOG_ENDL;
    tt.compile_success(1);
    //tt.compile_fail(1);
    tt.changed(0);
    DBG(files) << tt.compile_pop() << LOG_ENDL; */

    //restart(argv);

    //std::thread worker1([&](){
        
    //});
    //worker1.start();

    //t.find_deps();
    //t.compile();
    

    //worker1.join();

    return 0;
}


