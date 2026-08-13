#ifndef OTEL_SERIALIZER_H
#define OTEL_SERIALIZER_H

#include <array>
#include <chrono>
#include <format>
#include <string>
#include <string_view>

#include "../log_message.h"

namespace otel_serializer {

static constexpr std::array<std::string_view, 6> severity_texts {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static std::string escape_json(std::string_view value)
{
    std::string result;
    result.reserve(value.size());

    for (const char c : value) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b";  break;
            case '\f': result += "\\f";  break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;

            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    result += std::format(
                        "\\u{:04x}",
                        static_cast<unsigned char>(c));
                }
                else {
                    result += c;
                }
        }
    }

    return result;
}

inline static std::string serialize(const log_message& message)
{
    const auto level = static_cast<std::size_t>(message.level());

    const auto time_nanos =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            message.timestamp().time_since_epoch()).count();

    std::string json = std::format(
        "{{"
            "\"timeUnixNano\":\"{}\","
            "\"severityNumber\":{},"
            "\"severityText\":\"{}\","
            "\"body\":{{\"stringValue\":\"{}\"}},"
            "\"attributes\":[",
        time_nanos,
        1 + level * 4,
        severity_texts[level],
        escape_json(message.get_formatted_message()));

    bool first = true;

    const auto attribute = [&](std::string_view key, std::string_view value) {
        if (!first) json += ',';
        first = false;
        json += std::format("{{\"key\":\"{}\",\"value\":{}}}", key, value);
    };

    attribute("log.format",
        std::format("{{\"stringValue\":\"{}\"}}",
            escape_json(message.format())));

    const auto args = message.get_formatted_args();

    if (!args.empty()) {
        std::string values;

        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i) values += ',';

            values += std::format("{{\"key\":\"arg{}\",\"value\":{{\"stringValue\":\"{}\"}}}}",
                i, escape_json(args[i]));
        }

        attribute("log.args",
            std::format("{{\"kvlistValue\":{{\"values\":[{}]}}}}", values));
    }

    attribute("code.file.path",
        std::format("{{\"stringValue\":\"{}:{}\"}}",
            escape_json(message.location().file_name()),
            message.location().line()));

    attribute("code.function.name",
        std::format("{{\"stringValue\":\"{}\"}}",
            escape_json(message.location().function_name())));

    json += "]}\n";

    return json;
}

} // namespace otel_serializer

#endif