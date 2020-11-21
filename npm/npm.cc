#include <vector>
#include <string>
#include <thread>

#include <utils.h>
#include <lock.h>
#include <change_monitor.h>
#include <targets.h>
#include <manager.h>

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

    return 0;
}


