#include <twl/fs/fs_File.hpp>
#include <cstring>

namespace twl::fs {

    void BufferReaderWriter::CreateAllocate(const size_t buf_size) {
        this->Dispose();
        this->buf = new u8[buf_size]();
        this->buf_size = buf_size;
    }
    
    void BufferReaderWriter::CreateFrom(void *buf, const size_t buf_size, const bool transfer_ownership) {
        this->Dispose();
        if(transfer_ownership) {
            this->buf = buf;
        }
        else {
            auto owned_buf = new u8[buf_size]();
            memcpy(owned_buf, buf, buf_size);
            this->buf = owned_buf;
        }
        this->buf_size = buf_size;
    }

    Result BufferReaderWriter::SetOffset(const ssize_t offset, const Whence whence) {
        switch(whence) {
            case Whence::Begin: {
                if(offset > this->buf_size) {
                    TWL_R_FAIL(ResultUnableToSeekBuffer);
                }

                this->offset = offset;
                break;
            }
            case Whence::Current: {
                if((this->offset + offset) > this->buf_size) {
                    // Seeking beyond (in write mode) == expanding the buffer
                    const auto diff = (this->offset + offset) - this->buf_size;
                    auto empty_buf = new u8[diff]();
                    TWL_R_TRY(this->WriteBuffer(empty_buf, diff));
                }

                this->offset += offset;
                break;
            }
            default: {
                TWL_R_FAIL(ResultInvalidSeekWhence);
            }
        }

        TWL_R_SUCCEED();
    }

    Result BufferReaderWriter::ReadBuffer(void *read_buf, const size_t read_size) {
        if((this->offset + read_size) > this->buf_size) {
            TWL_R_FAIL(ResultUnableToReadBuffer);
        }

        const auto actual_read_size = std::min(read_size, this->buf_size - this->offset);
        memcpy(read_buf, reinterpret_cast<u8*>(this->buf) + this->offset, read_size);

        this->offset += read_size;
        TWL_R_SUCCEED();
    }

    Result BufferReaderWriter::WriteBuffer(const void *write_buf, const size_t write_size) {
        if((this->offset + write_size) >= this->buf_size) {
            // Extend buffer
            const auto new_size = this->offset + write_size;
            auto new_buf = new u8[new_size]();
            memcpy(new_buf, this->buf, this->offset);
            memcpy(new_buf + this->offset, write_buf, write_size);

            auto old_buf = reinterpret_cast<u8*>(this->buf);
            this->buf = new_buf;
            this->buf_size = new_size;
            this->offset = new_size;

            delete[] old_buf;
            TWL_R_SUCCEED();
        }

        memcpy(reinterpret_cast<u8*>(this->buf) + this->offset, write_buf, write_size);
        this->offset += write_size;

        TWL_R_SUCCEED();
    }

    Result File::DecompressRead() {
        size_t comp_size;
        TWL_R_TRY(this->GetSizeImpl(comp_size));

        auto comp_buf = new u8[comp_size]();
        TWL_R_TRY(this->ReadBufferImpl(comp_buf, comp_size));

        ScopeGuard delete_comp_buf([&]() {
            delete[] comp_buf;
        });

        u8 *decomp_buf;
        size_t decomp_size;
        size_t dummy_size;
        TWL_R_TRY(util::LzDecompress(comp_buf, decomp_buf, decomp_size, this->lz_ver, dummy_size));

        this->decomp_rw.CreateFrom(decomp_buf, decomp_size);

        TWL_R_SUCCEED();
    }

    Result File::CompressWrite() {
        if(CanWriteWithMode(this->mode)) {
            u8 *comp_buf;
            size_t comp_size;
            TWL_R_TRY(util::LzCompress(reinterpret_cast<const u8*>(this->decomp_rw.GetBuffer()), this->decomp_rw.GetBufferSize(), this->lz_ver, util::DefaultRepeatSize, comp_buf, comp_size));

            ScopeGuard delete_comp_buf([&]() {
                delete[] comp_buf;
            });

            TWL_R_TRY(this->SetOffsetImpl(0, Whence::Begin));
            TWL_R_TRY(this->WriteBufferImpl(comp_buf, comp_size));
        }

        this->decomp_rw.Dispose();

        TWL_R_SUCCEED();
    }

