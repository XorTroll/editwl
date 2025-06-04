
#pragma once
#include <twl/twl_Include.hpp>
#include <twl/util/util_Compression.hpp>
#include <twl/util/util_Align.hpp>
#include <cstdio>
#include <cstring>

namespace twl::fs {

    enum class FileMode : u8 {
        Invalid,
        Read,
        Write
    };

    inline constexpr bool CanReadWithMode(const FileMode mode) {
        return mode == FileMode::Read;
    }

    inline constexpr bool CanWriteWithMode(const FileMode mode) {
        return mode == FileMode::Write;
    }

    enum class Whence : u8 {
        Begin,
        Current
    };

    enum class FileCompression : u8 {
        Invalid,
        Auto,
        None,
        LZ77
    };

    class AbstractReaderWriter {
        public:
            virtual Result ReadBuffer(void *read_buf, const size_t read_size) = 0;
            virtual Result WriteBuffer(const void *write_buf, const size_t write_size) = 0;
            virtual Result SetOffset(const ssize_t offset, const Whence whence) = 0;
            virtual Result GetOffset(size_t &out_offset) = 0;
            virtual Result GetSize(size_t &out_size) = 0;

            inline Result SetAbsoluteOffset(const size_t offset) {
                TWL_R_TRY(this->SetOffset(offset, Whence::Begin));
                TWL_R_SUCCEED();
            }

            inline Result MoveOffset(const ssize_t offset) {
                TWL_R_TRY(this->SetOffset(offset, Whence::Current));
                TWL_R_SUCCEED();
            }

            template<typename T>
            inline Result Read(T &out_t) {
                TWL_R_TRY(this->ReadBuffer(std::addressof(out_t), sizeof(T)));
                TWL_R_SUCCEED();
            }

            template<typename T>
            inline Result Write(const T &t) {
                TWL_R_TRY(this->WriteBuffer(std::addressof(t), sizeof(T)));
                TWL_R_SUCCEED();
            }

            template<typename T>
            inline Result ReadLEB128(T &out_t) {
                out_t = {};
                while(true) {
                    u8 v;
                    TWL_R_TRY(this->Read(v));

                    out_t = (out_t << 7) | (v & 0x7F);
                    if(!(v & 0x80)) {
                        break;
                    }
                }

                TWL_R_SUCCEED();
            }

            template<typename C>
            inline Result ReadTerminatedString(std::basic_string<C> &out_str, const C terminator, const size_t tmp_buf_size = 0x200) {
                // Note: this approach is significantly faster than the classic read-char-by-char approach, specially when reading hundreds of strings ;)
                out_str.clear();
                
                size_t old_offset;
                TWL_R_TRY(this->GetOffset(old_offset));
                size_t f_size;
                TWL_R_TRY(this->GetSize(f_size));
                const auto available_size = f_size - old_offset;
                if(available_size == 0) {
                    TWL_R_FAIL(ResultEndOfData);
                }
                const auto r_size = std::min(tmp_buf_size, available_size);
                auto buf = new C[r_size]();
                ScopeGuard on_exit_cleanup([&]() {
                    delete[] buf;
                });

                TWL_R_TRY(this->ReadBuffer(buf, r_size));

                for(size_t i = 0; i < r_size; i++) {
                    if(buf[i] == terminator) {
                        TWL_R_TRY(this->SetAbsoluteOffset(old_offset + i + 1));
                        out_str = std::basic_string<C>(buf, i);
                        TWL_R_SUCCEED();
                    }
                }
                
                TWL_R_TRY(this->SetAbsoluteOffset(old_offset));
                return ReadNullTerminatedString(out_str, tmp_buf_size * 2);
            }
            
            template<typename C>
            inline Result ReadNullTerminatedString(std::basic_string<C> &out_str, const size_t tmp_buf_size = 0x200) {
                return this->ReadTerminatedString(out_str, static_cast<C>(0), tmp_buf_size);
            }

            template<typename C>
            inline Result WriteString(const std::basic_string<C> &str) {
                return this->WriteBuffer(str.c_str(), str.length() * sizeof(C));
            }

