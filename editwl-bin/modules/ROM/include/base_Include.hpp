
#pragma once
#include <mod/mod_Module.hpp>
#include <twl/fmt/fmt_ROM.hpp>
#include <QString>

constexpr twl::Result ResultPlaceholder = 0xffff;

constexpr twl::ResultDescriptionEntry ResultDescriptionTable[] = {
    { ResultPlaceholder, "Placeholder" }
};

inline constexpr twl::Result GetResultDescription(const twl::Result rc, std::string &out_desc) {
    TWL_R_TRY(twl::GetResultDescription(rc, out_desc));

    for(twl::u32 i = 0; i < std::size(ResultDescriptionTable); i++) {
        if(ResultDescriptionTable[i].first.value == rc.value) {
            out_desc = ResultDescriptionTable[i].second;
            TWL_R_SUCCEED(); 
        }
    }

    TWL_R_FAIL(twl::ResultUnknownResult);
}
