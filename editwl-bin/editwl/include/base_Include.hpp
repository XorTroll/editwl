
#pragma once
#include <twl/twl_Include.hpp>
#include <QString>

// Define our custom results, aside from the rest defined in libeditwl

constexpr twl::Result ResultModuleLoadError = 0xd001;
constexpr twl::Result ResultInvalidModuleSymbols = 0xd002;
constexpr twl::Result ResultModuleInitializationFailure = 0xd003;

QString FormatResult(const twl::Result rc);
