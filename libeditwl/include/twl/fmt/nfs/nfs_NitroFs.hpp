
#pragma once
#include <twl/fs/fs_File.hpp>
#include <twl/fs/fs_FileFormat.hpp>

namespace twl::fmt::nfs {

    struct NitroFile {
        std::string name;
        u16 file_id;
        fs::BufferFile inner_file;

        inline void Dispose() {
            this->inner_file.Dispose();
        }
    };

    struct NitroDirectory {
        std::string name;
        std::vector<NitroDirectory> dirs;
        std::vector<NitroFile> files;
    };

    struct NitroFileSystem;

    struct NitroFileSystem {
        struct DirectoryNameTableEntry {
            u32 start;
            u16 first_file_id;
            u16 parent_id;
        };

        struct FileAllocationTableEntry {
            u32 file_start;
            u32 file_end;
        };

        static constexpr u16 RootDirectoryId = 0xF000;
        static constexpr size_t MaxEntryNameLength = 128;

        size_t fat_data_offset;
        size_t fnt_data_offset;
        NitroDirectory root_dir;
        std::vector<NitroFile> ext_files;

        Result ReadFrom(fs::File &rf, const size_t file_data_offset, const size_t fat_data_offset, const size_t fnt_data_offset);
        Result WriteTo(fs::BufferReaderWriter &out_fnt_data, fs::BufferReaderWriter &out_file_data, std::vector<NitroFileSystem::DirectoryNameTableEntry> &out_fnt, std::vector<NitroFileSystem::FileAllocationTableEntry> &out_fat, const size_t file_end_align);

        void Dispose();
    };

    class NitroFileSystemFile : public fs::File {
        public:
            NitroFile *file_ref;

        public:
            NitroFileSystemFile() : File(), file_ref(nullptr) {}
            NitroFileSystemFile(NitroFile *file_ref) : File(), file_ref(file_ref) {}

            Result CreateById(NitroFileSystem &nitro_fs, const u32 file_id);
            Result CreateByPath(NitroFileSystem &nitro_fs, const std::string &path);

            inline bool IsValid() {
                return this->file_ref != nullptr;
            }

            Result OpenImpl(const fs::FileMode mode) override;

            inline Result GetSizeImpl(size_t &out_size) override {
                TWL_R_TRY(this->file_ref->inner_file.GetSizeImpl(out_size));
                TWL_R_SUCCEED();
            }

            inline Result SetOffsetImpl(const size_t offset, const fs::Whence whence) override {
                TWL_R_TRY(this->file_ref->inner_file.SetOffsetImpl(offset, whence));
                TWL_R_SUCCEED();
            }

            inline Result GetOffsetImpl(size_t &out_offset) override {
                TWL_R_TRY(this->file_ref->inner_file.GetOffsetImpl(out_offset));
                TWL_R_SUCCEED();
            }

            inline Result ReadBufferImpl(void *read_buf, const size_t read_size) override {
                TWL_R_TRY(this->file_ref->inner_file.ReadBufferImpl(read_buf, read_size));
                TWL_R_SUCCEED();
            }

            inline Result WriteBufferImpl(const void *write_buf, const size_t write_size) override {
                TWL_R_TRY(this->file_ref->inner_file.WriteBufferImpl(write_buf, write_size));
                TWL_R_SUCCEED();
            }

            inline Result CloseImpl() override {
                TWL_R_TRY(this->file_ref->inner_file.CloseImpl());
                TWL_R_SUCCEED();
            }
    };

    class NitroFileSystemFormat {
        protected:
            NitroFileSystem nitro_fs;

        public:
            inline Result CreateFileById(NitroFileSystemFile &file, const u32 file_id) {
                TWL_R_TRY(file.CreateById(this->nitro_fs, file_id));
                TWL_R_SUCCEED();
            }

            inline Result CreateFileByPath(NitroFileSystemFile &file, const std::string &path) {
                TWL_R_TRY(file.CreateByPath(this->nitro_fs, path));
                TWL_R_SUCCEED();
            }

            inline NitroFileSystem &GetFs() {
                return this->nitro_fs;
            }
    };

}
