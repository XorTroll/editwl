
#pragma once
#include <mod/mod_Module.hpp>
#include <twl/fmt/fmt_BMG.hpp>
#include <QString>

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

constexpr twl::ResultDescriptionEntry ResultDescriptionTable[] = {
    { ResultEditBMGInvalidEscapeByte, "Invalid escape byte found in BMG text" },
    { ResultEditBMGUnexpectedEscapeOpen, "Unexpected escape opening found in BMG text" },
    { ResultEditBMGUnexpectedEscapeClose, "Unexpected escape closing found in BMG text" },
    { ResultEditBMGUnclosedEscape, "Reached BMG text and with unclosed escape" },
    { ResultBMGInvalidMessageId, "Invalid BMG message ID integer" },
    { ResultBMGInvalidAttributes, "Invalid BMG attributes hex byte array" },
    { ResultLoadBMGMalformedXml, "Malformed XML file format" },
    { ResultLoadBMGXmlInvalidRootTag, "Invalid XML file to parse as BMG: expected root 'bmg' element" },
    { ResultLoadBMGXmlInvalidChildTag, "Invalid XML file to parse as BMG: expected child 'message' element" },
    { ResultLoadBMGXmlMessageIdMismatch, "Invalid XML file to parse as BMG: some messages have ID and others do not" },
    { ResultLoadBMGXmlAttributesMismatch, "Invalid XML file to parse as BMG: messages have different attributes size" },
    { ResultLoadBMGXmlInvalidMessageToken, "Invalid XML file to parse as BMG: invalid message token (expected plain text or escape token)" },
    { ResultBMGInvalidFileId, "Invalid BMG file ID integer" }
};

inline constexpr twl::Result GetResultDescription(const twl::Result rc, std::string &out_desc) {
    TWL_R_TRY(twl::GetResultDescription(rc, out_desc));

    for(twl::u32 i = 0; i < std::size(ResultDescriptionTable); i++) {
        if(ResultDescriptionTable[i].first.value == rc.value) {
            out_desc = ResultDescriptionTable[i].second;
            TWL_R_SUCCEED(); 
        }
    }

    TWL_R_FAIL(twl::ResultUnknownResult);
}

inline std::string FormatResult(const twl::Result rc) {
    std::string desc = "<unknown error: " + QString::number(rc.value, 16).toStdString() + ">";
    ::GetResultDescription(rc, desc);
    return desc;
}

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
twl::Result ParseEncoding(const QString &raw_enc, twl::fmt::BMG::Encoding &out_enc);

twl::Result ParseMessage(const QString &msg, twl::fmt::BMG::Message &out_msg);
twl::Result FormatMessage(const twl::fmt::BMG::Message &msg, QString &out_str);

twl::Result SaveBmgXml(twl::fmt::BMG &bmg, const QString &path);
twl::Result LoadBmgXml(const QString &path, twl::fmt::BMG &out_bmg);