            inline Result WriteCString(const char *str) {
                const auto str_len = std::strlen(str);
                return this->WriteBuffer(str, str_len);
            }

            inline Result WriteEnsureAlignmentPadding(const size_t align, size_t &out_pad_size) {
                if(align == 0) {
                    TWL_R_SUCCEED();
                }

                size_t cur_offset;
                TWL_R_TRY(this->GetOffset(cur_offset));
                out_pad_size = util::AlignUp(cur_offset, align) - cur_offset;

                auto zero_buf = new u8[out_pad_size]();
                ScopeGuard cleanup([&]() {
                    delete[] zero_buf;
                });

                TWL_R_TRY(this->WriteBuffer(zero_buf, out_pad_size));

                TWL_R_SUCCEED();
            }

            inline Result WriteEnsureAlignment(const size_t align) {
                size_t dummy_size;
                TWL_R_TRY(this->WriteEnsureAlignmentPadding(align, dummy_size));
                TWL_R_SUCCEED();
            }

            template<typename T>
            inline Result WriteVector(const std::vector<T> &vec) {
                return this->WriteBuffer(vec.data(), vec.size() * sizeof(T));
            }

            template<typename C>
            inline Result WriteNullTerminatedString(const std::basic_string<C> &str) {
                TWL_R_TRY(this->WriteString(str));
                TWL_R_TRY(this->Write(static_cast<C>(0)));

                TWL_R_SUCCEED();
            }
    };

    class BufferReaderWriter : public AbstractReaderWriter {
        private:
            void *buf;
            size_t buf_size;
            size_t offset;

        public:
            constexpr BufferReaderWriter() : buf(nullptr), buf_size(0), offset(0) {}
            
            inline BufferReaderWriter(const size_t buf_size) : buf(nullptr), buf_size(0), offset(0) {
                this->CreateAllocate(buf_size);
            }

            inline BufferReaderWriter(void *buf, const size_t buf_size, const bool transfer_ownership = true) : buf(nullptr), buf_size(0), offset(0) {
                this->CreateFrom(buf, buf_size, transfer_ownership);
            }

            BufferReaderWriter(const BufferReaderWriter&) = delete;
            BufferReaderWriter(BufferReaderWriter&&) = default;

            void CreateAllocate(const size_t buf_size);
            void CreateFrom(void *buf, const size_t buf_size, const bool transfer_ownership = true);

            inline void Dispose() {
                if(this->buf != nullptr) {
                    auto del_buf = reinterpret_cast<u8*>(this->buf);
                    delete[] del_buf;
                    this->buf = nullptr;
                }

                this->buf_size = 0;
                this->offset = 0;
            }

            inline bool IsValid() {
                return (this->buf != nullptr) && (this->buf_size > 0);
            }

            inline void *GetBuffer() {
                return this->buf;
            }

            inline size_t GetBufferSize() {
                return this->buf_size;
            }

            Result SetOffset(const ssize_t offset, const Whence whence) override;
            
            inline size_t GetBufferOffset() {
                return this->offset;
            }

            inline Result GetOffset(size_t &out_offset) override {
                out_offset = this->GetBufferOffset();
                TWL_R_SUCCEED();
            }

            inline Result GetSize(size_t &out_size) override {
                out_size = this->GetBufferSize();
                TWL_R_SUCCEED();
            }
            
            Result ReadBuffer(void *read_buf, const size_t read_size) override;
            Result WriteBuffer(const void *write_buf, const size_t write_size) override;
    };

    class File : public AbstractReaderWriter {
        protected:
            FileMode mode;

        private:
            bool opened;
            FileCompression comp;
            util::LzVersion lz_ver;
            BufferReaderWriter decomp_rw;

            Result DecompressRead();
            Result CompressWrite();

            Result Open(const FileMode mode, const FileCompression comp);

        public:
            constexpr File() : mode(FileMode::Invalid), opened(false), comp(FileCompression::Invalid), lz_ver(util::LzVersion::Invalid), decomp_rw() {}

            File(const File&) = delete;
            File(File&&) = default;

            inline bool IsOpened() {
                return this->opened;
            }

            inline constexpr bool IsCompressed() {
                return (this->comp != FileCompression::Invalid) && (this->comp != FileCompression::None);    
            }

