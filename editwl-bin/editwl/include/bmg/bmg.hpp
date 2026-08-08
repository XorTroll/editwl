
#pragma once
#include <twl/fmt/fmt_BMG.hpp>
#include <ui/ui_MainWindow.hpp>

constexpr twl::Result ResultEditBMGInvalidEscapeByte = 0xe001;
constexpr twl::Result ResultEditBMGUnexpectedEscapeOpen = 0xe002;
constexpr twl::Result ResultEditBMGUnexpectedEscapeClose = 0xe003;
constexpr twl::Result ResultEditBMGUnclosedEscape = 0xe004;
constexpr twl::Result ResultBMGInvalidMessageId = 0xe005;
constexpr twl::Result ResultBMGInvalidAttributes = 0xe006;
constexpr twl::Result ResultLoadBMGMalformedXml = 0xe007;
constexpr twl::Result ResultLoadBMGXmlInvalidRootTag = 0xe008;
constexpr twl::Result ResultLoadBMGXmlInvalidChildTag = 0xe009;
constexpr twl::Result ResultLoadBMGXmlMessageIdMismatch = 0xe00a;
constexpr twl::Result ResultLoadBMGXmlAttributesMismatch = 0xe00b;
constexpr twl::Result ResultLoadBMGXmlInvalidMessageToken = 0xe00c;
constexpr twl::Result ResultBMGInvalidFileId = 0xe00d;

QString FormatHexByteArray(const std::vector<twl::u8> &data);
bool ParseHexByteArray(const QString &raw, std::vector<twl::u8> &out_arr);

template<typename T>
inline bool ParseStringInteger(const QString &raw, T &out_val) {
    // Try decimal first, hex otherwise
    bool parse_ok = false;

    const auto dec_val = raw.toInt(&parse_ok);
    if(parse_ok) {
        out_val = static_cast<T>(dec_val);
        return true;
    }

    const auto hex_val = raw.toInt(&parse_ok, 16);
    if(parse_ok) {
        out_val = static_cast<T>(hex_val);
        return true;
    }

    return false;
}

#define NEDIT_MOD_BMG_FORMAT_BMG_EXTENSION "bmg"
#define NEDIT_MOD_BMG_FORMAT_BMG_FILTER "Binary message file (*." NEDIT_MOD_BMG_FORMAT_BMG_EXTENSION ")"

#define NEDIT_MOD_BMG_FORMAT_XML_EXTENSION "xml"
#define NEDIT_MOD_BMG_FORMAT_XML_FILTER "XML message file (*." NEDIT_MOD_BMG_FORMAT_XML_EXTENSION ")"

QString FormatEncoding(const twl::fmt::BMG::Encoding enc);
std::string FormatEscape(const twl::fmt::BMG::MessageEscape &esc);
twl::Result ParseEncoding(const QString &raw_enc, twl::fmt::BMG::Encoding &out_enc);

twl::Result ParseMessage(const QString &msg, twl::fmt::BMG::Message &out_msg);
twl::Result FormatMessage(const twl::fmt::BMG::Message &msg, QString &out_str);

twl::Result SaveBmgXml(twl::fmt::BMG &bmg, const QString &path);
twl::Result LoadBmgXml(const QString &path, twl::fmt::BMG &out_bmg);

namespace ui::bmg {

    bool HandleInputFile(MainWindow *win, const QString &path);

}

namespace cli::bmg {

    void HandleCommand(const std::vector<std::string> &args);

}
