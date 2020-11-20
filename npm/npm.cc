#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <unordered_set>
#include <set>
#include <regex>
#include <ranges>
#include <thread>

#include<sys/inotify.h>
#include<unistd.h>
#include<fcntl.h> // library for fcntl function

#include "a a.h"
#include "ooooooooooooooooooo.h"
#include "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb.h"
#include <utils.h>
#include <lock.h>

constexpr auto CHANNEL_LOG_LEVEL_files = LogLevel::DEBUG;
constexpr auto CHANNEL_LOG_LEVEL_compile = LogLevel::DEBUG;

constexpr auto noexisting_sequence = "___a___XxX___a___";


auto file_regex = std::regex("");
auto escaped_endl_regex = std::regex("\\\\\n");
auto escaped_space_regex = std::regex("\\\\ ");
auto space_regex = std::regex(" ");
auto double_space_regex = std::regex("  ");
auto noexisting_sequence_regex = std::regex(noexisting_sequence);

auto parse_compile_deps(std::string result) {
    result = std::regex_replace(result, escaped_endl_regex, "");
    result = std::regex_replace(result, escaped_space_regex, noexisting_sequence);
    result = std::regex_replace(result, double_space_regex, " ");
    result = std::regex_replace(result, space_regex, "\n");
    result = std::regex_replace(result, noexisting_sequence_regex, " ");

    std::vector<std::string> deps;
    int last_pos = 0, pos = 0;
    while (pos < result.length()) {
        auto c = result[pos];
        if (c == '\n') {
            auto s = result.substr(last_pos, pos-last_pos);
            if (s.length() && s.back() != ':') {
                deps.push_back(s);
            }
            last_pos = pos+1;
        }
        pos++;
    }

    return deps;
}

class compile_error : public std::exception {
    const char * what () const throw () {
    	return "Compilation error";
    }
};

auto run_compilation(std::vector<std::string> &cmd) {

    std::string cmd_s;
    for (auto &w : cmd) {
        cmd_s += " \"";
        cmd_s += w;
        cmd_s += "\"";
    }
    cmd_s += " 2>&1";
    DBG(compile) << "Running command: " << cmd_s << DBG_ENDL;

    FILE* pipe = popen(cmd_s.c_str(), "r");
    ASSERT(pipe, "Creating process failed");

    std::string stdout_result;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        //std::cout << "Reading..." << std::endl;
        stdout_result += buffer;
    }

    auto return_code = pclose(pipe);
    if (return_code) {
        ERR(compile) << "Compilation returned " << return_code << DBG_ENDL << stdout_result << DBG_ENDL;
        throw compile_error();
    }

    return stdout_result;
}

class CompileDependencyTracker : LockObject {
    std::unordered_map<int, std::vector<int>> reverse_dependency_graph;
    std::unordered_map<int, int> dep_cnt;
    std::unordered_set<int> to_compile;
    std::unordered_set<int> generated_obj;
    std::unordered_set<int> failed_obj;

    void add_to_compile(int v) {
        to_compile.insert(v);
        notify();
    }

    template<bool from_source>
    void inc_obj_dep(int v) {
        auto f = dep_cnt.find(v);
        if (f == dep_cnt.end()) {
            dep_cnt[v] = from_source ? 1 : 2;
            if (from_source) {
                add_to_compile(v);
            }
        } else if (!f->second) {
            f->second = from_source ? 1 : 2;
            if (from_source) {
                add_to_compile(v);
            }
        } else {
            if (!from_source) {
                f->second ++;
            }
            if (f->second <= 1
                    && failed_obj.find(v) != failed_obj.end()) {
                add_to_compile(v);
            }
            return;
        }
        for (auto &vv : reverse_dependency_graph[v]) {
            inc_obj_dep<false>(vv);
        }
    }

    template<bool start>
    void dec_dep(int v) {
        auto f = dep_cnt.find(v);
        if (f == dep_cnt.end() || !f->second) {
            if (!start) {
                add_to_compile(v);
            }
            return;
        }
        f->second --;
        if (f->second <= 0) {
            dep_cnt.erase(f);
            for (auto &vv : reverse_dependency_graph[v]) {
                dec_dep<false>(vv);
            }
        } else if (f->second == 1) {
                add_to_compile(v);
        }
    }

