#ifndef CHANNEL_BLOCK_STRATEGY_H
#define CHANNEL_BLOCK_STRATEGY_H

#include <cstdint>

namespace xtd
{
    enum class block_strategy : std::uint8_t
    {
        WAIT,
        TRY
    };

    enum class channel_read_errors {
        REQUEST_CANCELLED = 1,
        CHANNEL_EMPTY = 2,
    };
} // namespace xtd

#endif // CHANNEL_BLOCK_STRATEGY_H
