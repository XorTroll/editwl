#include "qdom.h"
#include "twl/fmt/nfs/nfs_NitroFs.hpp"
#include "twl/fs/fs_File.hpp"
#include "twl/twl_Include.hpp"
#include <QFile>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <rom/rom.hpp>
#include <args.hxx>
#include <string>
#include <twl/fmt/fmt_ROM.hpp>
#include <utility>

#define R_TRY_ERRLOG(rc, ...) { \
    const auto _tmp_rc = (rc); \
    if(_tmp_rc.IsFailure()) { \
        std::cerr << __VA_ARGS__ << ": " << _tmp_rc.GetDescription() << std::endl; \
        return; \
    } \
}

namespace {

    bool ParseProcessorType(const std::string &raw_type, twl::fmt::ROM::ProcessorType &out_type) {
        if((raw_type == "arm7") || (raw_type == "ARM7") || (raw_type == "7")) {
            out_type = twl::fmt::ROM::ProcessorType::ARM7;
            return true;
        }
        if((raw_type == "arm9") || (raw_type == "ARM9") || (raw_type == "9")) {
            out_type = twl::fmt::ROM::ProcessorType::ARM9;
            return true;
        }

        std::cerr << "Invalid processor type, must be one of: arm7, ARM7, 7; arm9, ARM9, 9" << std::endl;
        return false;
    }

    void PrintInformation(const std::string &rom_path) {
        twl::fs::StdioFile rom_file(rom_path);
        R_TRY_ERRLOG(rom_file.OpenRead(), "Unable to open ROM file '" << rom_path << "'");

        twl::ScopeGuard close_file([&]() {
            rom_file.Close();
        });

        twl::fmt::ROM rom;
        R_TRY_ERRLOG(rom.ReadFrom(rom_file), "Unable to read ROM file '" << rom_path << "'");

        // Print fields

        std::cout << "Header:" << std::endl;
        std::cout << "> Game title: " << rom.header.game_title << std::endl;

        if(!rom.arm9_ovl_table.empty()) {
            std::cout << "ARM9 Overlays:" << std::endl;

            int i = 0;
            for(const auto &ovt_entry: rom.arm9_ovl_table) {
                std::cout << "> Overlay " << i << ":" << std::endl;
                std::cout << ">>> File ID: " << ovt_entry.file_id << std::endl;
                std::cout << ">>> Size: " << ovt_entry.ram_size << std::endl;
                std::cout << ">>> RAM: from 0x" << std::hex << ovt_entry.ram_address << std::dec << " to 0x" << std::hex << ovt_entry.ram_address + ovt_entry.ram_size << std::dec << std::endl;
                std::cout << ">>> Init-array: from 0x" << std::hex << ovt_entry.static_init_start_address << std::dec << " to 0x" << std::hex << ovt_entry.static_init_end_address << std::dec << std::endl;

                i++;
            }
        }
        
        if(rom.footer.has_value()) {
            auto &footer = rom.footer.value();
            std::cout << "Nitro footer:" << std::endl;
            std::cout << "> Start module params offset: 0x" << std::hex << footer.start_module_params_offset << std::dec << std::endl;
        }

        if(rom.start_module_params.has_value()) {
            auto &params = rom.start_module_params.value();
            std::cout << "Start module params:" << std::endl;
            std::cout << "> Autoload list start: 0x" << std::hex << params.autoload_list_start << std::dec << std::endl;
            std::cout << "> Autoload list start: 0x" << std::hex << params.autoload_list_end << std::dec << std::endl;
            std::cout << "> Autoload start: 0x" << std::hex << params.autoload_start << std::dec << std::endl;
            std::cout << "> Static BSS start: 0x" << std::hex << params.static_bss_start << std::dec << std::endl;
            std::cout << "> Static BSS end: 0x" << std::hex << params.static_bss_end << std::dec << std::endl;
            std::cout << "> Compressed static end: 0x" << std::hex << params.compressed_static_end << std::dec << std::endl;
            std::cout << "> SDK version: 0x" << std::hex << params.sdk_version << std::dec << std::endl;
            std::cout << "> LE nitro-code: 0x" << std::hex << params.nitro_code_le << std::dec << std::endl;
            std::cout << "> BE nitro-code: 0x" << std::hex << params.nitro_code_be << std::dec << std::endl;

            if(!rom.lib_symbols.empty()) {
                std::cout << "Used libraries: " << std::endl;
                for(const auto &sym: rom.lib_symbols) {
                    std::cout << "> " << sym << std::endl;
                }
            }
        }
    }

