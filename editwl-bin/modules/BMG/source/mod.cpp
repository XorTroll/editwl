#include <mod/mod_Module.hpp>
#include <args.hxx>
#include <twl/fmt/fmt_BMG.hpp>
#include <twl/util/util_String.hpp>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ui/ui_BmgSubWindow.hpp>
#include <QFileInfo>

namespace {

    std::string FormatEscape(const twl::fmt::BMG::MessageEscape &esc) {
        std::stringstream strm;
        strm << "{";
        for(twl::u32 i = 0; i < esc.esc_data.size(); i++) {
            strm << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<twl::u32>(esc.esc_data.at(i));
            if((i + 1) < esc.esc_data.size()) {
                strm << "-";
            }
        }
        strm << "}";
        return strm.str();
    }

    bool BuildMessage(const std::string &input, twl::fmt::BMG::Message &out_msg) {
        out_msg = {};
        twl::fmt::BMG::MessageToken cur_token = {};
        std::string cur_escape_byte;

        #define _ETWL_MOD_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN \
            if(cur_token.text.length() > 0) { \
                cur_token.type = twl::fmt::BMG::MessageTokenType::Text; \
                out_msg.msg.push_back(cur_token); \
            }

        #define _ETWL_MOD_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE \
            try { \
                int byte = std::stoi(cur_escape_byte, nullptr, 16); \
                cur_token.escape.esc_data.push_back(byte & 0xFF); \
                cur_escape_byte = ""; \
            } \
            catch(std::exception&) { \
                std::cerr << "Formatting error: invalid escape byte supplied (" << cur_escape_byte << "), they must be in hexadecimal like here: {FF-00-AA-12}" << std::endl; \
                return false; \
            }

        for(const auto &ch: input) {
            if(ch == '{') {
                if(cur_token.type == twl::fmt::BMG::MessageTokenType::Escape) {
                    std::cerr << "Formatting error: parsed escape opening symbol '{' with an already opened escape" << std::endl;
                    return false;
                }

                _ETWL_MOD_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN
                cur_token = {
                    .type = twl::fmt::BMG::MessageTokenType::Escape
                };
            }
            else if(ch == '}') {
                if(cur_token.type != twl::fmt::BMG::MessageTokenType::Escape) {
                    std::cerr << "Formatting error: parsed escape closing symbol '}' without any previous escape opening symbol '{'" << std::endl;
                    return false;
                }

                if(!cur_escape_byte.empty()) {
                    _ETWL_MOD_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE
                }

                out_msg.msg.push_back(cur_token);
                cur_token = {};
            }
            else if(ch == '-') {
                if(cur_token.type != twl::fmt::BMG::MessageTokenType::Escape) {
                    std::cerr << "Formatting error: parsed escape closing symbol '}' without any previous escape opening symbol '{'" << std::endl;
                    return false;
                }
                if(cur_escape_byte.empty()) {
                    std::cerr << "Formatting error: found empty escape byte" << std::endl;
                    return false;
                }

                _ETWL_MOD_BMG_MSG_BUILD_PUSH_ESCAPE_BYTE
            }
            else {
                if(cur_token.type == twl::fmt::BMG::MessageTokenType::Escape) {
                    cur_escape_byte += ch;
                }
                else {
                    cur_token.text += ch;
                }
            }
        }

        if(cur_token.type == twl::fmt::BMG::MessageTokenType::Escape) {
            std::cerr << "Formatting error: reached end with unclosed escape" << std::endl;
            return false;
        }

        _ETWL_MOD_BMG_MSG_BUILD_CHECK_PUSH_TEXT_TOKEN

        return true;
    }

    void PrintMessage(const twl::fmt::BMG::Message &msg) {
        for(const auto &msg_token: msg.msg) {
            switch(msg_token.type) {
                case twl::fmt::BMG::MessageTokenType::Escape: {
                    std::cout << FormatEscape(msg_token.escape);
                    break;
                }
                case twl::fmt::BMG::MessageTokenType::Text: {
                    const auto utf8_text = twl::util::ConvertFromUnicode(msg_token.text);
                    std::cout << utf8_text;
                    break;
                }
            }
        }
    }

    bool ReadEncoding(const std::string &enc_str, twl::fmt::BMG::Encoding &out_enc) {
        const auto enc_l_str = twl::util::ToLowerString(enc_str);
        
        if((enc_l_str == "utf16") || (enc_l_str == "utf-16")) {
            out_enc = twl::fmt::BMG::Encoding::UTF16;
            return true;
        }
        if((enc_l_str == "utf8") || (enc_l_str == "utf-8")) {
            out_enc = twl::fmt::BMG::Encoding::UTF8;
            return true;
        }

        return false;
    }

