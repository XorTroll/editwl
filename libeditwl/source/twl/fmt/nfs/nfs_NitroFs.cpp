#include <twl/fmt/nfs/nfs_NitroFs.hpp>

namespace twl::fmt::nfs {

    namespace {

        Result ReadNitroFile(fs::File &rf, const size_t file_data_offset, const size_t fat_data_offset, const u16 file_id, NitroFile &out_file) {
            TWL_R_TRY(rf.SetAbsoluteOffset(fat_data_offset + file_id * sizeof(NitroFileSystem::FileAllocationTableEntry)));
            NitroFileSystem::FileAllocationTableEntry fat_entry;
            TWL_R_TRY(rf.Read(fat_entry));

            const auto file_size = fat_entry.file_end - fat_entry.file_start;

            TWL_R_TRY(rf.SetAbsoluteOffset(file_data_offset + fat_entry.file_start));
            auto file_buf = new u8[file_size]();
            ScopeGuard on_failure([&]() {
                delete[] file_buf;
            });
            TWL_R_TRY(rf.ReadBuffer(file_buf, file_size));
            
            on_failure.Cancel();

            out_file.inner_file.CreateFrom(file_buf, file_size);
            TWL_R_SUCCEED();
        }

        Result ReadNitroDirectory(const size_t file_data_offset, const size_t fat_data_offset, const size_t fnt_data_offset, fs::File &rf, NitroDirectory &nitro_dir, const u16 dir_id, u16 &min_tree_file_id) {
            const auto dir_idx = dir_id & 0xFFF;
            TWL_R_TRY(rf.SetAbsoluteOffset(fnt_data_offset + dir_idx * sizeof(NitroFileSystem::DirectoryNameTableEntry)));

            NitroFileSystem::DirectoryNameTableEntry dir_entry;
            TWL_R_TRY(rf.Read(dir_entry));

            // File IDs only increment in the loop, thus it's enough to just check the first one
            min_tree_file_id = std::min(dir_entry.first_file_id, min_tree_file_id);

            auto cur_file_id = dir_entry.first_file_id;
            TWL_R_TRY(rf.SetAbsoluteOffset(fnt_data_offset + dir_entry.start));
            while(true) {
                u8 entry_val;
                TWL_R_TRY(rf.Read(entry_val));

                if(entry_val == 0) {
                    // End of directory
                    break;
                }
                else if(entry_val < NitroFileSystem::MaxEntryNameLength) {
                    // File
                    NitroFile nitro_file = {
                        .file_id = cur_file_id
                    };

                    char name[NitroFileSystem::MaxEntryNameLength] = {};
                    const auto name_len = entry_val;
                    TWL_R_TRY(rf.ReadBuffer(name, name_len));
                    nitro_file.name.assign(name);

                    size_t old_offset;
                    TWL_R_TRY(rf.GetOffset(old_offset));

                    TWL_R_TRY(ReadNitroFile(rf, file_data_offset, fat_data_offset, cur_file_id, nitro_file));

                    nitro_dir.files.push_back(std::move(nitro_file));
                    cur_file_id++;
                    TWL_R_TRY(rf.SetAbsoluteOffset(old_offset));
                }
                else {
                    // Directory
                    NitroDirectory nitro_subdir = {};

                    char name[NitroFileSystem::MaxEntryNameLength] = {};
                    const auto name_len = entry_val - NitroFileSystem::MaxEntryNameLength;
                    TWL_R_TRY(rf.ReadBuffer(name, name_len));
                    nitro_subdir.name.assign(name);

                    u16 sub_dir_id;
                    TWL_R_TRY(rf.Read(sub_dir_id));

                    size_t old_offset;
                    TWL_R_TRY(rf.GetOffset(old_offset));

                    TWL_R_TRY(ReadNitroDirectory(file_data_offset, fat_data_offset, fnt_data_offset, rf, nitro_subdir, sub_dir_id, min_tree_file_id));

                    nitro_dir.dirs.push_back(std::move(nitro_subdir));
                    TWL_R_TRY(rf.SetAbsoluteOffset(old_offset));
                }
            }

            TWL_R_SUCCEED();
        }

        Result FindFileByIdInNitroDirectory(const u32 file_id, NitroDirectory &nitro_dir, NitroFile *&out_file) {
            for(auto &file: nitro_dir.files) {
                if(file.file_id == file_id) {
                    out_file = std::addressof(file);
                    TWL_R_SUCCEED();
                }
            }

            for(auto &dir: nitro_dir.dirs) {
                TWL_R_TRY(FindFileByIdInNitroDirectory(file_id, nitro_dir, out_file));
                if(out_file != nullptr) {
                    TWL_R_SUCCEED();
                }
            }

            TWL_R_SUCCEED();
        }

        void SplitPathItems(const std::string &path, std::vector<std::string> &out_path) {
            size_t path_offset = 0;
            while(path.at(path_offset) == '/') {
                path_offset++;
            }

            std::string cur_item;

            #define _CHECK_CUR_ITEM { \
                if(!cur_item.empty()) { \
                    out_path.push_back(cur_item); \
                    cur_item = ""; \
                } \
            }

