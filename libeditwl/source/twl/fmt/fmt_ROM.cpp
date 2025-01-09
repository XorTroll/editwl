#include <twl/fmt/fmt_ROM.hpp>
#include <twl/util/util_Align.hpp>

namespace twl::fmt {

    namespace {

        constexpr u16 g_CRC16Table[] = {
            0, 49345, 49537, 320, 49921, 960, 640, 49729, 50689, 1728,
            1920, 51009, 1280, 50625, 50305, 1088, 52225, 3264, 3456, 52545,
            3840, 53185, 52865, 3648, 2560, 51905, 52097, 2880, 51457, 2496,
            2176, 51265, 55297, 6336, 6528, 55617, 6912, 56257, 55937, 6720,
            7680, 57025, 57217, 8000, 56577, 7616, 7296, 56385, 5120, 54465,
            54657, 5440, 55041, 6080, 5760, 54849, 53761, 4800, 4992, 54081,
            4352, 53697, 53377, 4160, 61441, 12480, 12672, 61761, 13056, 62401,
            62081, 12864, 13824, 63169, 63361, 14144, 62721, 13760, 13440, 62529,
            15360, 64705, 64897, 15680, 65281, 16320, 16000, 65089, 64001, 15040,
            15232, 64321, 14592, 63937, 63617, 14400, 10240, 59585, 59777, 10560,
            60161, 11200, 10880, 59969, 60929, 11968, 12160, 61249, 11520, 60865,
            60545, 11328, 58369, 9408, 9600, 58689, 9984, 59329, 59009, 9792,
            8704, 58049, 58241, 9024, 57601, 8640, 8320, 57409, 40961, 24768,
            24960, 41281, 25344, 41921, 41601, 25152, 26112, 42689, 42881, 26432,
            42241, 26048, 25728, 42049, 27648, 44225, 44417, 27968, 44801, 28608,
            28288, 44609, 43521, 27328, 27520, 43841, 26880, 43457, 43137, 26688,
            30720, 47297, 47489, 31040, 47873, 31680, 31360, 47681, 48641, 32448,
            32640, 48961, 32000, 48577, 48257, 31808, 46081, 29888, 30080, 46401,
            30464, 47041, 46721, 30272, 29184, 45761, 45953, 29504, 45313, 29120,
            28800, 45121, 20480, 37057, 37249, 20800, 37633, 21440, 21120, 37441,
            38401, 22208, 22400, 38721, 21760, 38337, 38017, 21568, 39937, 23744,
            23936, 40257, 24320, 40897, 40577, 24128, 23040, 39617, 39809, 23360,
            39169, 22976, 22656, 38977, 34817, 18624, 18816, 35137, 19200, 35777,
            35457, 19008, 19968, 36545, 36737, 20288, 36097, 19904, 19584, 35905,
            17408, 33985, 34177, 17728, 34561, 18368, 18048, 34369, 33281, 17088,
            17280, 33601, 16640, 33217, 32897, 16448
        };

        inline constexpr u16 GetCRC16(const u8 *data, const size_t data_size) {
            uint16_t crc = 0xffff;
            for(size_t i = 0; i < data_size; i++) {
                crc = (uint16_t)((crc >> 8) ^ g_CRC16Table[(crc ^ data[i]) & 0xff]);
            }
            return crc;
        }

    }

    Result ROM::ReadValidateFrom(fs::File &rf) {
        TWL_R_TRY(rf.Read(this->header));

        TWL_R_TRY(rf.SetAbsoluteOffset(this->header.banner_offset));
        TWL_R_TRY(rf.Read(this->banner));

        if((this->header.unit_code != UnitCode::NDS) && (this->header.unit_code != UnitCode::NDS_NDSi) && (this->header.unit_code != UnitCode::NDSi)) {
            TWL_R_FAIL(ResultROMInvalidUnitCode);
        }

        const auto logo_crc16 = GetCRC16(this->header.nintendo_logo, sizeof(this->header.nintendo_logo));
        if(logo_crc16 != this->header.nintendo_logo_crc) {
            TWL_R_FAIL(ResultROMInvalidNintendoLogoCRC16);
        }

        TWL_R_SUCCEED();
    }

    Result ROM::ReadAllFrom(fs::File &rf) {
        this->nitro_fs.Dispose();

        const auto file_data_offset = 0; // File offsets are absolute in ROMs
        TWL_R_TRY(this->nitro_fs.ReadFrom(rf, file_data_offset, this->header.fat_offset, this->header.fnt_offset));        

        #define _ROM_READ_CODE(code_offset, code_size, code_rw) { \
            auto code_buf = new u8[code_size](); \
            ScopeGuard on_fail([&]() { \
                delete[] code_buf; \
            }); \
            TWL_R_TRY(rf.SetAbsoluteOffset(code_offset)); \
            TWL_R_TRY(rf.ReadBuffer(code_buf, code_size)); \
            on_fail.Cancel(); \
            code_rw.CreateFrom(code_buf, code_size); \
        }