    public:

    void changed(int v) {
        auto l = lock();

        if (generated_obj.find(v) == generated_obj.end()) {
            for (auto &vv : reverse_dependency_graph[v]) {
                inc_obj_dep<true>(vv);
            }
        } else {
            // TODO: do sth ? recompile ???
        }
        for (auto const &it : dep_cnt) {
            DBG(files) << it.first << " " << it.second << DBG_ENDL;
        }
    }

    int compile_pop() {
        auto l = lock();

        auto b = to_compile.begin();

        while (b == to_compile.end()) {
            wait();
            b = to_compile.begin();
        }

        auto r = *b;
        to_compile.erase(b);
        return r;
    }

    void compile_fail(int v) {
        auto l = lock();

        failed_obj.insert(v);
    }

    void compile_success(int v) {
        auto l = lock();

        failed_obj.erase(v);
        dec_dep<true>(v);
    }

    void update_deps(int v, std::vector<int> old_deps, std::vector<int> new_deps) {
        auto l = lock();

        if (new_deps.size()) {
            generated_obj.insert(v);
        } else {
            generated_obj.erase(v);
        }

        std::sort(old_deps.begin(), old_deps.end());
        std::sort(new_deps.begin(), new_deps.end());

        std::vector<int> add_deps(std::max(old_deps.size(), new_deps.size()));
        auto it = std::set_difference(
            new_deps.begin(), new_deps.end(),
            old_deps.begin(), old_deps.end(),
            add_deps.begin()
        );
        add_deps.resize(it-add_deps.begin());
        for (auto &d: add_deps) {
            reverse_dependency_graph[d].push_back(v);
            inc_obj_dep<true>(v);
        }

        std::vector<int> rm_deps(std::max(old_deps.size(), new_deps.size()));
        it = std::set_difference(
            old_deps.begin(), old_deps.end(),
            new_deps.begin(), new_deps.end(),
            rm_deps.begin()
        );
        rm_deps.resize(it-rm_deps.begin());
        for (auto &d: rm_deps) {
            auto &deps_vec = reverse_dependency_graph[d];
            deps_vec.erase(std::remove(deps_vec.begin(), deps_vec.end(), v), deps_vec.end());
            
            auto f = dep_cnt.find(d);
            if (f != dep_cnt.end() && f->second) {
                dec_dep<false>(v);
            }
        }
    }
};

class FromSourceTarget {
public:

    std::string target;
    std::string source;
    std::vector<std::string> options;
    std::vector<std::string> libs;
    std::vector<std::string> include_dirs;

    FromSourceTarget(
        std::string target, std::string source,
        std::vector<std::string> options,
        std::vector<std::string> include_dirs,
        std::vector<std::string> libs
    ) : target(target), source(source),
        options(options), include_dirs(include_dirs),
        libs(libs) {}

    void add_compile_opts(std::vector<std::string> &v) {
        v.insert(v.end(), options.begin(), options.end());
    }

    void add_libs(std::vector<std::string> &v) {
        for (auto &l : libs) {
            v.push_back("-l" + l);
            DBG(compile) << l << DBG_ENDL;
        }
    }

    void add_include_dirs(std::vector<std::string> &v) {
        for (auto &i : include_dirs) {
            v.push_back("-I");
            v.push_back(i);
        }
    }

    void add_show_deps_opts(std::vector<std::string> &v) {
        v.push_back("-MM");
        v.push_back("-MF");
        v.push_back("-");
    }

    void add_sources(std::vector<std::string> &v) {
        v.push_back(source);
    }

    FromSourceTarget() {
    }

    auto find_deps() {
        std::vector<std::string> cmd = {
            "g++"
        };
        add_compile_opts(cmd);
        add_sources(cmd);
        add_show_deps_opts(cmd);
        add_include_dirs(cmd);

        auto result = run_compilation(cmd);
        auto deps = parse_compile_deps(result);

        std::string dbg_msg = "Dependencies of " + target + ": ";
        for (auto &d : deps) {
            dbg_msg += "'" + d + "', ";
        }
        DBG(compile) << dbg_msg << DBG_ENDL;

        return deps;
    }