    void ListBmg(const std::string &bmg_path, const bool verbose) {
        twl::fs::StdioFile bmg_file(bmg_path);
        auto rc = bmg_file.OpenRead();
        if(rc.IsSuccess()) {
            twl::ScopeGuard close_file([&]() {
                bmg_file.Close();
            });

            twl::fmt::BMG bmg;
            rc = bmg.ReadFrom(bmg_file);
            if(rc.IsSuccess()) {
                if(verbose) {
                    std::cout << " - Encoding: " << FormatEncoding(bmg.header.encoding).toStdString() << std::endl;
                    std::cout << " - Message extra attribute size: 0x" << std::hex << bmg.info.GetAttributesSize() << std::dec << std::endl;

                    std::cout << " - Messages:" << std::endl;
                }
                for(const auto &msg: bmg.messages) {
                    PrintMessage(msg);

                    if(verbose) {
                        if(bmg.info.GetAttributesSize() > 0) {
                            std::cout << " (";
                            for(const auto &attr_byte: msg.attrs) {
                                std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<twl::u32>(attr_byte);
                            }
                            std::cout << ")";
                            
                        }
                    }

                    std::cout << std::endl;
                }
            }
            else {
                std::cerr << "Unable to read BMG file '" << bmg_path << "': " << rc.GetDescription() << std::endl;
                return;
            }
        }
        else {
            std::cerr << "Unable to open BMG file '" << bmg_path << "': " << rc.GetDescription() << std::endl;
            return;
        }
    }

    void GetBmgString(const std::string &bmg_path, const std::string &idx_str) {
        int idx;
        try {
            idx = std::stoi(idx_str);
        }
        catch(std::invalid_argument &ex) {
            std::cerr << "Invalid index supplied (invalid argument): " << ex.what() << std::endl;
            return;
        }
        catch(std::out_of_range &ex) {
            std::cerr << "Invalid index supplied (out of range): " << ex.what() << std::endl;
            return;
        }

        twl::fs::StdioFile bmg_file(bmg_path);
        auto rc = bmg_file.OpenRead();
        if(rc.IsSuccess()) {
            twl::ScopeGuard close_file([&]() {
                bmg_file.Close();
            });

            twl::fmt::BMG bmg;
            rc = bmg.ReadFrom(bmg_file);
            if(rc.IsSuccess()) {
                if(idx < bmg.messages.size()) {
                    const auto &msg = bmg.messages.at(idx);
                    PrintMessage(msg);
                    std::cout << std::endl;
                }
                else {
                    std::cerr << "Invalid index supplied (out of bounds): BMG only has " << bmg.messages.size() << " messages..." << std::endl;
                    return;
                }
            }
            else {
                std::cerr << "Unable to read BMG file '" << bmg_path << "': " << rc.GetDescription() << std::endl;
                return;
            }
        }
        else {
            std::cerr << "Unable to open BMG file '" << bmg_path << "': " << rc.GetDescription() << std::endl;
            return;
        }
    }

    void ConvertToXml(const std::string &bmg_path, const std::string &out_xml_path) {
        twl::fs::StdioFile bmg_file(bmg_path);
        auto rc = bmg_file.OpenRead();
        if(rc.IsSuccess()) {
            twl::ScopeGuard close_file([&]() {
                bmg_file.Close();
            });

            twl::fmt::BMG bmg;
            auto rc = bmg.ReadFrom(bmg_file);
            if(rc.IsSuccess()) {
                rc = SaveBmgXml(bmg, QString::fromStdString(out_xml_path));
                if(rc.IsFailure()) {
                    std::cerr << "Unable to convert BMG and save XML file to '" << out_xml_path << "': " << rc.GetDescription() << std::endl;
                    return;
                }
            }
            else {
                std::cerr << "Unable to read BMG file '" << bmg_path << "': " << rc.GetDescription() << std::endl;
                return;
            }
        }
        else {
            std::cerr << "Unable to open BMG file '" << bmg_path << "': " << rc.GetDescription() << std::endl;
            return;
        }
    }

