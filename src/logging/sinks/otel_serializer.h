#ifndef OTEL_SERIALIZER_H
#define OTEL_SERIALIZER_H

#include <array>
#include <chrono>
#include <cstdio>
#include <format>
#include <string>
#include <string_view>

#include "../log_message.h"

namespace otel_serializer {

static constexpr std::array<std::string_view, 6> severity_texts {
    "TRACE", "DEBUG", "INFO",
    "WARN", "ERROR", "FATAL"
};

static constexpr int to_otel_severity(log_level level) {
    switch (level) {
        case log_level::trace:       return 1;
        case log_level::debug:       return 5;
        case log_level::information: return 9;
        case log_level::warning:     return 13;
        case log_level::error:       return 17;
        case log_level::critical:    return 21;
        default:                     return 0;
    }
}

static std::string escape_json(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (char c : value) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b";  break;
            case '\f': escaped += "\\f";  break;
            case '\n': escaped += "\\n";  break;
            case '\r': escaped += "\\r";  break;
            case '\t': escaped += "\\t";  break;

            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buffer[7];
                    std::snprintf(
                        buffer,
                        sizeof(buffer),
                        "\\u%04x",
                        static_cast<unsigned char>(c));

                    escaped += buffer;
                }
                else {
                    escaped += c;
                }
        }
    }

    return escaped;
}

[[maybe_unused]]
static std::string serialize(const log_message& message) {
    const auto time_nanos =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            message.timestamp().time_since_epoch()).count();

    std::string json;
    json.reserve(512);

    json += std::format(
        "{{"
            "\"timeUnixNano\":\"{}\","
            "\"severityNumber\":{},"
            "\"severityText\":\"{}\","
            "\"body\":{{\"stringValue\":\"{}\"}},"
            "\"attributes\":[",
        time_nanos,
        to_otel_severity(message.level()),
        severity_texts[static_cast<std::size_t>(message.level())],
        escape_json(message.get_formatted_message()));

    bool first_attribute = true;

    const auto append_attribute = [&](
        std::string_view key,
        std::string_view value_json) {

        if (!first_attribute) {
            json += ',';
        }

        first_attribute = false;

        json += std::format(
            "{{\"key\":\"{}\",\"value\":{}}}",
            key,
            value_json);
    };

    append_attribute(
        "log.format",
        std::format(
            "{{\"stringValue\":\"{}\"}}",
            escape_json(message.format())));

    const auto formatted_args = message.get_formatted_args();

    if (!formatted_args.empty()) {
        std::string arguments =
            "{\"kvlistValue\":{\"values\":[";

        for (std::size_t index = 0; index < formatted_args.size(); ++index) {
            if (index != 0) {
                arguments += ',';
            }

            arguments += std::format(
                "{{"
                    "\"key\":\"arg{}\","
                    "\"value\":{{\"stringValue\":\"{}\"}}"
                "}}",
                index,
                escape_json(formatted_args[index]));
        }

        arguments += "]}}";

        append_attribute(
            "log.args",
            arguments);
    }

    append_attribute(
        "code.file.path",
        std::format(
            "{{\"stringValue\":\"{}\"}}",
            escape_json(message.location().file_name())));

    append_attribute(
        "code.line.number",
        std::format(
            "{{\"intValue\":\"{}\"}}",
            message.location().line()));

    append_attribute(
        "code.function.name",
        std::format(
            "{{\"stringValue\":\"{}\"}}",
            escape_json(message.location().function_name())));

    json += "]}";

    return json;
}

} // namespace otel_serializer

#endif // OTEL_SERIALIZER_H