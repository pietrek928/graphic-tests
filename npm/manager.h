#ifndef __MANAGER_H_
#define __MANAGER_H_

#include <tuple>

#include <lock.h>
#include <utils.h>

#include <dependency_tracker.h>
#include <translate.h>

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

#endif /* __MANAGER_H_ */

