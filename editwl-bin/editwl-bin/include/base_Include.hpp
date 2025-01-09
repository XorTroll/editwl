
#pragma once
#include <twl/twl_Include.hpp>
#include <QString>

// Define our custom results, aside from the rest defined in libnedit

constexpr twl::Result ResultModuleLoadError = 0xd001;
constexpr twl::Result ResultInvalidModuleSymbols = 0xd002;
constexpr twl::Result ResultModuleInitializationFailure = 0xd003;

constexpr std::pair<twl::Result, const char*> ResultDescriptionTable[] = {
    { ResultModuleLoadError, "Error loading module" },
    { ResultInvalidModuleSymbols, "Invalid module symbols" },
    { ResultModuleInitializationFailure, "Unable to get module metadata" }
};

QString FormatResult(const twl::Result rc);

void InitializeResults();
void RegisterResultDescriptionTable(const twl::ResultDescriptionEntry *table, const size_t table_size);