    void ExtractHeader(const std::string &rom_path, const std::string &out_header_path) {
        twl::fs::StdioFile rom_file(rom_path);
        R_TRY_ERRLOG(rom_file.OpenRead(), "Unable to open ROM file '" << rom_path << "'");

        twl::ScopeGuard close_file([&]() {
            rom_file.Close();
        });

        twl::fmt::ROM rom;
        R_TRY_ERRLOG(rom.ReadFrom(rom_file), "Unable to read ROM file '" << rom_path << "'");

        twl::fs::StdioFile out_header_file(out_header_path);
        R_TRY_ERRLOG(out_header_file.OpenWrite(), "Unable to open out header file '" << out_header_path << "'");

        twl::ScopeGuard close_out_file([&]() {
            out_header_file.Close();
        });

        R_TRY_ERRLOG(out_header_file.Write(rom.header), "Unable to save header file to '" << out_header_path << "'");
    }

    void ExtractOverlayTables(const std::string &rom_path, const std::string &out_arm7_ovt_path, const std::string &out_arm9_ovt_path) {
        twl::fs::StdioFile rom_file(rom_path);
        R_TRY_ERRLOG(rom_file.OpenRead(), "Unable to open ROM file '" << rom_path << "'");

        twl::ScopeGuard close_file([&]() {
            rom_file.Close();
        });

        twl::fmt::ROM rom;
        R_TRY_ERRLOG(rom.ReadFrom(rom_file), "Unable to read ROM file '" << rom_path << "'");

        if(!out_arm7_ovt_path.empty()) {
            twl::fs::StdioFile out_arm7_ovt_file(out_arm7_ovt_path);
            R_TRY_ERRLOG(out_arm7_ovt_file.OpenWrite(), "Unable to open out ARM7 overlay table file '" << out_arm7_ovt_path << "'");

            twl::ScopeGuard close_out_file([&]() {
                out_arm7_ovt_file.Close();
            });

            R_TRY_ERRLOG(out_arm7_ovt_file.WriteVector(rom.arm7_ovl_table), "Unable to save ARM7 overlay table file to '" << out_arm7_ovt_path << "'");
        }

        if(!out_arm9_ovt_path.empty()) {
            twl::fs::StdioFile out_arm9_ovt_file(out_arm9_ovt_path);
            R_TRY_ERRLOG(out_arm9_ovt_file.OpenWrite(), "Unable to open out ARM9 overlay table file '" << out_arm9_ovt_path << "'");

            twl::ScopeGuard close_out_file([&]() {
                out_arm9_ovt_file.Close();
            });

            R_TRY_ERRLOG(out_arm9_ovt_file.WriteVector(rom.arm9_ovl_table), "Unable to save ARM9 overlay table file to '" << out_arm9_ovt_path << "'");
        }
    }

    /*
    void ExtractOverlays(const std::string &rom_path, const std::string &out_arm7_ovl_path, const std::string &out_arm9_ovl_path) {
        twl::fs::StdioFile rom_file(rom_path);
        R_TRY_ERRLOG(rom_file.OpenRead(), "Unable to open ROM file '" << rom_path << "'");

        twl::ScopeGuard close_file([&]() {
            rom_file.Close();
        });

        twl::fmt::ROM rom;
        R_TRY_ERRLOG(rom.ReadFrom(rom_file), "Unable to read ROM file '" << rom_path << "'");

        if(!out_arm7_ovt_path.empty()) {
            twl::fs::StdioFile out_arm7_ovt_file(out_arm7_ovt_path);
            R_TRY_ERRLOG(out_arm7_ovt_file.OpenWrite(), "Unable to open out ARM7 overlay table file '" << out_arm7_ovt_path << "'");

            twl::ScopeGuard close_out_file([&]() {
                out_arm7_ovt_file.Close();
            });

            R_TRY_ERRLOG(out_arm7_ovt_file.WriteVector(rom.arm7_ovl_table), "Unable to save ARM7 overlay table file to '" << out_arm7_ovt_path << "'");
        }

        if(!out_arm9_ovt_path.empty()) {
            twl::fs::StdioFile out_arm9_ovt_file(out_arm9_ovt_path);
            R_TRY_ERRLOG(out_arm9_ovt_file.OpenWrite(), "Unable to open out ARM9 overlay table file '" << out_arm9_ovt_path << "'");

            twl::ScopeGuard close_out_file([&]() {
                out_arm9_ovt_file.Close();
            });

            R_TRY_ERRLOG(out_arm9_ovt_file.WriteVector(rom.arm9_ovl_table), "Unable to save ARM9 overlay table file to '" << out_arm9_ovt_path << "'");
        }
    }
    */
    
