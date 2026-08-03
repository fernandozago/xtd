#include <algorithm>
#include <cassert>
#include <cctype>
#include <string_view>
#include <tuple>
#include <array>
#include <string>
#include "pipeline/segmented_byte_view.h"

#define COMMAND_LIST(X) \
    X(name)             \
    X(help)             \
    X(quit)

enum class command_type {
    unknown,
    message,

    #define X(command) \
    command,
    COMMAND_LIST(X)
    #undef X
};

class commands_parser
{
private:
    struct command_definition {
        std::string_view name;
        command_type type;
    };

    static constexpr auto command_definitions = std::to_array<command_definition>
    ({
        #define X(command) \
        command_definition{#command, command_type::command},
        COMMAND_LIST(X)
        #undef X
    });

    inline static const std::string_view whitespace = " \t\n\r\f\v";

    static std::string_view trim(std::string_view value)
    {
        const auto first = value.find_first_not_of(whitespace);
        if (first == std::string_view::npos) {
            return {};
        }

        const auto last = value.find_last_not_of(whitespace);
        return value.substr(first, last - first + 1);
    }

    static bool is_equal_ignore_case(std::string_view a, std::string_view b)
    {
        if (a.size() != b.size()) {
            return false;
        }

        return std::equal(a.begin(), a.end(), b.begin(),
            [](unsigned char a, unsigned char b) {
                return std::tolower(a) == std::tolower(b);
            }
        );
    }

    static command_type parse_command_type(std::string_view command)
    {
        const auto it = std::ranges::find_if(command_definitions, 
            [command](const command_definition& definition) {
                return is_equal_ignore_case(command, definition.name);
            });

        return it == command_definitions.end() ? command_type::unknown : it->type;
    }

public:
    static std::tuple<command_type, std::string> parse_command(const xtd::segmented_byte_view& line_bytes)
    {
        assert(!line_bytes.empty());

        if (line_bytes[0] == std::byte('/')) {
            const xtd::position separator = line_bytes.position_of_any(whitespace);
            return {
                parse_command_type(line_bytes.slice(1, separator 
                    ? separator 
                    : line_bytes.end()).to_string()),

                std::string{separator 
                    ? trim(line_bytes.slice(separator + 1, line_bytes.end()).to_string()) 
                    : std::string{}}
            };
        }
        
        return {command_type::message, std::string{line_bytes.to_string()}};
    }
};