    Result File::Open(const FileMode mode, const FileCompression comp) {
        if(this->opened) {
            TWL_R_FAIL(ResultFileAlreadyOpened);
        }

        TWL_R_TRY(this->OpenImpl(mode));

        if(CanReadWithMode(this->mode)) {
            if(comp == fs::FileCompression::Auto) {
                // Check for LZ77 comp
                u32 lz_header;
                if(this->ReadBufferImpl(&lz_header, sizeof(lz_header)).IsSuccess()) {
                    TWL_R_TRY(this->SetOffsetImpl(0, Whence::Begin));

                    util::LzVersion lz_version;
                    if(util::LzValidateCompressed(lz_header, lz_version).IsSuccess()) {
                        this->comp = FileCompression::LZ77;
                        this->lz_ver = lz_version;

                        TWL_R_TRY(this->DecompressRead());
                    }
                    else {
                        // Not valid header, assume not compressed
                        this->comp = FileCompression::None;
                    }
                }
                else {
                    // File might be less than 4 bytes, assume not compressed
                    this->comp = FileCompression::None;
                }
            }
            else {
                this->comp = comp;
            }
        }
        else if(CanWriteWithMode(this->mode)) {
            this->comp = comp;
        }

        this->opened = true;

        TWL_R_SUCCEED();
    }

    Result File::SetOffset(const ssize_t offset, const Whence whence) {
        if(this->IsCompressed()) {
            TWL_R_TRY(this->decomp_rw.SetOffset(offset, whence));
        }
        else {
            TWL_R_TRY(this->SetOffsetImpl(offset, whence));
        }

        TWL_R_SUCCEED();
    }

    Result File::GetOffset(size_t &out_offset) {
        if(this->IsCompressed()) {
            out_offset = this->decomp_rw.GetBufferOffset();
        }
        else {
            TWL_R_TRY(this->GetOffsetImpl(out_offset));
        }

        TWL_R_SUCCEED();
    }

    Result File::ReadBuffer(void *read_buf, const size_t read_size) {
        if(this->IsCompressed()) {
            if(!CanReadWithMode(this->mode)) {
                TWL_R_FAIL(ResultReadNotSupported);
            }

            TWL_R_TRY(this->decomp_rw.ReadBuffer(read_buf, read_size));
        }
        else {
            TWL_R_TRY(this->ReadBufferImpl(read_buf, read_size));
        }

        TWL_R_SUCCEED();
    }

    Result File::WriteBuffer(const void *write_buf, const size_t write_size) {
        if(this->IsCompressed()) {
            if(!CanWriteWithMode(this->mode)) {
                TWL_R_FAIL(ResultWriteNotSupported);
            }

            TWL_R_TRY(this->decomp_rw.WriteBuffer(write_buf, write_size));
        }
        else {
            TWL_R_TRY(this->WriteBufferImpl(write_buf, write_size));
        }

        TWL_R_SUCCEED();
    }

    Result File::Close() {
        if(!this->IsOpened()) {
            TWL_R_FAIL(ResultFileAlreadyClosed);
        }

        if(this->IsCompressed()) {
            TWL_R_TRY(this->CompressWrite());
        }

        TWL_R_TRY(this->CloseImpl());
        TWL_R_SUCCEED();
    }

    Result StdioFile::OpenImpl(const FileMode mode) {
        this->mode = mode;

        if(this->file != nullptr) {
            TWL_R_FAIL(ResultUnableToOpenFile);
        }

        const char *conv_mode;
        switch(this->mode) {
            case FileMode::Read: {
                conv_mode = "rb";
                break;
            }
            case FileMode::Write: {
                conv_mode = "wb";
                break;
            }
            default: {
                TWL_R_FAIL(ResultInvalidFileMode);
            }
        }

        this->file = fopen(this->path.c_str(), conv_mode);
        if(this->file == nullptr) {
            TWL_R_FAIL(ResultUnableToOpenFile);
        }
        else {
            TWL_R_SUCCEED();
        }
    }

