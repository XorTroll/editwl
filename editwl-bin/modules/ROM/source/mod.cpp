#include <mod/mod_Module.hpp>
#include <args.hxx>
#include <base_Include.hpp>

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
        else if(extract_code) {
            const auto rom_path = extract_code_rom_file.Get();
            const auto processor = extract_code_processor.Get();
            const auto out_code_path = extract_code_out_code_file.Get();

            twl::fmt::ROM::ProcessorType type;
            if(!ParseProcessorType(processor, type)) {
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
    }

}

ETWL_MOD_DEFINE_START(
    "ROM",
    "DS(i) ROM support and utilities",
    "XorTroll",
    ETWL_MAJOR,ETWL_MINOR,ETWL_MICRO,ETWL_BUGFIX,
    ResultDescriptionTable, std::size(ResultDescriptionTable)
)

ETWL_MOD_DEFINE_REGISTER_COMMAND("rom", HandleCommand)

ETWL_MOD_DEFINE_END()

ETWL_MOD_SYMBOL bool ETWL_MOD_TRY_HANDLE_INPUT_SYMBOL(const QString &path, etwl::mod::Context *ctx) {
    twl::fs::StdioFile rf(path.toStdString());
    auto rc = rf.OpenRead();
    if(rc.IsSuccess()) {
        twl::ScopeGuard close_f([&]() {
            rf.Close();
        });

        twl::fmt::ROM rom;
        rc = rom.ReadFrom(rf);
        if(rc.IsSuccess()) {
            // TODO
            return true;
        }
    }

    return false;
}
