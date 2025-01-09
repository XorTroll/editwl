
#pragma once
#include <twl/twl_Include.hpp>

namespace twl::util {

    template<typename T>
	inline constexpr T AlignUp(const T value, const u64 align) {
		const auto inv_mask = align - 1;
		return static_cast<T>((value + inv_mask) & ~inv_mask);
	}

    template<typename T>
	inline constexpr bool IsAlignedTo(const T value, const u64 align) {
		return (value % align) == 0;
	}

}