    auto compile() {
        std::vector<std::string> cmd = {
            "g++"
        };
        add_compile_opts(cmd);
        add_libs(cmd);
        add_sources(cmd);
        add_include_dirs(cmd);

        cmd.push_back("-o");
        cmd.push_back(target);

        return run_compilation(cmd);
    }

    void clear() {
        unlink(target.c_str());
    }
};

template<class Taction>
class ActionTarget {
    public:

    Taction action;
    std::string source;
    std::string target;

    ActionTarget(std::string source, std::string target, Taction action)
        : source(source), target(target), action(action) {}

    std::vector<std::string> find_deps() {
        return {source};
    }

    auto compile() {
        return action();
    }

    void clear() {
    }
};


class ChangeMonitor : LockObject {
    constexpr static auto MAX_EVENTS = 1024;  /* Maximum number of events to process*/
    constexpr static auto LEN_NAME = 16;  /* Assuming that the length of the filename
won't exceed 16 bytes*/
    constexpr static auto EVENT_SIZE = ( sizeof (struct inotify_event) ); /*size of one event*/
    constexpr static auto BUF_LEN = ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) ;

    int fd;
    std::map<int, std::string> watch2path;
    std::map<std::string, int> path2watch;
    std::map<std::string, time_t> last_changed;

    public:
    ChangeMonitor() {
        fd = inotify_init();
        ASSERT_SYS(fd);
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    }

    void add(std::string path) {
        auto l = lock();

        auto wd = inotify_add_watch(fd, path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
        ASSERT_SYS(wd, path);
        path2watch[path] = wd;
        watch2path[wd] = path;
    }

    void remove(std::string path) {
        auto l = lock();

        auto wd = path2watch[path];
        ASSERT(inotify_rm_watch(fd, wd) != -1, "Removing watch failed", path);
        path2watch.erase(path);
        watch2path.erase(wd);
    }

    time_t get_time() {
        struct timespec tm;
        clock_gettime(CLOCK_MONOTONIC, &tm);
        return ((time_t)tm.tv_sec) * 1000000000 + tm.tv_nsec;
    }

    void fd_wait(int fd, float timeout_s) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1e6f * timeout_s;

        ASSERT_SYS(select(fd + 1, &set, NULL, NULL, &timeout), "Select failed");
    }

    template<class Thandler>
    void listen(Thandler change_handler) {
        char buffer[BUF_LEN];
        auto cur_time = get_time();
        constexpr time_t debounce_t = 0.4f * 1e9f;

        while (1) {
            /* wait for change timestamp */
            float wait_t = 256.0f;
            for (auto c = last_changed.begin(); c != last_changed.end(); ) {
                auto diff_t = c->second - cur_time;
                if (diff_t <= 0) {
                    change_handler(c->first);
                    c = last_changed.erase(c);
                } else {
                    wait_t = std::min(wait_t, 1e-9f * (float)diff_t);
                    c++;
                }
            }

            fd_wait(fd, std::max(0.0f, wait_t));

            auto length = read(fd, buffer, BUF_LEN);
            int i = 0;

            cur_time = get_time();
            while (i < length) {
                auto l = lock();
                struct inotify_event *event = (struct inotify_event *) &buffer[i];
                if (event->len) {
                    std::string full_name = watch2path[event->wd] + "/" + event->name;
                    if (event->mask & IN_ISDIR) {
                        full_name += "/";
                    }

                    if ( event->mask & IN_CREATE ) {
                        last_changed[full_name] = cur_time + debounce_t;
                    } else if ( event->mask & IN_DELETE ) {
                        // TODO: do something ?
                    } else if ( event->mask & IN_MODIFY ) {
                        last_changed[full_name] = cur_time + debounce_t;
                    }
                }
                i += EVENT_SIZE + event->len;
            }
        }
    }
};

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