            virtual Result OpenImpl(const FileMode mode) = 0;
            virtual Result GetSizeImpl(size_t &out_size) = 0;
            virtual Result SetOffsetImpl(const size_t offset, const Whence whence) = 0;
            virtual Result GetOffsetImpl(size_t &out_offset) = 0;
            virtual Result ReadBufferImpl(void *read_buf, const size_t read_size) = 0;
            virtual Result WriteBufferImpl(const void *write_buf, const size_t write_size) = 0;
            virtual Result CloseImpl() = 0;

            inline Result OpenRead(const FileCompression comp = FileCompression::Auto) {
                TWL_R_TRY(this->Open(fs::FileMode::Read, comp));
                TWL_R_SUCCEED();
            }

            inline Result OpenWrite(const FileCompression comp = FileCompression::None) {
                TWL_R_TRY(this->Open(fs::FileMode::Write, comp));
                TWL_R_SUCCEED();
            }

            inline Result GetSize(size_t &out_size) override {
                if(this->IsCompressed()) {
                    out_size = this->decomp_rw.GetBufferSize();
                }
                else {
                    TWL_R_TRY(this->GetSizeImpl(out_size));
                }

                TWL_R_SUCCEED();
            }

            Result SetOffset(const ssize_t offset, const Whence whence) override;
            Result GetOffset(size_t &out_offset) override;
            Result ReadBuffer(void *read_buf, const size_t read_size) override;
            Result WriteBuffer(const void *write_buf, const size_t write_size) override;

            Result Close();
    };

    class StdioFile : public File {
        private:
            std::string path;
            FILE *file;

        public:
            inline StdioFile(const std::string &path) : File(), path(path), file(nullptr) {}

            StdioFile(const StdioFile&) = delete;
            StdioFile(StdioFile&&) = default;

            Result OpenImpl(const FileMode mode) override;
            Result GetSizeImpl(size_t &out_size) override;
            Result SetOffsetImpl(const size_t offset, const Whence whence) override;
            Result GetOffsetImpl(size_t &out_offset) override;
            Result ReadBufferImpl(void *read_buf, const size_t read_size) override;
            Result WriteBufferImpl(const void *write_buf, const size_t write_size) override;
            Result CloseImpl() override;

            inline std::string &GetPath() {
                return this->path;
            }
    };

    class BufferFile : public File {
        private:
            BufferReaderWriter rw;

        public:
            constexpr BufferFile() : File(), rw() {}
            inline BufferFile(size_t buf_size) : File(), rw(buf_size) {}
            inline BufferFile(void *buf, size_t buf_size, const bool transfer_ownership = true) : File(), rw(buf, buf_size, transfer_ownership) {}

            BufferFile(const BufferFile&) = delete;
            BufferFile(BufferFile&&) = default;

            inline void CreateAllocate(size_t buf_size) {
                this->Close();
                this->rw.CreateAllocate(buf_size);
            }

            inline void CreateFrom(void *buf, size_t buf_size, const bool transfer_ownership = true) {
                this->Close();
                this->rw.CreateFrom(buf, buf_size, transfer_ownership);
            }

            inline void Dispose() {
                this->Close();
                this->rw.Dispose();
            }

            inline bool IsValid() {
                return this->rw.IsValid();
            }

            inline void *GetBuffer() {
                return this->rw.GetBuffer();
            }

            inline size_t GetBufferSize() {
                return this->rw.GetBufferSize();
            }

            Result OpenImpl(const FileMode mode) override;
            
            inline Result GetSizeImpl(size_t &out_size) override {
                out_size = this->rw.GetBufferSize();
                TWL_R_SUCCEED();
            }
            
            Result SetOffsetImpl(const size_t offset, const Whence whence) override;

            inline Result GetOffsetImpl(size_t &out_offset) override {
                out_offset = this->rw.GetBufferOffset();
                TWL_R_SUCCEED();
            }

            Result ReadBufferImpl(void *read_buf, const size_t read_size) override;
            Result WriteBufferImpl(const void *write_buf, const size_t write_size) override;
            Result CloseImpl() override;
    };

}
