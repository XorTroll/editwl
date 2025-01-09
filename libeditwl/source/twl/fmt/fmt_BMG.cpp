#include <twl/fmt/fmt_BMG.hpp>
#include <twl/util/util_Align.hpp>

namespace twl::fmt {

    namespace {

        constexpr size_t DataAlignment = 0x20;
        constexpr char16_t EscapeCharacter = '\u001A';

        inline Result ReadCharacter(fs::File &rf, const BMG::Encoding enc, char16_t &out_ch) {
            switch(enc) {
                case BMG::Encoding::CP1252: {
                    // TODO: unsupported yet
                    TWL_R_FAIL(ResultBMGInvalidUnsupportedEncoding);
                }
                case BMG::Encoding::UTF16: {
                    TWL_R_TRY(rf.Read(out_ch));
                    break;
                }
                case BMG::Encoding::UTF8: {
                    char ch;
                    TWL_R_TRY(rf.Read(ch));
                    out_ch = ch;
                    break;
                }
                case BMG::Encoding::ShiftJIS: {
                    // TODO: unsupported yet
                    TWL_R_FAIL(ResultBMGInvalidUnsupportedEncoding);
                }
            }

            TWL_R_SUCCEED();
        }

        inline Result WriteCharacter(fs::File &wf, const BMG::Encoding enc, const char16_t &ch) {
            switch(enc) {
                case BMG::Encoding::CP1252: {
                    // TODO: unsupported yet
                    TWL_R_FAIL(ResultBMGInvalidUnsupportedEncoding);
                }
                case BMG::Encoding::UTF16: {
                    TWL_R_TRY(wf.Write(ch));
                    break;
                }
                case BMG::Encoding::UTF8: {
                    TWL_R_TRY(wf.Write((char)ch));
                    break;
                }
                case BMG::Encoding::ShiftJIS: {
                    // TODO: unsupported yet
                    TWL_R_FAIL(ResultBMGInvalidUnsupportedEncoding);
                }
            }

            TWL_R_SUCCEED();
        }

    }

    void BMG::CreateFrom(const Encoding enc, const bool has_message_ids, const size_t attr_size, const std::vector<Message> &msgs, const u32 file_id) {
        // The rest of the fields will be automatically set when writing
    
        this->header.encoding = enc;
        this->header.unk_1 = 0;
        this->header.unk_2 = 0;
        this->header.unk_3 = 0;
        this->header.unk_4 = 0;
        this->header.unk_5 = 0;

        this->info.SetAttributesSize(attr_size);
        this->info.file_id = file_id;
        this->messages = msgs;

        if(has_message_ids) {
            this->msg_id.emplace();
        }
        else {
            this->msg_id = {};
        }
    }

    Result BMG::ReadValidateFrom(fs::File &rf) {
        TWL_R_TRY(rf.Read(this->header));
        if(!this->header.IsValid()) {
            TWL_R_FAIL(ResultBMGInvalidHeader);
        }
        if(this->header.section_count < 2) {
            TWL_R_FAIL(ResultBMGUnexpectedSectionCount);
        }

        TWL_R_TRY(rf.Read(this->info));
        if(!this->info.IsValid()) {
            TWL_R_FAIL(ResultBMGInvalidInfoSection);
        }

        if(!IsValidEncoding(this->header.encoding)) {
            TWL_R_FAIL(ResultBMGInvalidUnsupportedEncoding);
        }
        
        const auto data_offset = sizeof(Header) + this->info.block_size;
        TWL_R_TRY(rf.SetAbsoluteOffset(data_offset));
        TWL_R_TRY(rf.Read(this->data));
        if(!this->data.IsValid()) {
            TWL_R_FAIL(ResultBMGInvalidDataSection);
        }

        if(this->header.section_count >= 3) {
            const auto msg_id_offset = sizeof(Header) + this->info.block_size + this->data.block_size;
            TWL_R_TRY(rf.SetAbsoluteOffset(msg_id_offset));

            MessageIdSection msg_id;
            TWL_R_TRY(rf.Read(msg_id));
            if(!msg_id.IsValid()) {
                TWL_R_FAIL(ResultBMGInvalidMessageIdSection);
            }

            this->msg_id = msg_id;
        }

        TWL_R_SUCCEED();
    }

