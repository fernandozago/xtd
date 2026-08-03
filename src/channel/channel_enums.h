#ifndef CHANNEL_ENUMS_H
#define CHANNEL_ENUMS_H

#include <cstdint>

namespace xtd
{
    enum class block_strategy : std::uint8_t
    {
        WAIT,
        TRY
    };

    enum class channel_read_errors : std::uint8_t {
        REQUEST_CANCELLED = 1,
        CHANNEL_EMPTY = 2,
    };
} // namespace xtd

#endif // CHANNEL_ENUMS_H
