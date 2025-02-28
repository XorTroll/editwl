
#pragma once
#include <twl/twl_Include.hpp>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <cstring>

namespace twl::util {

    inline std::string ConvertFromUnicode(const std::u16string &u_str) {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
        return convert.to_bytes(u_str);
    }

    inline std::u16string ConvertToUnicode(const std::string &str) {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
        return convert.from_bytes(str);
    }

    inline bool ConvertStringToNumber(const std::string &str, u32 &out_num, const u32 base = 10) {
        errno = 0;
        char *end_ptr;
        const auto res = std::strtoul(str.c_str(), &end_ptr, base);
        if((errno != 0) || (end_ptr == str.c_str())) {
            return false;
        }
        else {
            out_num = res;
            return true;
        }
    }

    template<size_t N, typename C>
    inline std::basic_string<C> GetNonNullTerminatedCString(const C (&c_str)[N]) {
        const auto actual_len = std::min(N, std::char_traits<C>::length(c_str));
        return std::basic_string<C>(c_str, actual_len);
    }

    template<size_t N, typename C>
    inline void SetNonNullTerminatedCString(C (&out_c_str)[N], const std::basic_string<C> &str) {
        const auto actual_len = std::min(N, str.length());
        std::memset(out_c_str, 0, N * sizeof(C));
        std::memcpy(out_c_str, str.c_str(), actual_len * sizeof(C));
    }

    inline std::string ToLowerString(const std::string &str) {
        auto lower_str = str;
        std::transform(str.begin(), str.end(), lower_str.begin(), ::tolower);
        return lower_str;
    }

    std::vector<std::string> SplitString(const std::string &str, const char separator);

}