    void CreateFromXml(const std::string &xml_path, const std::string &out_bmg_path) {
        twl::fmt::BMG bmg;
        auto rc = LoadBmgXml(QString::fromStdString(xml_path), bmg);
        if(rc.IsSuccess()) {
            twl::fs::StdioFile out_bmg_file(out_bmg_path);
            rc = out_bmg_file.OpenWrite();
            if(rc.IsSuccess()) {
                twl::ScopeGuard close_file([&]() {
                    out_bmg_file.Close();
                });

                rc = bmg.WriteTo(out_bmg_file);
                if(rc.IsFailure())  {
                    std::cerr << "Unable to save BMG file to '" << out_bmg_path << "': " << rc.GetDescription() << std::endl;
                    return;
                }
            }
            else {
                std::cerr << "Unable to open output BMG file '" << out_bmg_path << "': " << rc.GetDescription() << std::endl;
                return;
            }
        }
        else {
            std::cerr << "Unable to load XML file '" << xml_path << "' as BMG: " << rc.GetDescription() << std::endl;
            return;
        }
    }

    void HandleCommand(const std::vector<std::string> &args) {
        args::ArgumentParser parser("Module for BMG files");
        args::HelpFlag help(parser, "help", "Displays this help menu", {'h', "help"});

        args::Group commands(parser, "Commands:", args::Group::Validators::Xor);

        args::Command list(commands, "list", "List BMG messages");
        args::Group list_required(list, "", args::Group::Validators::All);
        args::ValueFlag<std::string> list_bmg_file(list_required, "bmg_file", "Input BMG file", {'i', "in"});
        args::Flag list_verbose(list, "verbose", "Print more information, not just the messages", {'v', "verbose"});

        args::Command get(commands, "get", "Get specific BMG message by index");
        args::Group get_required(get, "", args::Group::Validators::All);
        args::ValueFlag<std::string> get_bmg_file(get_required, "bmg_file", "Input BMG file", {'i', "in"});
        args::ValueFlag<std::string> get_idx(get_required, "idx", "BMG message index", {"idx"});

        args::Command create(commands, "create", "Create BMG file from XML file");
        args::Group create_required(create, "", args::Group::Validators::All);
        args::ValueFlag<std::string> create_txt_file(create_required, "xml_file", "Input XML file", {'i', "in"});
        args::ValueFlag<std::string> create_out_bmg_file(create_required, "out_bmg_file", "Output BMG file", {'o', "out"});

        args::Command convert(commands, "convert", "Convert BMG file to XML file");
        args::Group convert_required(convert, "", args::Group::Validators::All);
        args::ValueFlag<std::string> convert_bmg_file(convert_required, "bmg_file", "Input BMG file", {'i', "in"});
        args::ValueFlag<std::string> convert_out_xml_file(convert_required, "out_xml_file", "Output XML file", {'o', "out"});

        try {
            parser.ParseArgs(args);
        }
        catch(std::exception &e) {
            std::cerr << parser;
            std::cout << e.what() << std::endl;
            return;
        }

        if(list) {
            const auto bmg_path = list_bmg_file.Get();
            const auto verbose = list_verbose.Get();
            ListBmg(bmg_path, verbose);
        }
        else if(get) {
            const auto bmg_path = get_bmg_file.Get();
            const auto idx_str = get_idx.Get();
            GetBmgString(bmg_path, idx_str);
        }
        else if(create) {
            const auto xml_path = create_txt_file.Get();
            const auto out_bmg_path = create_out_bmg_file.Get();
            CreateFromXml(xml_path, out_bmg_path);
        }
        else if(convert) {
            const auto bmg_path = convert_bmg_file.Get();
            const auto out_xml_path = convert_out_xml_file.Get();
            ConvertToXml(bmg_path, out_xml_path);
        }
    }

}

ETWL_MOD_DEFINE_START(
    "BMG",
    "BMG support and utilities",
    "XorTroll",
    ETWL_MAJOR,ETWL_MINOR,ETWL_MICRO,ETWL_BUGFIX,
    ResultDescriptionTable, std::size(ResultDescriptionTable)
)

ETWL_MOD_DEFINE_REGISTER_COMMAND("bmg", HandleCommand)

ETWL_MOD_DEFINE_END()

ETWL_MOD_SYMBOL bool ETWL_MOD_TRY_HANDLE_INPUT_SYMBOL(const QString &path, etwl::mod::Context *ctx) {
    twl::fs::StdioFile rf(path.toStdString());
    auto rc = rf.OpenRead();
    if(rc.IsSuccess()) {
        twl::ScopeGuard close_f([&]() {
            rf.Close();
        });

        twl::fmt::BMG bmg;
        rc = bmg.ReadFrom(rf);
        if(rc.IsSuccess()) {
            auto subwin = new ui::BmgSubWindow(ctx, std::move(bmg), rf.GetPath());

            QFileInfo file_info(QString::fromStdString(rf.GetPath()));
            subwin->setWindowTitle("BMG editor - " + file_info.fileName());

            ctx->ShowSubWindow(subwin);
            return true;
        }
    }

    return false;
}