    Result BMG::ReadAllFrom(fs::File &rf) {
        this->messages.clear();

        const auto data_offset = sizeof(Header) + this->info.block_size;
        const auto entries_offset = sizeof(Header) + sizeof(InfoSection);
        const auto messages_offset = data_offset + sizeof(DataSection);

        this->messages.reserve(this->info.entry_count);
        TWL_R_TRY(rf.SetAbsoluteOffset(entries_offset));

        const auto attrs_size = this->info.entry_size - InfoSection::OffsetSize;
        for(auto i = 0; i < this->info.entry_count; i++) {
            u32 offset = 0;
            TWL_R_TRY(rf.Read(offset));

            Message msg = {};
            for(u32 j = 0; j < attrs_size; j++) {
                u8 attr;
                TWL_R_TRY(rf.Read(attr));

                msg.attrs.push_back(attr);
            }

            size_t old_offset;
            TWL_R_TRY(rf.GetOffset(old_offset));

            TWL_R_TRY(rf.SetAbsoluteOffset(messages_offset + offset));

            MessageToken cur_token = {};
            while(true) {
                char16_t ch;
                TWL_R_TRY(ReadCharacter(rf, this->header.encoding, ch));

                if(ch == EscapeCharacter) {
                    if(cur_token.text.length() > 0) {
                        cur_token.type = MessageTokenType::Text;
                        msg.msg.push_back(cur_token);
                    }
                    else if(cur_token.escape.esc_data.size() > 0) {
                        // No unfinished escapes should exist
                        TWL_R_FAIL(ResultBMGInvalidEscapeSequence);
                    }

                    cur_token = {
                        .type = MessageTokenType::Escape
                    };

                    u8 escape_size;
                    TWL_R_TRY(rf.Read(escape_size));
                    escape_size -= GetCharacterSize(this->header.encoding);
                    escape_size -= sizeof(u8);

                    cur_token.escape.esc_data.reserve(escape_size);
                    for(u32 i = 0; i < escape_size; i++) {
                        u8 escape_byte;
                        TWL_R_TRY(rf.Read(escape_byte));

                        cur_token.escape.esc_data.push_back(escape_byte);
                    }

                    msg.msg.push_back(cur_token);
                    cur_token = {};
                }
                else if(ch == '\u0000') {
                    if(cur_token.text.length() > 0) {
                        cur_token.type = MessageTokenType::Text;
                        msg.msg.push_back(cur_token);
                    }
                    else if(cur_token.escape.esc_data.size() > 0) {
                        // No unfinished escapes should exist
                        TWL_R_FAIL(ResultBMGInvalidEscapeSequence);
                    }

                    break;
                }
                else {
                    if(cur_token.escape.esc_data.size() > 0) {
                        // No unfinished escapes should exist
                        TWL_R_FAIL(ResultBMGInvalidEscapeSequence);
                    }

                    cur_token.text.push_back(ch);
                }
            }

            this->messages.push_back(msg);

            TWL_R_TRY(rf.SetAbsoluteOffset(old_offset));
        }

        if(this->HasMessageIds()) {
            const auto msg_id_offset = sizeof(Header) + this->info.block_size + this->data.block_size;
            const auto ids_offset = msg_id_offset + sizeof(MessageIdSection);
            TWL_R_TRY(rf.SetAbsoluteOffset(ids_offset));

            for(u32 i = 0; i < this->msg_id->id_count; i++) {
                u32 id;
                TWL_R_TRY(rf.Read(id));

                if(i < this->messages.size()) {
                    this->messages.at(i).id = id;
                }
            }
        }

        TWL_R_SUCCEED();
    }

