#ifndef __FPS_H_
#define __FPS_H_

#include <time.h>
#include <unistd.h>

class FPSController {
    time_t fps_dt;
    time_t sleep_t = 0;
    float avg_util = 0.0f;

    time_t last_time;

    time_t get_time() {
        struct timespec tm;
        clock_gettime(CLOCK_MONOTONIC, &tm);
        return tm.tv_sec * 1000000000 + tm.tv_nsec;
    }

    void update_stats() {
        const float a = 0.1;

        float util;
        if (sleep_t > 0) {
            util = 1.0f - ((float)sleep_t) / fps_dt;
        } else {
            util = 1.0f + ((float)-sleep_t) / fps_dt;
        }
        avg_util = avg_util * (1.0f - a) + util * a;
    }

    public:
    FPSController(float fps)
        : fps_dt(1000000000.0f / fps), last_time(get_time()) {
        if (fps < 1.0f) {
            throw std::runtime_error("Invalid fps value " + std::to_string(fps));
        }
    }

    void show_stats() {
        if (avg_util < 1.0f) {
            DBG_STREAM(fps) << "FPS: " << (1000000000.0f / fps_dt) << " Utilization: " << avg_util << ENDL;
        } else {
            DBG_STREAM(fps) << "FPS: " << (1000000000.0f / fps_dt) / avg_util << ENDL;
        }
    }

    void sleep() {
        auto new_time = get_time();
        auto dt = new_time - last_time;
        last_time = new_time;

        auto ch_dt = fps_dt - dt;
        if (ch_dt > 0) {
            if (sleep_t < 0) {
                sleep_t = ch_dt;
            } else {
                sleep_t += ch_dt;
            }
            usleep(sleep_t / 1000);
        } else {
            sleep_t = ch_dt;
        }
        update_stats();
    }
};

#endif /* __FPS_H_ */

