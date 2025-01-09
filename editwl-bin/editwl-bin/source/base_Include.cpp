#include <base_Include.hpp>

namespace {

    std::vector<std::pair<twl::Result, QString>> g_ResultDescriptionTable;

}

QString FormatResult(const twl::Result rc) {
    for(const auto &[t_rc, t_desc]: g_ResultDescriptionTable) {
        if(rc.value == t_rc.value) {
            return t_desc;
        }
    }

    return QString("unknown result: 0x%1").arg(rc.value, 0, 16);
}

void InitializeResults() {
    // Push libeditwl base results
    RegisterResultDescriptionTable(twl::ResultDescriptionTable, std::size(twl::ResultDescriptionTable));

    // Push our results
    RegisterResultDescriptionTable(ResultDescriptionTable, std::size(ResultDescriptionTable));
}

void RegisterResultDescriptionTable(const twl::ResultDescriptionEntry *table, const size_t table_size) {
    for(twl::u32 i = 0; i < table_size; i++) {
        g_ResultDescriptionTable.push_back(table[i]);
    }
}
