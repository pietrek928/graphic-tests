#include "utils.h"

constexpr auto CHANNEL_LOG_LEVEL_xD = LogLevel::DEBUG;

int main() {
    int a = 3333;
    DBG(xD) << "yoooooooooooooo" << LOG_ENDL;
    INFO(xD) << "yoooooooooooooo" << LOG_ENDL;
    WARN(xD) << "yoooooooooooooo" << LOG_ENDL;
    ERR(xD) << "yoooooooooooooo" << LOG_ENDL;
    CRIT(xD) << "yoooooooooooooo" << LOG_ENDL;
    ASSERT_DBG(1 > 2, "ERROR", a);

    return 0;
}

