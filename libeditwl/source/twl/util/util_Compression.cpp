#include <twl/util/util_Compression.hpp>
#include <twl/util/util_Align.hpp>
#include <cstring>

namespace twl::util {

    namespace {

        // TODO: improve these?

        void Lz10CompressSearch(const u8 *data, const size_t data_size, const size_t pos, size_t &match, size_t &length)
		{
			size_t num = 4096;
			size_t num2 = 18;
			match = 0;
			length = 0;
			size_t num3 = pos - num;
			if (num3 < 0)
			{
				num3 = 0;
			}
			for (size_t i = num3; i < pos; i++)
			{
				size_t num4 = 0;
				while ((num4 < num2) && ((i + num4) < pos) && ((pos + num4) < data_size) && (data[pos + num4] == data[i + num4]))
				{
					num4++;
				}
				if (num4 > length)
				{
					match = i;
					length = num4;
				}
				if (length == num2)
				{
					return;
				}
			}
		}

    }

    Result LzValidateCompressed(const u32 lz_header, LzVersion &out_ver) {
        out_ver = static_cast<LzVersion>(lz_header & 0xff);
        if((out_ver != LzVersion::LZ10) && (out_ver != LzVersion::LZ11)) {
            TWL_R_FAIL(ResultCompressionInvalidLzFormat);
        }

        TWL_R_SUCCEED();
    }

    Result LzCompress(const u8 *data, const size_t data_size, const LzVersion ver, const u32 repeat_size, u8 *&out_data, size_t &out_size) {
        if(ver != LzVersion::LZ10) {
            TWL_R_FAIL(ResultCompressionInvalidLzFormat);
        }

        const auto tmp_data_size = 2 * data_size + 4;
        auto tmp_array = new u8[tmp_data_size]();
        *(u32*)tmp_array = static_cast<u32>(data_size << 8 | 16);
        size_t offset = sizeof(u32);
        u8 array[16] = {};
        size_t i = 0;
        while (i < data_size)
        {
            size_t num = 0;
            u8 b = 0;
            for (int j = 0; j < 8; j++)
            {
                if (i >= data_size)
                {
                    array[num++] = 0;
                }
                else
                {
                    size_t num2 = 0;
                    size_t num3 = 0;
                    Lz10CompressSearch(data, data_size, i, num2, num3);
                    size_t num4 = i - num2 - 1;
                    if (num3 > 2)
                    {
                        b |= (u8)(1 << (7 - j));
                        array[num++] = (u8)((((num3 - 3) & 15) << 4) + (num4 >> 8 & 15));
                        array[num++] = (u8)(num4 & 255);
                        i += num3;
                    }
                    else
                    {
                        array[num++] = data[i++];
                    }
                }
            }
            tmp_array[offset] = b;
            offset++;
            for (size_t k = 0; k < num; k++)
            {
                tmp_array[offset] = array[k];
                offset++;
            }
        }

        out_data = new u8[offset]();
        out_size = offset;
        std::memcpy(out_data, tmp_array, offset);
        delete[] tmp_array;

        TWL_R_SUCCEED();
    }

    Result LzDecompress(const u8 *data, u8 *&out_data, size_t &out_size, LzVersion &out_ver, size_t &out_used_data_size) {
        size_t offset = 0;
        const auto lz_header = *reinterpret_cast<const u32*>(data);
        offset += sizeof(u32);

        LzVersion ver;
        TWL_R_TRY(LzValidateCompressed(lz_header, ver));

        out_size = lz_header >> 8;
        out_ver = ver;
        if((out_size == 0) && (ver == LzVersion::LZ11)) {
            out_size = *reinterpret_cast<const u32*>(data + offset);
            offset += sizeof(u32);
        }
        out_data = new u8[out_size]();

        size_t out_offset = 0;
        while(out_offset < out_size) {
            const auto cur_byte = data[offset];
            offset++;

            for(u8 i = 0; i < 8; i++) {
                if(out_offset >= out_size) {
                    break;
                }

                const auto bit = 8 - (i + 1);
                if((cur_byte >> bit) & 1) {
                    const size_t msb_len = data[offset];
                    offset++;
                    const size_t lsb = data[offset]; // 4 bits (length) + 12 bits (disp)
                    offset++;

                    auto length = msb_len >> 4; // 4 high bits
                    auto disp = ((msb_len & 0xF) << 8) + lsb; // 4 low bits * 0x100 + lsb

                    if(ver == LzVersion::LZ10) {
                        length += 3;
                    }
                    else if(length > 1) {
                        length++;
                    }
                    else if(length == 0) {
                        length = (msb_len & 15) << 4;
                        length += lsb >> 4;
                        length += 0x11;
                        const size_t msb = data[offset];
                        offset++;
                        disp = ((lsb & 15) << 8) + msb;
                    }
                    else {
                        length = (msb_len & 15) << 12;
                        length += lsb << 4;
                        const size_t byte_1 = data[offset];
                        offset++;
                        const size_t byte_2 = data[offset];
                        offset++;
                        length += byte_1 >> 4;
                        length += 0x111;
                        disp = ((byte_1 & 15) << 8) + byte_2;
                    }

                    const auto start_offset = out_offset - disp - 1;
                    for(size_t j = 0; j < length; j++) {
                        const auto val = out_data[start_offset + j];
                        out_data[out_offset] = val;
                        out_offset++;
                    }
                }
                else {
                    out_data[out_offset] = data[offset];
                    offset++;
                    out_offset++;
                }
            }
        }

        out_used_data_size = offset;
        TWL_R_SUCCEED();
    }

}
