#ifndef __UTILS_H_
#define __UTILS_H_

#include <cstdint>

#define __STRINGIFY__(v) #v
#define STRINGIFY(v) __STRINGIFY__(v)

#define __CONCAT__(x, y) x ## y
#define CONCAT(x, y) __CONCAT__(x, y)
#define __CONCAT__3(x, y, z) x ## y ## z
#define CONCAT3(x, y, z) __CONCAT__3(x, y, z)

#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)

// TODO: something with flatten
// TODO: something with pure
#ifdef __clang__

#define INLINE_WRAPPER inline __attribute__((optimize("-Os"), always_inline, artificial))
#define RARE_FUNC __attribute__((minsize, cold))
#define FREQUENT_FUNC __attribute__((hot))

#elif __GNUC__

#define INLINE_WRAPPER inline __attribute__((always_inline, artificial))
#define RARE_FUNC __attribute__((optimize("-Os"), cold))
#define FREQUENT_FUNC __attribute__((optimize("-Ofast"), hot))

#else

#define INLINE_WRAPPER inline __attribute__((always_inline, artificial))
#define RARE_FUNC __attribute__((optimize("-Os"), minsize, cold))
#define FREQUENT_FUNC __attribute__((optimize("-Ofast"), hot))

#endif


typedef uint8_t byte;


/**********  LOGGING  ************/
#include <ctime>
#include <sys/time.h>
#include <iomanip>
#include <iostream>

enum class LogLevel {
    NOTSET = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    WARN = WARNING,
    ERROR = 4,
    CRITICAL = 5,
    FATAL = CRITICAL,
};

#define BASH_ESC_CHAR "\x1b"
#define BASH_SET_COLOR(a, b) BASH_ESC_CHAR "[" #a ";" #b "m"
#define BASH_CLEAR_COLOR BASH_ESC_CHAR "[0m"

RARE_FUNC
auto log_level_to_colored_name(LogLevel l) {
    switch(l) {
        case LogLevel::NOTSET: return "?";
        case LogLevel::DEBUG: return BASH_SET_COLOR(1, 90) "dbg" BASH_CLEAR_COLOR;
        case LogLevel::INFO: return BASH_SET_COLOR(1, 34) "info" BASH_CLEAR_COLOR;
        case LogLevel::WARNING: return BASH_SET_COLOR(1, 93) "warn" BASH_CLEAR_COLOR;
        case LogLevel::ERROR: return BASH_SET_COLOR(1, 31) "err" BASH_CLEAR_COLOR;
        case LogLevel::CRITICAL: return BASH_SET_COLOR(1, 91) "critical" BASH_CLEAR_COLOR;
        default: return "";
    }
}

class NopStream {
public:

    template<class T>
    inline auto &operator<<(T v) const {
        return *this;
    }
};
const auto NOP_STREAM = NopStream();

template<bool visible>
RARE_FUNC
auto &start_log_line_stream(LogLevel level, const char *channel_name) {
    if constexpr (visible) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm *lt = localtime(&tv.tv_sec);
        char tm_s[24];
        strftime(tm_s, sizeof(tm_s), "%F %X", lt);
        return std::cerr << tm_s << "." << std::setfill('0') << std::setw(3) << tv.tv_usec/1000
            << " " << log_level_to_colored_name(level) << " " << channel_name << ":: ";
    } else {
        return NOP_STREAM;
    }
}

#ifndef LOG_LEVEL
#define LOG_LEVEL DEBUG
#endif /* LOG_LEVEL */

#define DBG_ENDL "\n"
#define DBG_STREAM(channel) std::cerr << #channel << " :: "

#define LOG_ENDL "\n"
#define LOG(level, channel) start_log_line_stream<LogLevel::level >= LogLevel::LOG_LEVEL && LogLevel::level >= CHANNEL_LOG_LEVEL_ ## channel>( \
        LogLevel::level, #channel)

#define DBG(channel) LOG(DEBUG, channel)
#define INFO(channel) LOG(INFO, channel)
#define WARN(channel) LOG(WARNING, channel)
#define ERR(channel) LOG(ERROR, channel)
#define CRIT(channel) LOG(CRITICAL, channel)

/**********  ASSERT  ***********/
#include <string>

namespace std {
    string to_string(const char *s) {
        return s;
    }
    string to_string(string &s) {
        return s;
    }
}
template<const char *sep>
RARE_FUNC std::string __dbg_args_to_string__() {
    return "";
}
template<const char *sep, class Tfirst, class ... Targs>
RARE_FUNC std::string __dbg_args_to_string__(Tfirst &first_arg, Targs&... args) {
    return std::to_string(first_arg) + sep + __dbg_args_to_string__<sep>(args...);
}
const char __colon_sep__[] = ":";
const char __space_sep__[] = " ";

#define ASSERT(cond, ...) {if (unlikely(!(cond))) { \
    auto line_num = __LINE__; \
    auto loc_str = __dbg_args_to_string__<__colon_sep__>(__FILE__, __FUNCTION__, line_num); \
    auto args_str = __dbg_args_to_string__<__space_sep__>(__VA_ARGS__); \
    throw std::runtime_error(__dbg_args_to_string__<__space_sep__>(loc_str, "Assert", "`" #cond "`", "failed:", args_str)); \
}}
#ifdef DEBUG
#define ASSERT_DBG(...) ASSERT(__VA_ARGS__)
#else /* DEBUG */
#define ASSERT_DBG(...) {}
#endif /* DEBUG */


/********** TUPLE FOREACH LOOP  ***********/
#include <tuple>

template<int N, typename Tp, class... Targs>
inline void for_each(Tp &p, std::tuple<Targs...> &t) {
    if constexpr(N < sizeof...(Targs)) {
        p.process(std::get<N>(t));
        for_each<(int)(N+1)>(p, t);
    }
}

template<typename Tp, class... Targs>
void for_each(Tp &p, std::tuple<Targs...> t) {
    for_each<0>(p, t);
}


#define ENDL "\n"

class EmptyObject {
};
static constexpr EmptyObject empty;

#endif /* __UTILS_H_ */

