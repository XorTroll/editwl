#include <twl/fmt/fmt_Utility.hpp>

#include <iostream>

namespace twl::fmt {

    Result Utility::ReadValidateFrom(fs::File &rf) {
        TWL_R_TRY(rf.Read(this->header));
        
        size_t f_size;
        TWL_R_TRY(rf.GetSize(f_size));

        if((this->header.fat_offset > f_size) || ((this->header.fat_offset + this->header.fat_size) > f_size)) {
            TWL_R_FAIL(ResultUtilityInvalidSections);
        }
        if((this->header.fnt_offset > f_size) || ((this->header.fnt_offset + this->header.fnt_size) > f_size)) {
            TWL_R_FAIL(ResultUtilityInvalidSections);
        }

        TWL_R_SUCCEED();
    }

    Result Utility::ReadAllFrom(fs::File &rf) {
        this->nitro_fs.Dispose();

        const auto file_data_offset = 0; // FAT offsets are absolute in this format
        TWL_R_TRY(this->nitro_fs.ReadFrom(rf, file_data_offset, this->header.fat_offset, this->header.fnt_offset));

        TWL_R_SUCCEED();
    }

    Result Utility::WriteTo(fs::File &rf) {
        fs::BufferReaderWriter file_data_rw(0);
        fs::BufferReaderWriter fnt_data_rw(0);

        ScopeGuard save([&]() {
            file_data_rw.Dispose();
            fnt_data_rw.Dispose();
        });

        std::vector<nfs::NitroFileSystem::DirectoryNameTableEntry> gen_fnt;
        std::vector<nfs::NitroFileSystem::FileAllocationTableEntry> gen_fat;
        TWL_R_TRY(this->nitro_fs.WriteTo(fnt_data_rw, file_data_rw, gen_fnt, gen_fat, 0x200));

        /*

        const auto fat_offset = util::AlignUp(this->GetFatEntriesOffset() + this->header.fat_size, 0x20);

        TWL_R_TRY(wf.SetAbsoluteOffset(sizeof(Header) + sizeof(FileAllocationTableBlock)));
        TWL_R_TRY(wf.WriteVector(gen_fat));
        TWL_R_TRY(wf.WriteEnsureAlignment(0x4));
        */

        TWL_R_SUCCEED();
    }

}
