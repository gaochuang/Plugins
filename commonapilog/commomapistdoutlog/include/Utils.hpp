#ifndef COMMON_API_STDOUT_LOGGER_UTILS_HPP_
#define COMMON_API_STDOUT_LOGGER_UTILS_HPP_

#include <charconv>
#include <string_view>
#include <type_traits>
#include <string>
#include <system_error>

namespace commonapistdoutlogger
{
    template<typename IntegerType>
    [[nodiscard]] bool stringToInt(std::string_view str, IntegerType& ret)
    {
        static_assert(std::is_integral_v<IntegerType>);

        const auto end{str.data() + str.size()};

        const auto [ptr, error] = std::from_chars(str.data(), end, ret);
        return (error == std::errc()) && (ptr == end);
    }

    bool isEquals(const std::string& a, const std::string& b);

    std::string getLogHostname();

    std::string getLogFqd();
}

#endif
