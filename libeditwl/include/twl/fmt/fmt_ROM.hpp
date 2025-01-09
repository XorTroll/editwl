
#pragma once
#include <twl/fmt/nfs/nfs_NitroFs.hpp>
#include <twl/fs/fs_FileFormat.hpp>
#include <twl/util/util_String.hpp>
#include <twl/gfx/gfx_BannerIcon.hpp>
#include <optional>

namespace twl::fmt {

    struct ROM : public fs::FileFormat, public nfs::NitroFileSystemFormat {

        enum class ProcessorType : u8 {
            ARM7,
            ARM9
        };

        enum class Language : u8 {
            Ja,
            En,
            Fr,
            Ge,
            It,
            Es,

            Count
        };

        enum class Region : u8 {
            Normal = 0x00,
            Korea = 0x40,
            China = 0x80
        };

        enum class UnitCode : u8 {
            NDS = 0x00,
            NDS_NDSi = 0x02,
            NDSi = 0x03
        };

        enum class AutostartFlags : u8 {
            None = 0,
            SkipHealthSafetyPress = TWL_BITMASK(2)
        };

        struct Header {
            char game_title[12];
            char game_code[4];
            char developer_code[2];
            UnitCode unit_code;
            u8 encryption_seed_select;
            u8 device_capacity;
            u8 reserved_1[7];
            union {
                u16 ndsi;
                struct {
                    u8 reserved;
                    Region region;
                } nds;
            } game_revision;
            u8 version;
            AutostartFlags autostart_flags;
            u32 arm9_rom_offset;
            u32 arm9_entry_address;
            u32 arm9_ram_address;
            u32 arm9_rom_size;
            u32 arm7_rom_offset;
            u32 arm7_entry_address;
            u32 arm7_ram_address;
            u32 arm7_rom_size;
            u32 fnt_offset;
            u32 fnt_size;
            u32 fat_offset;
            u32 fat_size;
            u32 arm9_overlay_table_offset;
            u32 arm9_overlay_table_size;
            u32 arm7_overlay_table_offset;
            u32 arm7_overlay_table_size;
            u32 normal_card_control_register_settings;
            u32 secure_card_control_register_settings;
            u32 banner_offset;
            u16 secure_area_crc;
            u16 secure_transfer_timeout;
            u32 arm9_autoload;
            u32 arm7_autoload;
            u64 secure_area_disable;
            u32 rom_size;
            u32 header_size;
            u8 reserved_2[56];
            u8 nintendo_logo[156];
            u16 nintendo_logo_crc;
            u16 header_crc;
            u32 debug_rom_offset;
            u32 debug_rom_size;
            u32 debug_ram_address;
            u32 reserved_3;
            u8 reserved_4[0x90];

            // Note: helpers since these strings don't neccessarily end with a null character, so std::string(<c_str>) wouldn't work as expected there

            inline std::string GetGameTitle() {
                return util::GetNonNullTerminatedCString(this->game_title);
            }

            inline void SetGameTitle(const std::string &game_title_str) {
                return util::SetNonNullTerminatedCString(this->game_title, game_title_str);
            }

            inline std::string GetGameCode() {
                return util::GetNonNullTerminatedCString(this->game_code);
            }

            inline void SetGameCode(const std::string &game_code_str) {
                return util::SetNonNullTerminatedCString(this->game_code, game_code_str);
            }
            
            inline std::string GetDeveloperCode() {
                return util::GetNonNullTerminatedCString(this->developer_code);
            }

            inline void SetDeveloperCode(const std::string &dev_code_str) {
                return util::SetNonNullTerminatedCString(this->developer_code, dev_code_str);
            }
        };
        static_assert(sizeof(Header) == 0x200);

        static constexpr u32 GameTitleLength = 128;

        struct Banner {
            u8 version;
            u8 reserved_1;
            u16 crc16_v1;
            u8 reserved_2[28];
            u8 icon_chr[gfx::IconCharSize];
            u8 icon_plt[gfx::IconPaletteSize];
            char16_t game_titles[static_cast<u32>(Language::Count)][GameTitleLength];

            inline std::u16string GetGameTitle(const Language lang) {
                if(lang < Language::Count) {
                    return util::GetNonNullTerminatedCString(this->game_titles[static_cast<u32>(lang)]);
                }
                return u"";
            }

            inline void SetGameTitle(const Language lang, const std::u16string &game_title_str) {
                if(lang < Language::Count) {
                    util::SetNonNullTerminatedCString(this->game_titles[static_cast<u32>(lang)], game_title_str);
                }
            }
        };

        struct OverlayTableEntry {
            u32 id;
            u32 ram_address;
            u32 ram_size;
            u32 bss_size;
            u32 static_init_start_address;
            u32 static_init_end_address;
            u32 file_id;
            u32 compressed_size_and_flags;
        };
        static_assert(sizeof(OverlayTableEntry) == 0x20);
        
        struct NitroFooter {
            u32 code;
            u32 start_module_params_offset;
            u32 unk;

            static constexpr u32 Code = 0xDEC00621;
        };
        static_assert(sizeof(NitroFooter) == 0xC);

        struct StartModuleParams {
            u32 autoload_list_start;
            u32 autoload_list_end;
            u32 autoload_start;
            u32 static_bss_start;
            u32 static_bss_end;
            u32 compressed_static_end;
            u32 sdk_version;
            u32 nitro_code_le;
            u32 nitro_code_be;
        };
        static_assert(sizeof(StartModuleParams) == 0x24);

        static constexpr size_t SectionAlignment = 0x200;

        Header header;
        Banner banner;
        std::vector<std::string> lib_symbols;
        fs::BufferReaderWriter arm7_rw;
        fs::BufferReaderWriter arm9_rw;
        std::optional<NitroFooter> footer;
        std::optional<StartModuleParams> start_module_params;
        std::vector<OverlayTableEntry> arm7_ovl_table;
        std::vector<OverlayTableEntry> arm9_ovl_table;

        ROM() {}
        ROM(const ROM&) = delete;
        ROM(ROM&&) = default;
        
        Result ReadValidateFrom(fs::File &rf) override;
        Result ReadAllFrom(fs::File &rf) override;
        Result WriteTo(fs::File &wf) override;

        inline Result CreateOverlayFile(nfs::NitroFileSystemFile &file, const OverlayTableEntry &entry) {
            TWL_R_TRY(this->CreateFileById(file, entry.file_id));
            TWL_R_SUCCEED();
        }
    };

    TWL_ENUM_BIT_OPERATORS(ROM::AutostartFlags, u8)

}