    Result StdioFile::GetSizeImpl(size_t &out_size) {
        const auto cur_pos = ftell(this->file);
        if(fseek(this->file, 0, SEEK_END) != 0) {
            TWL_R_FAIL(ResultUnableToSeekFile);
        }
        const auto f_size = ftell(this->file);
        if(fseek(this->file, cur_pos, SEEK_SET) != 0) {
            TWL_R_FAIL(ResultUnableToSeekFile);
        }

        out_size = f_size;
        TWL_R_SUCCEED();
    }

    Result StdioFile::SetOffsetImpl(const size_t offset, const Whence whence) {
        int conv_whence;
        switch(whence) {
            case Whence::Begin: {
                conv_whence = SEEK_SET;
                break;
            }
            case Whence::Current: {
                conv_whence = SEEK_CUR;
                break;
            }
            default: {
                TWL_R_FAIL(ResultInvalidSeekWhence);
            }
        }

        if(fseek(this->file, offset, conv_whence) != 0) {
            TWL_R_FAIL(ResultUnableToSeekFile);
        }
        else {
            TWL_R_SUCCEED();
        }
    }

    Result StdioFile::GetOffsetImpl(size_t &out_offset) {
        out_offset = ftell(this->file);
        TWL_R_SUCCEED();
    }

    Result StdioFile::ReadBufferImpl(void *read_buf, const size_t read_size) {
        if(read_size == 0) {
            TWL_R_SUCCEED();
        }

        if(fread(read_buf, read_size, 1, this->file) != 1) {
            TWL_R_FAIL(ResultUnableToReadFile);
        }
        else {
            TWL_R_SUCCEED();
        }
    }

    Result StdioFile::WriteBufferImpl(const void *write_buf, const size_t write_size) {
        if(write_size == 0) {
            TWL_R_SUCCEED();
        }

        if(fwrite(write_buf, write_size, 1, this->file) != 1) {
            TWL_R_FAIL(ResultUnableToWriteFile);
        }
        else {
            TWL_R_SUCCEED();
        }
    }

    Result StdioFile::CloseImpl() {
        if(this->file == nullptr) {
            TWL_R_FAIL(ResultUnableToCloseFile);
        }

        const auto res = fclose(this->file);
        this->file = nullptr;
        if(res != 0) {
            TWL_R_FAIL(ResultUnableToCloseFile);
        }
        else {
            TWL_R_SUCCEED();
        }
    }

    Result BufferFile::OpenImpl(const FileMode mode) {
        this->mode = mode;

        TWL_R_TRY(this->rw.SetOffset(0, Whence::Begin));
        TWL_R_SUCCEED();
    }

    Result BufferFile::SetOffsetImpl(const size_t offset, const Whence whence) {
        TWL_R_TRY(this->rw.SetOffset(offset, whence));
        TWL_R_SUCCEED();
    }

    Result BufferFile::ReadBufferImpl(void *read_buf, const size_t read_size) {
        if(!CanReadWithMode(this->mode)) {
            TWL_R_FAIL(ResultReadNotSupported);
        }

        if(read_size == 0) {
            TWL_R_SUCCEED();
        }

        TWL_R_TRY(this->rw.ReadBuffer(read_buf, read_size));
        TWL_R_SUCCEED();
    }

    Result BufferFile::WriteBufferImpl(const void *write_buf, const size_t write_size) {
        if(!CanWriteWithMode(this->mode)) {
            TWL_R_FAIL(ResultWriteNotSupported);
        }

        if(write_size == 0) {
            TWL_R_SUCCEED();
        }

        TWL_R_TRY(this->rw.WriteBuffer(write_buf, write_size));
        TWL_R_SUCCEED();
    }
    
    Result BufferFile::CloseImpl() {
        if(this->IsValid()) {
            this->rw.Dispose();
        }

        TWL_R_SUCCEED();
    }

}
 