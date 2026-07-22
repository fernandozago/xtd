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
} // namespace xtd

#endif // CHANNEL_BLOCK_STRATEGY_H