            while(path_offset < path.length()) {
                const auto cur_ch = path.at(path_offset);
                if(cur_ch == '/') {
                    _CHECK_CUR_ITEM;
                }
                else {
                    cur_item += cur_ch;
                }

                path_offset++;
            }
            _CHECK_CUR_ITEM;

            #undef _CHECK_CUR_ITEM
        }

        Result FindFileByPathInNitroDirectory(const std::vector<std::string> &path_items, const u32 cur_path_item_idx, NitroDirectory &nitro_dir, NitroFile *&out_file) {
            if(cur_path_item_idx >= path_items.size()) {
                TWL_R_FAIL(ResultNitroFsFileNotFound);
            }

            const auto cur_item = path_items.at(cur_path_item_idx);

            for(auto &file: nitro_dir.files) {
                if(file.name == cur_item) {
                    if(cur_path_item_idx != (path_items.size() - 1)) {
                        TWL_R_FAIL(ResultNitroFsDirectoryNotFound);
                    }

                    out_file = std::addressof(file);
                    TWL_R_SUCCEED();
                }
            }

            for(auto &dir: nitro_dir.dirs) {
                if(dir.name == cur_item) {
                    if(cur_path_item_idx == (path_items.size() - 1)) {
                        TWL_R_FAIL(ResultNitroFsFileNotFound);
                    }

                    TWL_R_TRY(FindFileByPathInNitroDirectory(path_items, cur_path_item_idx + 1, dir, out_file));
                }
            }

            TWL_R_SUCCEED();
        }

        Result WriteNitroFile(fs::BufferReaderWriter &out_file_data, std::vector<NitroFileSystem::FileAllocationTableEntry> &out_fat, const size_t file_end_align, NitroFile &file) {
            const auto cur_offset = out_file_data.GetBufferOffset();

            out_fat.push_back(NitroFileSystem::FileAllocationTableEntry {
                .file_start = static_cast<u32>(cur_offset),
                .file_end = static_cast<u32>(cur_offset + file.inner_file.GetBufferSize())
            });

            TWL_R_TRY(out_file_data.WriteBuffer(file.inner_file.GetBuffer(), file.inner_file.GetBufferSize()));
            TWL_R_TRY(out_file_data.WriteEnsureAlignment(file_end_align));

            TWL_R_SUCCEED();
        }

        Result WriteNitroDirectory(fs::BufferReaderWriter &out_fnt_data, fs::BufferReaderWriter &out_file_data, std::vector<NitroFileSystem::DirectoryNameTableEntry> &out_fnt, std::vector<NitroFileSystem::FileAllocationTableEntry> &out_fat, const size_t file_end_align, NitroDirectory &nitro_dir, const u16 cur_dir_id, const u16 parent_dir_id, u16 &out_dir_count) {
            out_dir_count++;

            auto &cur_fnt_entry = out_fnt.at(cur_dir_id - NitroFileSystem::RootDirectoryId);
            cur_fnt_entry.parent_id = parent_dir_id;

            cur_fnt_entry.start = out_fnt_data.GetBufferOffset();
            cur_fnt_entry.first_file_id = out_fat.size();

            for(auto &file: nitro_dir.files) {
                TWL_R_TRY(WriteNitroFile(out_file_data, out_fat, file_end_align, file));

                TWL_R_TRY(out_fnt_data.Write(static_cast<u8>(file.name.length())));
                TWL_R_TRY(out_fnt_data.WriteBuffer(file.name.c_str(), file.name.length()));
            }

            std::vector<size_t> subdir_id_offsets;

            for(u32 i = 0; i < nitro_dir.dirs.size(); i++) {
                auto &dir = nitro_dir.dirs.at(i);

                TWL_R_TRY(out_fnt_data.Write(static_cast<u8>(NitroFileSystem::MaxEntryNameLength + dir.name.length())));
                TWL_R_TRY(out_fnt_data.WriteBuffer(dir.name.c_str(), dir.name.length()));

                size_t subdir_id_offset;
                TWL_R_TRY(out_fnt_data.GetOffset(subdir_id_offset));
                subdir_id_offsets.push_back(subdir_id_offset);

                TWL_R_TRY(out_fnt_data.Write<u16>(0));
            }

            std::vector<u16> subdir_ids;
            
            for(u32 i = 0; i < nitro_dir.dirs.size(); i++) {
                auto &dir = nitro_dir.dirs.at(i);
                const u16 cur_subdir_id = NitroFileSystem::RootDirectoryId + out_fnt.size();
                subdir_ids.push_back(cur_subdir_id);
                out_fnt.emplace_back();

                size_t cur_offset;
                TWL_R_TRY(out_fnt_data.GetOffset(cur_offset));
                TWL_R_TRY(out_fnt_data.SetAbsoluteOffset(subdir_id_offsets.at(i)));
                TWL_R_TRY(out_fnt_data.Write(cur_subdir_id));
                TWL_R_TRY(out_fnt_data.SetAbsoluteOffset(cur_offset));
            }

            TWL_R_TRY(out_fnt_data.Write<u8>(0));

            for(u32 i = 0; i < nitro_dir.dirs.size(); i++) {
                auto &dir = nitro_dir.dirs.at(i);
                TWL_R_TRY(WriteNitroDirectory(out_fnt_data, out_file_data, out_fnt, out_fat, file_end_align, dir, subdir_ids.at(i), cur_dir_id, out_dir_count));
            }

            TWL_R_SUCCEED();
        }

