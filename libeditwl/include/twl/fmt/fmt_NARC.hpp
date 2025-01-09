
#pragma once
#include <twl/fmt/fmt_Common.hpp>
#include <twl/fmt/nfs/nfs_NitroFs.hpp>

namespace twl::fmt {

    struct NARC : public fs::FileFormat, public nfs::NitroFileSystemFormat {

        struct Header : public CommonHeader<0x4352414E /* "NARC" */ > {

            static constexpr u16 SupportedByteOrder = 0xFFFE;
            static constexpr u16 SupportedVersion = 0x100;
        };

        struct FileAllocationTableBlock : public CommonBlock<0x46415442 /* "BTAF" */ > {
            u16 entry_count;
            u8 reserved[2];
        };

        struct FileNameTableBlock : public CommonBlock<0x464E5442 /* "BTNF" */ > {
        };

        struct FileImageBlock : public CommonBlock<0x46494D47 /* "GMIF" */ > {
        };

        static constexpr size_t SectionAlignment = 0x4;

        Header header;
        FileAllocationTableBlock fat;
        FileNameTableBlock fnt;
        FileImageBlock fimg;

        NARC() {}
        NARC(const NARC&) = delete;
        NARC(NARC&&) = default;
        
        Result ReadValidateFrom(fs::File &rf) override;
        Result ReadAllFrom(fs::File &rf) override;
        Result WriteTo(fs::File &wf) override;
    };

}
