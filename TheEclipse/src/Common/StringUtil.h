//==============================================================================
// StringUtil.h : 文字列ユーティリティ
//==============================================================================
#pragma once

#include <string>

namespace ecl {
namespace str {

// printf 形式のフォーマット
std::string Format(const char* fmt, ...);
// 1234567 -> "1,234,567"
std::string Comma(int value);
// 秒数 -> "01:23.45"
std::string TimeText(float seconds);
// 符号付き表記 "+12" / "-3"
std::string Signed(int value);

} // namespace str
} // namespace ecl