        void DisposeNitroDirectory(NitroDirectory &nitro_dir) {
            for(auto &file: nitro_dir.files) {
                file.Dispose();
            }

            for(auto &dir: nitro_dir.dirs) {
                DisposeNitroDirectory(dir);
            }
        }

    }

    Result NitroFileSystem::ReadFrom(fs::File &rf, const size_t file_data_offset, const size_t fat_data_offset, const size_t fnt_data_offset) {
        this->Dispose();
        
        // Note: we assume that, if any files are outside the directory tree structure, they will have the first IDs (ID 0, 1, 2...)
        // This is the case for the only relevant usage of extra files (overlays inside ROM filesystems)
        // For that purpose, we just keep track of the lowest file ID of any file loaded in the tree -> that will be the count of extra files

        // Read and load tree structure (files and dirs)

        u16 min_tree_file_id = UINT16_MAX;
        TWL_R_TRY(ReadNitroDirectory(file_data_offset, fat_data_offset, fnt_data_offset, rf, this->root_dir, NitroFileSystem::RootDirectoryId, min_tree_file_id));

        const auto ext_file_count = min_tree_file_id;
        for(u32 i = 0; i < ext_file_count; i++) {
            NitroFile ext_file = {};
            TWL_R_TRY(ReadNitroFile(rf, file_data_offset, fat_data_offset, i, ext_file));
            this->ext_files.push_back(std::move(ext_file));
        }
    
        this->fat_data_offset = fat_data_offset;
        this->fnt_data_offset = fnt_data_offset;
        TWL_R_SUCCEED();
    }

    Result NitroFileSystem::WriteTo(fs::BufferReaderWriter &out_fnt_data, fs::BufferReaderWriter &out_file_data, std::vector<NitroFileSystem::DirectoryNameTableEntry> &out_fnt, std::vector<NitroFileSystem::FileAllocationTableEntry> &out_fat, const size_t file_end_align) {
        // Start with extra files

        for(auto &ext_file: this->ext_files) {
            TWL_R_TRY(WriteNitroFile(out_file_data, out_fat, file_end_align, ext_file));
        }

        // Then write the tree structure

        out_fnt.clear();
        out_fnt.emplace_back();

        u16 total_dir_count = 0;
        TWL_R_TRY(WriteNitroDirectory(out_fnt_data, out_file_data, out_fnt, out_fat, file_end_align, this->root_dir, NitroFileSystem::RootDirectoryId, 0, total_dir_count));

        // Special use of the 'parent ID' field of the root directory (some editors check/rely on this)
        
        out_fnt.at(0).parent_id = total_dir_count;

        TWL_R_SUCCEED();
    }

    Result NitroFileSystemFile::OpenImpl(const fs::FileMode mode) {
        if(!this->IsValid()) {
            TWL_R_FAIL(ResultFileNotInitialized);
        }

        TWL_R_TRY(this->file_ref->inner_file.OpenImpl(mode));
        TWL_R_SUCCEED();
    }

    Result NitroFileSystemFile::CreateById(NitroFileSystem &nitro_fs, const u32 file_id) {
        for(auto &ext_file: nitro_fs.ext_files) {
            if(ext_file.file_id == file_id) {
                this->file_ref = std::addressof(ext_file);
                TWL_R_SUCCEED();
            }
        }

        NitroFile *tree_file = nullptr;
        TWL_R_TRY(FindFileByIdInNitroDirectory(file_id, nitro_fs.root_dir, tree_file));

        if(tree_file != nullptr) {
            this->file_ref = tree_file;
            TWL_R_SUCCEED();
        }
        
        TWL_R_FAIL(ResultNitroFsFileNotFound);
    }

    Result NitroFileSystemFile::CreateByPath(NitroFileSystem &nitro_fs, const std::string &path) {
        std::vector<std::string> path_items;
        SplitPathItems(path, path_items);

        NitroFile *file = nullptr;
        TWL_R_TRY(FindFileByPathInNitroDirectory(path_items, 0, nitro_fs.root_dir, file));

        if(file != nullptr) {
            this->file_ref = file;
            TWL_R_SUCCEED();
        }
        
        TWL_R_FAIL(ResultNitroFsFileNotFound);
    }

    void NitroFileSystem::Dispose() {
        DisposeNitroDirectory(this->root_dir);

        for(auto &ext_file: this->ext_files) {
            ext_file.Dispose();
        }
    }

}
