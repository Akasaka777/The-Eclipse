#include "Common/StringUtil.h"

#include <cstdarg>
#include <cstdio>
#include <vector>

namespace ecl {
namespace str {

std::string Format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    va_list copy;
    va_copy(copy, args);
    const int needed = std::vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);

    std::string result;
    if (needed > 0) {
        std::vector<char> buffer(static_cast<size_t>(needed) + 1, '\0');
        std::vsnprintf(buffer.data(), buffer.size(), fmt, args);
        result.assign(buffer.data(), static_cast<size_t>(needed));
    }
    va_end(args);
    return result;
}

std::string Comma(int value)
{
    const bool negative = value < 0;
    long long v = negative ? -static_cast<long long>(value) : value;

    std::string digits = std::to_string(v);
    std::string out;
    int counter = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        out.push_back(digits[static_cast<size_t>(i)]);
        if (++counter % 3 == 0 && i > 0) out.push_back(',');
    }
    if (negative) out.push_back('-');
    return std::string(out.rbegin(), out.rend());
}

std::string TimeText(float seconds)
{
    if (seconds < 0.0f) seconds = 0.0f;
    const int total = static_cast<int>(seconds);
    const int minutes = total / 60;
    const int secs = total % 60;
    const int hundredths = static_cast<int>((seconds - static_cast<float>(total)) * 100.0f);
    return Format("%02d:%02d.%02d", minutes, secs, hundredths);
}

std::string Signed(int value)
{
    return value >= 0 ? Format("+%d", value) : Format("%d", value);
}

} // namespace str
} // namespace ecl