        _ROM_READ_CODE(this->header.arm7_rom_offset, this->header.arm7_rom_size, this->arm7_rw);
        _ROM_READ_CODE(this->header.arm9_rom_offset, this->header.arm9_rom_size, this->arm9_rw);

        #undef _ROM_READ_CODE

        TWL_R_TRY(rf.SetAbsoluteOffset(this->header.arm9_rom_offset + this->header.arm9_rom_size));

        u32 nitro_footer_code;
        TWL_R_TRY(rf.Read(nitro_footer_code));
        if(nitro_footer_code == NitroFooter::Code) {
            TWL_R_TRY(rf.SetAbsoluteOffset(this->header.arm9_rom_offset + this->header.arm9_rom_size));
            
            NitroFooter footer;
            TWL_R_TRY(rf.Read(footer));
            this->footer = footer;

            TWL_R_TRY(rf.SetAbsoluteOffset(this->header.arm9_rom_offset + footer.start_module_params_offset));

            StartModuleParams params;
            TWL_R_TRY(rf.Read(params));
            this->start_module_params = params;

            while(true) {
                std::string lib_symbol;
                TWL_R_TRY(rf.ReadNullTerminatedString(lib_symbol));
                
                if(lib_symbol.empty()) {
                    for(u32 i = 0; i < 5; i++) {
                        // Some of them are separated up to 4 null-characters (due to align I guess)
                        // Each empty-string read will advance the null-character it encounters
                        TWL_R_TRY(rf.ReadNullTerminatedString(lib_symbol));

                        if(!lib_symbol.empty()) {
                            break;
                        }
                    }
                }
                
                if(lib_symbol.empty()) {
                    break;
                }
                
                this->lib_symbols.push_back(lib_symbol);
            }
        }
        else {
            this->footer = {};
            this->start_module_params = {};
        }

