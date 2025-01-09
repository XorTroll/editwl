#include <twl/fmt/fmt_NARC.hpp>

namespace twl::fmt {

    Result NARC::ReadValidateFrom(fs::File &rf) {
        TWL_R_TRY(rf.Read(this->header));
        if(!this->header.IsValid()) {
            TWL_R_FAIL(ResultNARCInvalidHeader);
        }

        TWL_R_TRY(rf.Read(this->fat));
        if(!this->fat.IsValid()) {
            TWL_R_FAIL(ResultNARCInvalidFileAllocationTableBlock);
        }

        const auto fat_entries_size = this->fat.entry_count * sizeof(nfs::NitroFileSystem::FileAllocationTableEntry);
        TWL_R_TRY(rf.MoveOffset(fat_entries_size));

        TWL_R_TRY(rf.Read(this->fnt));
        if(!this->fnt.IsValid()) {
            TWL_R_FAIL(ResultNARCInvalidFileNameTableBlock);
        }

        const auto fimg_offset = this->header.header_size + this->fat.block_size + this->fnt.block_size;
        TWL_R_TRY(rf.SetAbsoluteOffset(fimg_offset));
        TWL_R_TRY(rf.Read(this->fimg));
        if(!this->fimg.IsValid()) {
            TWL_R_FAIL(ResultNARCInvalidFileImageBlock);
        }

        TWL_R_SUCCEED();
    }

    Result NARC::ReadAllFrom(fs::File &rf) {
        this->nitro_fs.Dispose();

        const auto fat_size = this->fat.entry_count * sizeof(nfs::NitroFileSystem::FileAllocationTableEntry);
        const auto fat_offset = sizeof(Header) + sizeof(FileAllocationTableBlock);
        const auto fnt_offset = fat_offset + fat_size + sizeof(FileNameTableBlock);
        const auto file_data_offset = this->header.header_size + this->fat.block_size + this->fnt.block_size + sizeof(FileImageBlock);
        TWL_R_TRY(this->nitro_fs.ReadFrom(rf, file_data_offset, fat_offset, fnt_offset));

        TWL_R_SUCCEED();
    }

    Result NARC::WriteTo(fs::File &wf) {
        fs::BufferReaderWriter file_data_rw(0);
        fs::BufferReaderWriter fnt_data_rw(0);

        ScopeGuard save([&]() {
            file_data_rw.Dispose();
            fnt_data_rw.Dispose();
        });

        std::vector<nfs::NitroFileSystem::DirectoryNameTableEntry> gen_fnt;
        std::vector<nfs::NitroFileSystem::FileAllocationTableEntry> gen_fat;
        TWL_R_TRY(this->nitro_fs.WriteTo(fnt_data_rw, file_data_rw, gen_fnt, gen_fat, 0x200));

        // FAT

        TWL_R_TRY(wf.SetAbsoluteOffset(sizeof(Header) + sizeof(FileAllocationTableBlock)));
        TWL_R_TRY(wf.WriteVector(gen_fat));
        TWL_R_TRY(wf.WriteEnsureAlignment(0x4));

        size_t fat_end_offset;
        TWL_R_TRY(wf.GetOffset(fat_end_offset));

        this->fat.EnsureMagic();
        this->fat.entry_count = gen_fat.size();
        this->fat.block_size = fat_end_offset - sizeof(Header);
        TWL_R_TRY(wf.SetAbsoluteOffset(sizeof(Header)));
        TWL_R_TRY(wf.Write(this->fat));

        // FNT

        TWL_R_TRY(wf.SetAbsoluteOffset(fat_end_offset + sizeof(FileNameTableBlock)));
        TWL_R_TRY(wf.WriteVector(gen_fnt));
        TWL_R_TRY(wf.WriteBuffer(fnt_data_rw.GetBuffer(), fnt_data_rw.GetBufferSize()));
        TWL_R_TRY(wf.WriteEnsureAlignment(0x4));

        size_t fnt_end_offset;
        TWL_R_TRY(wf.GetOffset(fnt_end_offset));

        this->fnt.EnsureMagic();
        this->fnt.block_size = fnt_end_offset - fat_end_offset;
        TWL_R_TRY(wf.SetAbsoluteOffset(fat_end_offset));
        TWL_R_TRY(wf.Write(this->fnt));

        // FIMG

        TWL_R_TRY(wf.SetAbsoluteOffset(fnt_end_offset + sizeof(FileImageBlock)));
        TWL_R_TRY(wf.WriteBuffer(file_data_rw.GetBuffer(), file_data_rw.GetBufferSize()));
        TWL_R_TRY(wf.WriteEnsureAlignment(0x4));

        size_t fimg_end_offset;
        TWL_R_TRY(wf.GetOffset(fimg_end_offset));

        this->fimg.EnsureMagic();
        this->fimg.block_size = fimg_end_offset - fnt_end_offset;
        TWL_R_TRY(wf.SetAbsoluteOffset(fnt_end_offset));
        TWL_R_TRY(wf.Write(this->fimg));

        // Finally, header

        size_t end_offset;
        TWL_R_TRY(wf.GetOffset(end_offset));

        this->header.EnsureMagic();
        this->header.byte_order = Header::SupportedByteOrder;
        this->header.version = Header::SupportedVersion;
        this->header.file_size = end_offset;
        this->header.header_size = sizeof(Header);
        this->header.block_count = 3; // FAT + FNT + FIMG

        TWL_R_TRY(wf.SetAbsoluteOffset(0));
        TWL_R_TRY(wf.Write(this->header));

        TWL_R_SUCCEED();
    }

}