    void ExtractCodes(const std::string &rom_path, const std::string &out_arm7_code_path, const std::string &out_arm9_code_path) {
        twl::fs::StdioFile rom_file(rom_path);
        R_TRY_ERRLOG(rom_file.OpenRead(), "Unable to open ROM file '" << rom_path << "'");

        twl::ScopeGuard close_file([&]() {
            rom_file.Close();
        });

        twl::fmt::ROM rom;
        R_TRY_ERRLOG(rom.ReadFrom(rom_file), "Unable to read ROM file '" << rom_path << "'");

        if(!out_arm7_code_path.empty()) {
            twl::fs::StdioFile out_arm7_code_file(out_arm7_code_path);
            R_TRY_ERRLOG(out_arm7_code_file.OpenWrite(), "Unable to open out ARM7 ROM code file '" << out_arm7_code_path << "'");

            twl::ScopeGuard close_out_file([&]() {
                out_arm7_code_file.Close();
            });

            R_TRY_ERRLOG(out_arm7_code_file.WriteBuffer(rom.arm7_rw.GetBuffer(), rom.arm7_rw.GetBufferSize()), "Unable to save ARM7 ROM code file to '" << out_arm7_code_path << "'");
        }

        if(!out_arm9_code_path.empty()) {
            twl::fs::StdioFile out_arm9_code_file(out_arm9_code_path);
            R_TRY_ERRLOG(out_arm9_code_file.OpenWrite(), "Unable to open out ARM9 ROM code file '" << out_arm9_code_path << "'");

            twl::ScopeGuard close_out_file([&]() {
                out_arm9_code_file.Close();
            });

            R_TRY_ERRLOG(out_arm9_code_file.WriteBuffer(rom.arm9_rw.GetBuffer(), rom.arm9_rw.GetBufferSize()), "Unable to save ARM9 ROM code file to '" << out_arm9_code_path << "'");
        }
    }

    void ReplaceCodes(const std::string &rom_path, const std::string &arm7_code_path, const std::string &arm9_code_path, const std::string &out_rom_path) {
        twl::fs::StdioFile rom_file(rom_path);
        R_TRY_ERRLOG(rom_file.OpenRead(), "Unable to open ROM file '" << rom_path << "'");

        twl::ScopeGuard close_rom_file([&]() {
            rom_file.Close();
        });

        twl::fmt::ROM rom;
        R_TRY_ERRLOG(rom.ReadFrom(rom_file), "Unable to open output ROM file '" << out_rom_path << "'");

        twl::fs::StdioFile out_rom_file(out_rom_path);
        R_TRY_ERRLOG(out_rom_file.OpenWrite(), "Unable to open output ROM file '" << out_rom_path << "'");
        
        twl::ScopeGuard close_out_rom_file([&]() {
            out_rom_file.Close();
        });

        if(!arm7_code_path.empty()) {
            twl::fs::StdioFile arm7_code_file(arm7_code_path);
            R_TRY_ERRLOG(arm7_code_file.OpenRead(), "Unable to open input ARM7 code file '" << arm7_code_path << "'");

            twl::ScopeGuard close_arm7_code_file([&]() {
                arm7_code_file.Close();
            });

            size_t code_size;
            R_TRY_ERRLOG(arm7_code_file.GetSize(code_size), "Unable to get size of input ARM7 code file '" << arm7_code_path << "'");
  
            auto code_buf = new twl::u8[code_size]();
            twl::ScopeGuard fail_delete_buf([&]() {
                delete[] code_buf;
            });

            R_TRY_ERRLOG(arm7_code_file.ReadBuffer(code_buf, code_size), "Unable to read input ARM7 code file '" << arm7_code_path << "'");

            fail_delete_buf.Cancel();
            rom.arm7_rw.CreateFrom(code_buf, code_size);
        }

        if(!arm9_code_path.empty()) {
            twl::fs::StdioFile arm9_code_file(arm9_code_path);
            R_TRY_ERRLOG(arm9_code_file.OpenRead(), "Unable to open input ARM9 code file '" << arm9_code_path << "'");

            twl::ScopeGuard close_arm9_code_file([&]() {
                arm9_code_file.Close();
            });

            size_t code_size;
            R_TRY_ERRLOG(arm9_code_file.GetSize(code_size), "Unable to get size of input ARM9 code file '" << arm9_code_path << "'");
  
            auto code_buf = new twl::u8[code_size]();
            twl::ScopeGuard fail_delete_buf([&]() {
                delete[] code_buf;
            });

            R_TRY_ERRLOG(arm9_code_file.ReadBuffer(code_buf, code_size), "Unable to read input ARM9 code file '" << arm9_code_path << "'");

            fail_delete_buf.Cancel();
            rom.arm9_rw.CreateFrom(code_buf, code_size);
        }

        R_TRY_ERRLOG(rom.WriteTo(out_rom_file), "Unable to save output ROM file '" << out_rom_path << "'");
    }