    Result BMG::WriteTo(fs::File &wf) {
        // Ensure message attributes are correct
        const auto attrs_size = this->info.entry_size - InfoSection::OffsetSize;
        for(const auto &msg: this->messages) {
            if(msg.attrs.size() != attrs_size) {
                TWL_R_FAIL(ResultBMGInvalidMessageAttributeSize);
            }
        }

        const auto info_offset = sizeof(Header);
        const auto entries_offset = info_offset + sizeof(InfoSection);
        const auto entries_size = this->messages.size() * this->info.entry_size;
        this->info.EnsureMagic();
        this->info.entry_count = this->messages.size();
        this->info.block_size = util::AlignUp(sizeof(InfoSection) + entries_size, DataAlignment);
        TWL_R_TRY(wf.SetAbsoluteOffset(info_offset));
        TWL_R_TRY(wf.Write(this->info));

        const auto data_offset = info_offset + this->info.block_size;

        TWL_R_TRY(wf.SetAbsoluteOffset(entries_offset + entries_size));

        TWL_R_TRY(wf.WriteEnsureAlignment(DataAlignment));

        const auto messages_offset = data_offset + sizeof(DataSection);
        u32 cur_message_rel_offset = GetCharacterSize(this->header.encoding); // For some reason, all BMGs apparently leave an unused null character at the start of the section...
        for(auto i = 0; i < this->info.entry_count; i++) {
            const auto cur_msg = this->messages.at(i);

            TWL_R_TRY(wf.SetAbsoluteOffset(entries_offset + i * this->info.entry_size));
            TWL_R_TRY(wf.Write(cur_message_rel_offset));
            TWL_R_TRY(wf.WriteVector(cur_msg.attrs));

            TWL_R_TRY(wf.SetAbsoluteOffset(messages_offset + cur_message_rel_offset));
            for(const auto &token: cur_msg.msg) {
                switch(token.type) {
                    case MessageTokenType::Escape: {
                        TWL_R_TRY(WriteCharacter(wf, this->header.encoding, EscapeCharacter));
                        TWL_R_TRY(wf.Write(static_cast<u8>(token.GetByteLength(this->header.encoding))));

                        TWL_R_TRY(wf.WriteVector(token.escape.esc_data));
                        break;
                    }
                    case MessageTokenType::Text: {
                        for(const auto &ch: token.text) {
                            TWL_R_TRY(WriteCharacter(wf, this->header.encoding, ch));
                        }
                        break;
                    }
                }
            }
            TWL_R_TRY(WriteCharacter(wf, this->header.encoding, '\u0000'));

            size_t end_offset;
            TWL_R_TRY(wf.GetOffset(end_offset));

            cur_message_rel_offset = end_offset - messages_offset;
        }

        size_t data_pad_size;
        TWL_R_TRY(wf.WriteEnsureAlignmentPadding(DataAlignment, data_pad_size));

        const auto data_end_offset = messages_offset + cur_message_rel_offset + data_pad_size;

        if(this->HasMessageIds()) {
            const auto msg_id_offset = data_end_offset;
            const auto ids_offset = msg_id_offset + sizeof(MessageIdSection);

            TWL_R_TRY(wf.SetAbsoluteOffset(ids_offset));
            for(const auto &msg: this->messages) {
                TWL_R_TRY(wf.Write(msg.id));
            }

            TWL_R_TRY(wf.WriteEnsureAlignment(DataAlignment));
        }

        size_t cur_file_size;
        TWL_R_TRY(wf.GetOffset(cur_file_size));

        this->header.EnsureMagic();
        this->header.file_size = cur_file_size;
        
        this->header.section_count = 2;
        if(this->HasMessageIds()) {
            this->header.section_count++;
        }

        TWL_R_TRY(wf.SetAbsoluteOffset(0));
        TWL_R_TRY(wf.Write(this->header));

        this->data.EnsureMagic();
        this->data.block_size = data_end_offset - data_offset;
        TWL_R_TRY(wf.SetAbsoluteOffset(data_offset));
        TWL_R_TRY(wf.Write(this->data));

        if(this->HasMessageIds()) {
            this->msg_id->EnsureMagic();
            this->msg_id->id_count = this->messages.size();   

            const auto msg_id_offset = sizeof(Header) + this->info.block_size + this->data.block_size;
            TWL_R_TRY(wf.SetAbsoluteOffset(msg_id_offset));
            TWL_R_TRY(wf.Write(*this->msg_id));
        }

        TWL_R_SUCCEED();
    }

}
