
#pragma once
#include <twl/fmt/fmt_Common.hpp>
#include <twl/fmt/nfs/nfs_NitroFs.hpp>

namespace twl::fmt {

    struct Utility : public fs::FileFormat, public nfs::NitroFileSystemFormat {

        struct Header {
            u32 fnt_offset;
            u32 fnt_size;
            u32 fat_offset;
            u32 fat_size;
        };

        static constexpr size_t SectionAlignment = 0x20;

        Header header;

        Utility() {}
        Utility(const Utility&) = delete;
        Utility(Utility&&) = default;

        Result ReadValidateFrom(fs::File &rf) override;
        Result ReadAllFrom(fs::File &rf) override;
        Result WriteTo(fs::File &wf) override;
    };

}