    namespace {

        void ExtractNitroDirectory(const std::filesystem::path &base_path, const std::filesystem::path &cur_rel_path, const bool verbose, const twl::fmt::nfs::NitroDirectory &dir, QDomDocument &file_id_doc, QDomElement &file_id_elem) {
            const auto dir_rel_path = cur_rel_path / dir.name;
            std::filesystem::create_directory(base_path / dir_rel_path);

            for(const auto &child_dir: dir.dirs) {
                ExtractNitroDirectory(base_path, dir_rel_path, verbose, child_dir, file_id_doc, file_id_elem);
            }
            for(const auto &child_file: dir.files) {
                const auto file_rel_path = dir_rel_path / child_file.name;
                const auto file_path = base_path / file_rel_path;
                auto f = fopen(file_path.c_str(), "wb");
                if(f != nullptr) {
                    twl::ScopeGuard close_file([&]() {
                        fclose(f);
                    });

                    fwrite(child_file.inner_file.GetBuffer(), child_file.inner_file.GetBufferSize(), 1, f);
                }
                else {
                    std::cerr << "Unable to open file '" << file_path.string() << "'" << std::endl;
                    return;
                }

                auto file_elem = file_id_doc.createElement("file");
                file_elem.setAttribute("id", QString::number(child_file.file_id));
                file_elem.setAttribute("path", QString::fromStdString(file_rel_path.string()));
                file_id_elem.appendChild(file_elem);

                if(verbose) {
                    std::cout << "-- Extracted " << file_rel_path << "..." << std::endl;
                }
            }
        }

    }

    void ExtractNitroFs(const std::string &rom_path, const std::string &out_dir, const bool verbose) {
        twl::fs::StdioFile rom_file(rom_path);
        R_TRY_ERRLOG(rom_file.OpenRead(), "Unable to open ROM file '" << rom_path << "'");

        twl::ScopeGuard close_file([&]() {
            rom_file.Close();
        });

        twl::fmt::ROM rom;
        R_TRY_ERRLOG(rom.ReadFrom(rom_file), "Unable to read ROM file '" << rom_path << "'");

        // Save file IDs
        QDomDocument file_id_doc("xml");
        auto nitrofs_elem = file_id_doc.createElement("nitrofs");
        file_id_doc.appendChild(nitrofs_elem);

        std::filesystem::create_directory(out_dir);
        const auto &nitro_fs = rom.GetFs();
        const auto out_path = std::filesystem::path(out_dir);
        ExtractNitroDirectory(out_path / "root", "", verbose, nitro_fs.root_dir, file_id_doc, nitrofs_elem);

        QFile file(out_path / "nitrofs.xml");
        if(!file.open(QIODevice::WriteOnly)) {
            std::cerr << "Unable to write XML file '" << file.fileName().toStdString() << "'" << std::endl;
            return;
        }

        QTextStream out(&file);
        file_id_doc.save(out, 4);
        file.close();
    }

    namespace {

