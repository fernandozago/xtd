#ifndef CHANNEL_BLOCK_STRATEGY_H
#define CHANNEL_BLOCK_STRATEGY_H

#include <cstdint>

enum class block_strategy : std::uint8_t
{
    WAIT,
    TRY
};
#endif // CHANNEL_BLOCK_STRATEGY_H