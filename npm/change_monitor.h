#ifndef __CHANGE_MONITOR_H_
#define __CHANGE_MONITOR_H_

#include <string>
#include <map>

#include<sys/inotify.h>
#include<unistd.h>
#include<fcntl.h> // library for fcntl function

#include <utils.h>

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


#endif /* __CHANGE_MONITOR_H_ */