        bool ReadNitroFsFile(const std::filesystem::path &file_path, twl::u8 *&out_buf, size_t &out_buf_size) {
            auto f = fopen(file_path.c_str(), "rb");
            if(f == nullptr) {
                return false;
            }
            twl::ScopeGuard close_file([&]() {
                fclose(f);
            });

            if(fseek(f, 0, SEEK_END) != 0) {
                return false;
            }
            const auto file_size = ftell(f);
            if(file_size < 0) {
                return false;
            }

            rewind(f);
            auto file_buf = new twl::u8[file_size]();
            if(fread(file_buf, file_size, 1, f) != 1) {
                return false;
            }

            out_buf = file_buf;
            out_buf_size = file_size;
            return true;
        }

        bool InsertNitroFile(twl::fmt::nfs::NitroDirectory &root_dir, const twl::u16 file_id, void *file_buf, const size_t file_size, const std::filesystem::path &rel_path) {
            twl::fmt::nfs::NitroDirectory *cur_iter_dir = std::addressof(root_dir);
            const auto filename = rel_path.filename().string();
            for(auto it = rel_path.begin(); it != rel_path.end(); it++) {
                const auto token = it->string();
                auto found = false;
                for(auto &dir: cur_iter_dir->dirs) {
                    if(dir.name == token) {
                        cur_iter_dir = std::addressof(dir);
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    if(token == filename) {
                        cur_iter_dir->files.push_back(twl::fmt::nfs::NitroFile {
                            .name = filename,
                            .file_id = file_id,
                            .inner_file = twl::fs::BufferFile(file_buf, file_size, true)
                        });
                        return true;
                    }
                    else {
                        auto &new_dir = cur_iter_dir->dirs.emplace_back();
                        new_dir.name = token;
                        cur_iter_dir = std::addressof(new_dir);
                    }
                }
            }

            return false;
        }

    }

    void ReplaceNitroFs(const std::string &rom_path, const std::string &dir, const std::string &out_rom_path, const bool verbose) {
        twl::fs::StdioFile rom_file(rom_path);
        R_TRY_ERRLOG(rom_file.OpenRead(), "Unable to open ROM file '" << rom_path << "'");

        twl::ScopeGuard close_file([&]() {
            rom_file.Close();
        });

        twl::fmt::ROM rom;
        R_TRY_ERRLOG(rom.ReadFrom(rom_file), "Unable to read ROM file '" << rom_path << "'");
        rom.GetFs().root_dir.files.clear();
        rom.GetFs().root_dir.dirs.clear();
        
        const auto base_path = std::filesystem::path(dir);
        const auto nitrofs_path = base_path / "root";
        
        QDomDocument doc;
        QFile file(base_path / "nitrofs.xml");
        if(!file.open(QIODevice::ReadOnly)) {
            std::cerr << "Unable to open NitroFS XML file" << std::endl;
            return;
        }

        twl::ScopeGuard close_xml_file([&]() {
            file.close();
        });

        if(!doc.setContent(&file)) {
            std::cerr << "Unable to read NitroFS XML file" << std::endl;
            return;
        }

        twl::fmt::nfs::NitroDirectory new_root_dir;
        const auto root = doc.documentElement();
        for(int i = 0; i < root.childNodes().size(); i++) {
            const auto child_elem = root.childNodes().at(i).toElement();
            const auto file_id = child_elem.attribute("id").toInt();
            const auto rel_path = std::filesystem::path(child_elem.attribute("path").toStdString());

            const auto file_path = nitrofs_path / rel_path;
            if(!std::filesystem::is_regular_file(file_path)) {
                std::cerr << "Unable to locate NitroFS file '" << rel_path << "' of ID " << file_id << std::endl;
                return;
            }

            twl::u8 *file_buf;
            size_t file_size;
            if(!ReadNitroFsFile(file_path, file_buf, file_size)) {
                std::cerr << "Unable to read NitroFS file '" << file_path.string() << "'" << std::endl;
                return;
            }

            if(!InsertNitroFile(new_root_dir, file_id, file_buf, file_size, rel_path)) {
                delete[] file_buf;
                std::cerr << "Unable to insert NitroFS file '" << rel_path.string() << "'" << std::endl;
                return;
            }

            if(verbose) {
                std::cout << "-- Imported " << rel_path << "..." << std::endl;
            }
        }

        rom.GetFs().root_dir = std::move(new_root_dir);

        twl::fs::StdioFile out_rom_file(out_rom_path);
        R_TRY_ERRLOG(out_rom_file.OpenWrite(), "Unable to open output ROM file '" << out_rom_path << "'");
        
        twl::ScopeGuard close_out_rom_file([&]() {
            out_rom_file.Close();
        });

        R_TRY_ERRLOG(rom.WriteTo(out_rom_file), "Unable to save output ROM file '" << out_rom_path << "'");
    }

}

namespace cli::rom {