        #define _ROM_READ_OVERLAY_TABLE(offset, size, table) { \
            TWL_R_TRY(rf.SetAbsoluteOffset(offset)); \
            const auto arm_overlay_count = size / sizeof(OverlayTableEntry); \
            table.clear(); \
            table.reserve(arm_overlay_count); \
            for(u32 i = 0; i < arm_overlay_count; i++) { \
                OverlayTableEntry entry = {}; \
                TWL_R_TRY(rf.Read(entry)); \
                table.push_back(entry); \
            } \
        }
        
        _ROM_READ_OVERLAY_TABLE(this->header.arm7_overlay_table_offset, this->header.arm7_overlay_table_size, this->arm7_ovl_table);
        _ROM_READ_OVERLAY_TABLE(this->header.arm9_overlay_table_offset, this->header.arm9_overlay_table_size, this->arm9_ovl_table);

        #undef _ROM_READ_OVERLAY_TABLE

        TWL_R_SUCCEED();
    }

    Result ROM::WriteTo(fs::File &wf) {
        // Generate FNT, FAT, write files
        
        fs::BufferReaderWriter file_data_rw(0);
        fs::BufferReaderWriter fnt_data_rw(0);

        ScopeGuard save([&]() {
            file_data_rw.Dispose();
            fnt_data_rw.Dispose();
        });

        std::vector<nfs::NitroFileSystem::DirectoryNameTableEntry> gen_fnt;
        std::vector<nfs::NitroFileSystem::FileAllocationTableEntry> gen_fat;
        TWL_R_TRY(this->nitro_fs.WriteTo(fnt_data_rw, file_data_rw, gen_fnt, gen_fat, SectionAlignment));

        #define _WRITE_CODE(rw, type, out_rom_offset, out_rom_size, write_footer) { \
            size_t cur_offset; \
            TWL_R_TRY(wf.GetOffset(cur_offset)); \
            const auto rom_size = rw.GetBufferSize(); \
            out_rom_offset = cur_offset; \
            out_rom_size = rom_size; \
            TWL_R_TRY(wf.WriteBuffer(rw.GetBuffer(), rom_size)); \
            if(write_footer && this->footer.has_value()) { \
                TWL_R_TRY(wf.Write(this->footer.value())); \
            } \
            TWL_R_TRY(wf.WriteEnsureAlignment(SectionAlignment)); \
        }

        // TODO: this is not properly implemented for modifying the overlay count (adding or removing new overlays), since file IDs are assumed to be the same (see assumptions with extra files in the nitrofs)

        #define _WRITE_OVERLAY_TABLE(ovl_table, out_ovl_table_offset, out_ovl_table_size) { \
            if(!ovl_table.empty()) { \
                size_t overlay_table_offset; \
                TWL_R_TRY(wf.GetOffset(overlay_table_offset)); \
                out_ovl_table_offset = overlay_table_offset; \
                out_ovl_table_size = ovl_table.size() * sizeof(OverlayTableEntry); \
                for(u32 i = 0; i < ovl_table.size(); i++) { \
                    auto &ovl = ovl_table.at(i); \
                    TWL_R_TRY(wf.Write(ovl)); \
                } \
                TWL_R_TRY(wf.WriteEnsureAlignment(SectionAlignment)); \
            } \
            else { \
                out_ovl_table_offset = 0; \
                out_ovl_table_size = 0; \
            } \
        }

        // Write ARM9 ROM

        constexpr size_t Arm9BaseRomOffset = 0x4000;
        TWL_R_TRY(wf.SetAbsoluteOffset(Arm9BaseRomOffset));

        this->header.header_size = Arm9BaseRomOffset;

        _WRITE_CODE(this->arm9_rw, ProcessorType::ARM9, this->header.arm9_rom_offset, this->header.arm9_rom_size, true);

        // ARM9 overlay table

        _WRITE_OVERLAY_TABLE(this->arm9_ovl_table, this->header.arm9_overlay_table_offset, this->header.arm9_overlay_table_size);

        // Write ARM7 ROM

        _WRITE_CODE(this->arm7_rw, ProcessorType::ARM7, this->header.arm7_rom_offset, this->header.arm7_rom_size, false);

        // ARM7 overlay table

        _WRITE_OVERLAY_TABLE(this->arm7_ovl_table, this->header.arm7_overlay_table_offset, this->header.arm7_overlay_table_size);

        // FNT

        const auto fnt_entries_size = gen_fnt.size() * sizeof(nfs::NitroFileSystem::DirectoryNameTableEntry);
        for(auto &fnt_entry: gen_fnt) {
            fnt_entry.start += fnt_entries_size;
        }

        size_t fnt_offset;
        TWL_R_TRY(wf.GetOffset(fnt_offset));
        this->header.fnt_offset = fnt_offset;
        TWL_R_TRY(wf.WriteVector(gen_fnt));
        TWL_R_TRY(wf.WriteBuffer(fnt_data_rw.GetBuffer(), fnt_data_rw.GetBufferSize()));
        size_t fnt_end_offset;
        TWL_R_TRY(wf.GetOffset(fnt_end_offset));
        this->header.fnt_size = fnt_end_offset - fnt_offset;

        // Set FAT values

        this->header.fat_offset = fnt_end_offset;
        this->header.fat_size = gen_fat.size() * sizeof(nfs::NitroFileSystem::FileAllocationTableEntry);
        wf.MoveOffset(this->header.fat_size);
        TWL_R_TRY(wf.WriteEnsureAlignment(SectionAlignment));

        // Banner

        size_t banner_offset;
        TWL_R_TRY(wf.GetOffset(banner_offset));
        this->header.banner_offset = banner_offset;
        TWL_R_TRY(wf.Write(this->banner));
        TWL_R_TRY(wf.WriteEnsureAlignment(SectionAlignment));

        // Adjust absolute FAT offsets

        size_t file_data_offset;
        TWL_R_TRY(wf.GetOffset(file_data_offset));
        for(auto &fat_entry: gen_fat) {
            fat_entry.file_start += file_data_offset;
            fat_entry.file_end += file_data_offset;
        }

        // Write all file contents

        TWL_R_TRY(wf.WriteBuffer(file_data_rw.GetBuffer(), file_data_rw.GetBufferSize()));
        TWL_R_TRY(wf.WriteEnsureAlignment(4));

        // Set header fields

        size_t base_rom_size;
        TWL_R_TRY(wf.GetOffset(base_rom_size));
        this->header.rom_size = base_rom_size;

        auto capacity_size = base_rom_size;
        capacity_size |= capacity_size >> 16;
        capacity_size |= capacity_size >> 8;
        capacity_size |= capacity_size >> 4;
        capacity_size |= capacity_size >> 2;
        capacity_size |= capacity_size >> 1;
        capacity_size++;
        if(capacity_size <= 0x20000) {
            capacity_size = 0x20000;
        }

        auto capacity = -18;
        while(capacity_size != 0) {
            capacity_size >>= 1;
            capacity++;
        }

        this->header.device_capacity = static_cast<u8>(std::min(0, capacity));

        // TODO: RSA signature

        // FAT

        TWL_R_TRY(wf.SetAbsoluteOffset(this->header.fat_offset));
        TWL_R_TRY(wf.WriteVector(gen_fat));

        // Header (finally)

        TWL_R_TRY(wf.SetAbsoluteOffset(0));
        TWL_R_TRY(wf.Write(this->header));

        TWL_R_SUCCEED();
    }

}
