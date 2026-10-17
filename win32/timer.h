#pragma once

#include <algorithm>
#include <cstdint>

namespace timer {
constexpr unsigned maxSeconds = 99 * 3600 + 59 * 60 + 59;

inline unsigned duration(long long hours, long long minutes, long long seconds) {
    return static_cast<unsigned>(std::clamp(hours * 3600 + minutes * 60 + seconds,
                                           0LL, static_cast<long long>(maxSeconds)));
}

// Round up so the display does not reach zero before the deadline.
inline unsigned secondsLeft(std::uint64_t deadline, std::uint64_t now) {
    return now >= deadline ? 0 : static_cast<unsigned>((deadline - now + 999) / 1000);
}

inline unsigned progress(unsigned remaining, unsigned total) {
    return total == 0 ? 0 : static_cast<unsigned>(
        static_cast<std::uint64_t>(std::min(remaining, total)) * 10000 / total);
}
}
