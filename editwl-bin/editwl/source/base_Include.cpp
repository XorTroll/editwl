#include <base_Include.hpp>

#include <bmg/bmg.hpp>
#include <rom/rom.hpp>

namespace {

    constexpr std::pair<twl::Result, const char*> ResultDescriptionTable[] = {
        { ResultModuleLoadError, "Error loading module" },
        { ResultInvalidModuleSymbols, "Invalid module symbols" },
        { ResultModuleInitializationFailure, "Unable to get module metadata" },

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

}

QString FormatResult(const twl::Result rc) {
    for(const auto &[t_rc, t_desc]: twl::ResultDescriptionTable) {
        if(rc.value == t_rc.value) {
            return t_desc;
        }
    }
    for(const auto &[t_rc, t_desc]: ResultDescriptionTable) {
        if(rc.value == t_rc.value) {
            return t_desc;
        }
    }

    return QString("unknown result: 0x%1").arg(rc.value, 0, 16);
}