    void HandleCommand(const std::vector<std::string> &args) {
        args::ArgumentParser parser("Module for DS(i) ROM files");
        args::HelpFlag help(parser, "help", "Displays this help menu", {'h', "help"});

        args::Group commands(parser, "Commands:", args::Group::Validators::Xor);

        args::Command info(commands, "info", "Show ROM information (header and more)");
        args::Group info_required(info, "", args::Group::Validators::All);
        args::ValueFlag<std::string> info_rom_file(info_required, "rom_file", "Input ROM file", {'r', "rom"});
        
        args::Command extract_header(commands, "extract-header", "Extract/export raw (binary) header (first 0x200 bytes)");
        args::Group extract_header_required(extract_header, "", args::Group::Validators::All);
        args::ValueFlag<std::string> extract_header_rom_file(extract_header_required, "rom_file", "Input ROM file", {'r', "rom"});
        args::ValueFlag<std::string> extract_header_out_header_file(extract_header_required, "out_header_file", "Output header file", {'o', "out"});
        
        args::Command extract_ovt(commands, "extract-overlay-table", "Extract/export ARM7 or ARM9 raw (binary) overlay table");
        args::Group extract_ovt_required(extract_ovt, "", args::Group::Validators::All);
        args::ValueFlag<std::string> extract_ovt_rom_file(extract_ovt_required, "rom_file", "Input ROM file", {'r', "rom"});
        args::ValueFlag<std::string> extract_ovt_processor(extract_ovt_required, "processor", "Processor (ARM7 or ARM9)", {'p', "proc"});
        args::ValueFlag<std::string> extract_ovt_out_ovt_file(extract_ovt_required, "out_ovt_file", "Output overlay table file", {'o', "out"});

        args::Command extract_ovts(commands, "extract-overlay-tables", "Extract/export ARM7 + ARM9 raw (binary) overlay tables");
        args::Group extract_ovts_required(extract_ovts, "", args::Group::Validators::All);
        args::ValueFlag<std::string> extract_ovts_rom_file(extract_ovts_required, "rom_file", "Input ROM file", {'r', "rom"});
        args::ValueFlag<std::string> extract_ovts_out_arm7_ovt_file(extract_ovts_required, "out_arm7_ovt_file", "Output ARM7 overlay table file", {'7', "out7"});
        args::ValueFlag<std::string> extract_ovts_out_arm9_ovt_file(extract_ovts_required, "out_arm9_ovt_file", "Output ARM9 overlay table file", {'9', "out9"});

        /*
        args::Command extract_ovl(commands, "extract-overlay", "Extract/export ARM7 or ARM9 raw (binary) overlay");
        args::Group extract_ovl_required(extract_ovl, "", args::Group::Validators::All);
        args::ValueFlag<std::string> extract_ovl_rom_file(extract_ovl_required, "rom_file", "Input ROM file", {'r', "rom"});
        args::ValueFlag<std::string> extract_ovl_processor(extract_ovl_required, "processor", "Processor (ARM7 or ARM9)", {'p', "proc"});
        args::ValueFlag<std::string> extract_ovl_out_ovl_file(extract_ovl_required, "out_ovl_file", "Output overlay file", {'o', "out"});
        */
        
        args::Command extract_code(commands, "extract-code", "Extract/export ARM7 or ARM9 code binary");
        args::Group extract_code_required(extract_code, "", args::Group::Validators::All);
        args::ValueFlag<std::string> extract_code_rom_file(extract_code_required, "rom_file", "Input ROM file", {'r', "rom"});
        args::ValueFlag<std::string> extract_code_processor(extract_code_required, "processor", "Processor (ARM7 or ARM9)", {'p', "proc"});
        args::ValueFlag<std::string> extract_code_out_code_file(extract_code_required, "out_code_file", "Output code file", {'o', "out"});

        args::Command extract_codes(commands, "extract-codes", "Extract/export ARM7 + ARM9 code binaries");
        args::Group extract_codes_required(extract_codes, "", args::Group::Validators::All);
        args::ValueFlag<std::string> extract_codes_rom_file(extract_codes_required, "rom_file", "Input ROM file", {'r', "rom"});
        args::ValueFlag<std::string> extract_codes_out_arm7_code_file(extract_codes_required, "out_arm7_code_file", "Output ARM7 code file", {'7', "out7"});
        args::ValueFlag<std::string> extract_codes_out_arm9_code_file(extract_codes_required, "out_arm9_code_file", "Output ARM9 code file", {'9', "out9"});

        args::Command replace_code(commands, "replace-code", "Replace/import ARM7 or ARM9 code binary");
        args::Group replace_code_required(replace_code, "", args::Group::Validators::All);
        args::ValueFlag<std::string> replace_code_rom_file(replace_code_required, "rom_file", "Input ROM file", {'r', "rom"});
        args::ValueFlag<std::string> replace_code_processor(replace_code_required, "processor", "Processor (ARM7 or ARM9)", {'p', "proc"});
        args::ValueFlag<std::string> replace_code_code_file(replace_code_required, "code_file", "Input code file", {'i', "in"});
        args::ValueFlag<std::string> replace_code_out_rom_file(replace_code_required, "out_rom_file", "Output ROM file", {'o', "out"});

        args::Command replace_codes(commands, "replace-codes", "Replace/import ARM7 + ARM9 code binaries");
        args::Group replace_codes_required(replace_codes, "", args::Group::Validators::All);
        args::ValueFlag<std::string> replace_codes_rom_file(replace_codes_required, "rom_file", "Input ROM file", {'r', "rom"});
        args::ValueFlag<std::string> replace_codes_arm7_code_file(replace_codes_required, "arm7_code_file", "Input ARM7 code file", {'7', "in7"});
        args::ValueFlag<std::string> replace_codes_arm9_code_file(replace_codes_required, "arm9_code_file", "Input ARM9 code file", {'9', "in9"});
        args::ValueFlag<std::string> replace_codes_out_rom_file(replace_codes_required, "out_rom_file", "Output ROM file", {'o', "out"});

        args::Command extract_nitrofs(commands, "extract-nitrofs", "Extract/export NitroFS filesystem");
        args::Group extract_nitrofs_required(extract_nitrofs, "", args::Group::Validators::All);
        args::ValueFlag<std::string> extract_nitrofs_rom_file(extract_nitrofs_required, "rom_file", "Input ROM file", {'r', "rom"});
        args::ValueFlag<std::string> extract_nitrofs_out_dir(extract_nitrofs_required, "out_dir", "Output directory", {'o', "out"});
        args::Flag extract_nitrofs_verbose(extract_nitrofs, "verbose", "Verbose output", {'v', "verbose"});

        args::Command replace_nitrofs(commands, "replace-nitrofs", "Replace/import NitroFS filesystem");
        args::Group replace_nitrofs_required(replace_nitrofs, "", args::Group::Validators::All);
        args::ValueFlag<std::string> replace_nitrofs_rom_file(replace_nitrofs_required, "rom_file", "Input ROM file", {'r', "rom"});
        args::ValueFlag<std::string> replace_nitrofs_dir(replace_nitrofs_required, "dir", "Input directory", {'i', "in"});
        args::ValueFlag<std::string> replace_nitrofs_out_rom_file(replace_nitrofs_required, "out_rom_file", "Output ROM file", {'o', "out"});
        args::Flag replace_nitrofs_verbose(replace_nitrofs, "verbose", "Verbose output", {'v', "verbose"});

        try {
            parser.ParseArgs(args);
        }
        catch(std::exception &e) {
            std::cerr << parser;
            std::cout << e.what() << std::endl;
            return;
        }

        if(info) {
            const auto rom_path = info_rom_file.Get();
            PrintInformation(rom_path);
        }
        else if(extract_header) {
            const auto rom_path = extract_header_rom_file.Get();
            const auto out_header_path = extract_header_out_header_file.Get();

            ExtractHeader(rom_path, out_header_path);
        }
        else if(extract_ovt) {
            const auto rom_path = extract_ovt_rom_file.Get();
            const auto processor = extract_ovt_processor.Get();
            const auto out_ovt_path = extract_ovt_out_ovt_file.Get();

            twl::fmt::ROM::ProcessorType type;
            if(!ParseProcessorType(processor, type)) {
                std::cerr << "Invalid processor type specified..." << std::endl;
                return;
            }

            const auto is_7 = type == twl::fmt::ROM::ProcessorType::ARM7;
            ExtractOverlayTables(rom_path, is_7 ? out_ovt_path : "", is_7 ? "" : out_ovt_path);
        }
        else if(extract_ovts) {
            const auto rom_path = extract_ovts_rom_file.Get();
            const auto out_arm7_ovt_path = extract_ovts_out_arm7_ovt_file.Get();
            const auto out_arm9_ovt_path = extract_ovts_out_arm9_ovt_file.Get();

            ExtractOverlayTables(rom_path, out_arm7_ovt_path, out_arm9_ovt_path);
        }
        /*
        else if(extract_ovl) {
            const auto rom_path = extract_ovl_rom_file.Get();
            const auto processor = extract_ovl_processor.Get();
            const auto out_ovl_path = extract_ovl_out_ovl_file.Get();

            twl::fmt::ROM::ProcessorType type;
            if(!ParseProcessorType(processor, type)) {
                std::cerr << "Invalid processor type specified..." << std::endl;
                return;
            }

            const auto is_7 = type == twl::fmt::ROM::ProcessorType::ARM7;
            ExtractOverlays(rom_path, is_7 ? out_ovl_path : "", is_7 ? "" : out_ovl_path);
        }
        */
        else if(extract_code) {
            const auto rom_path = extract_code_rom_file.Get();
            const auto processor = extract_code_processor.Get();
            const auto out_code_path = extract_code_out_code_file.Get();

            twl::fmt::ROM::ProcessorType type;
            if(!ParseProcessorType(processor, type)) {
                std::cerr << "Invalid processor type specified..." << std::endl;
                return;
            }

            const auto is_7 = type == twl::fmt::ROM::ProcessorType::ARM7;
            ExtractCodes(rom_path, is_7 ? out_code_path : "", is_7 ? "" : out_code_path);
        }
        else if(extract_codes) {
            const auto rom_path = extract_codes_rom_file.Get();
            const auto out_arm7_code_path = extract_codes_out_arm7_code_file.Get();
            const auto out_arm9_code_path = extract_codes_out_arm9_code_file.Get();

            ExtractCodes(rom_path, out_arm7_code_path, out_arm9_code_path);
        }
        else if(replace_code) {
            const auto rom_path = replace_code_rom_file.Get();
            const auto processor = replace_code_processor.Get();
            const auto code_path = replace_code_code_file.Get();
            const auto out_rom_path = replace_code_out_rom_file.Get();

            twl::fmt::ROM::ProcessorType type;
            if(!ParseProcessorType(processor, type)) {
                std::cerr << "Invalid processor type specified..." << std::endl;
                return;
            }

            const auto is_7 = type == twl::fmt::ROM::ProcessorType::ARM7;
            ReplaceCodes(rom_path, is_7 ? code_path : "", is_7 ? "" : code_path, out_rom_path);
        }
        else if(replace_codes) {
            const auto rom_path = replace_codes_rom_file.Get();
            const auto arm7_code_path = replace_codes_arm7_code_file.Get();
            const auto arm9_code_path = replace_codes_arm9_code_file.Get();
            const auto out_rom_path = replace_codes_out_rom_file.Get();

            ReplaceCodes(rom_path, arm7_code_path, arm9_code_path, out_rom_path);
        }
        else if(extract_nitrofs) {
            const auto rom_path = extract_nitrofs_rom_file.Get();
            const auto out_dir = extract_nitrofs_out_dir.Get();

            ExtractNitroFs(rom_path, out_dir, extract_nitrofs_verbose);
        }
        else if(replace_nitrofs) {
            const auto rom_path = replace_nitrofs_rom_file.Get();
            const auto dir = replace_nitrofs_dir.Get();
            const auto out_rom_path = replace_nitrofs_out_rom_file.Get();

            ReplaceNitroFs(rom_path, dir, out_rom_path, replace_nitrofs_verbose);
        }
    }

}
