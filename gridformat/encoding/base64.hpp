// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Encoding
 * \brief Encoder and stream using base64
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_BASE64_HPP_
#define GRIDFORMAT_COMMON_ENCODING_BASE64_HPP_

#include <array>
#include <vector>
#include <utility>
#include <cassert>
#include <algorithm>
#include <istream>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/istream_helper.hpp>
#include <gridformat/common/output_stream.hpp>
#include <gridformat/common/concepts.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace Base64Detail {

static constexpr auto alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static constexpr std::array<unsigned char, 256> letter_to_index = [] {
    std::array<unsigned char, 256> result;
    std::ranges::fill(result, 0);
    for (int i = 0; i < 64; ++i)
        result[static_cast<unsigned>(alphabet[i])] = i;
    return result;
} ();

}  // namespace Base64Detail
#endif  // DOXYGEN

namespace Base64 {

//! Return the number of decoded bytes for the given number of encoded bytes
std::size_t decoded_size(std::size_t encoded_size) {
    if (encoded_size%4 != 0)
        throw SizeError("Given size is not a multiple of 4");
    return encoded_size*3/4;
}

//! Return the number of encoded bytes for the given number of raw bytes
std::size_t encoded_size(std::size_t raw_size) {
    return 4*static_cast<std::size_t>(
        std::ceil(static_cast<double>(raw_size)/3.0)
    );
}

}  // namespace Base64

//! \addtogroup Encoding
//! \{

struct Base64Decoder {
    Serialization decode_from(std::istream& stream, std::size_t target_num_decoded_bytes) const {
        InputStreamHelper helper{stream};
        const auto encoded_size = Base64::encoded_size(target_num_decoded_bytes);
        std::string chars = helper.read_until_any_of("=", encoded_size);
        if (chars.size() != encoded_size)
            chars += helper.read_until_not_any_of("=");

        Serialization result{chars.size()};
        auto result_chars = result.template as_span_of<char>();
        std::ranges::move(std::move(chars), result_chars.begin());
        result.resize(decode(result_chars));
        return result;
    }

    template<std::size_t s>
    std::size_t decode(std::span<char, s> chars) const {
        if (chars.size() == 0)
            return 0;
        if (chars.size()%4 != 0)
            throw SizeError("Buffer size is not a multiple of 4");

        std::size_t in_offset = 0;
        std::size_t out_offset = 0;
        while (in_offset < chars.size()) {
            std::ranges::copy(
                _decode_triplet(chars.data() + in_offset),
                chars.data() + out_offset
            );
            in_offset += 4;
            out_offset += 3;
        }

        const auto end_chars = chars.subspan(chars.size() - 3);
        std::string_view end_str{end_chars.data(), end_chars.size()};
        const auto num_padding_chars = std::ranges::count(end_str, '=');
        return out_offset - (num_padding_chars > 0 ? num_padding_chars : 0);
    }

 private:
    std::array<char, 3> _decode_triplet(const char* in) const {
        using Base64Detail::letter_to_index;
        std::array<char, 3> result;
        result[0] = ((letter_to_index[in[0]] & 0b0011'1111) << 2) | ((letter_to_index[in[1]] & 0b0011'0000) >> 4);
        result[1] = ((letter_to_index[in[1]] & 0b0000'1111) << 4) | ((letter_to_index[in[2]] & 0b0011'1100) >> 2);
        result[2] = ((letter_to_index[in[2]] & 0b0000'0011) << 6) | ((letter_to_index[in[3]] & 0b0011'1111));
        return result;
    };
};

//! Options for formatted output of ranges with base64 encoding
struct Base64EncoderOptions {
    std::size_t num_cached_buffers = 4000;  //!< Number of triplets cached between write operations
};

//! Wrapper around a given stream to write output encoded with base64
template<typename OStream>
class Base64Stream : public OutputStreamWrapperBase<OStream> {
    using Byte = char;
    static_assert(sizeof(std::byte) == sizeof(Byte));
    static constexpr int buffer_size = 3;
    static constexpr int encoded_buffer_size = 4;

    // Top 6 bits of byte 0
    inline Byte _encode_sextet_0(const Byte* buffer) const {
        return Base64Detail::alphabet[((buffer[0] & 0b1111'1100) >> 2)];
    }
    // Bottom 2 bits of byte 0, Top 4 bits of byte 1
    inline Byte _encode_sextet_1(const Byte* buffer) const {
        return Base64Detail::alphabet[((buffer[0] & 0b0000'0011) << 4)
               | ((buffer[1] & 0b1111'0000) >> 4)];
    }
    // Bottom 4 bits of byte 1, Top 2 bits of byte 2
    inline Byte _encode_sextet_2(const Byte* buffer) const {
        return Base64Detail::alphabet[((buffer[1] & 0b0000'1111) << 2)
               | ((buffer[2] & 0b1100'0000) >> 6)];
    }
    // Bottom 6 bits of byte 2
    inline Byte _encode_sextet_3(const Byte* buffer) const {
        return Base64Detail::alphabet[(buffer[2] & 0b0011'1111)];
    }

 public:
    explicit Base64Stream(OStream& s, Base64EncoderOptions opts = {})
    : OutputStreamWrapperBase<OStream>(s)
    , _opts{std::move(opts)}
    {}

    template<typename T, std::size_t size>
    void write(std::span<T, size> data) {
        auto byte_span = std::as_bytes(data);
        const Byte* bytes = reinterpret_cast<const Byte*>(byte_span.data());
        _write(bytes, byte_span.size());
    }

 private:
    std::size_t _cache_size_in() const { return _opts.num_cached_buffers*buffer_size; }
    std::size_t _cache_size_out() const { return _opts.num_cached_buffers*encoded_buffer_size; }

    void _write(const Byte* data, std::size_t size) {
        const auto num_full_buffers = size/buffer_size;
        const auto num_full_caches = num_full_buffers/_opts.num_cached_buffers;
        for (const auto i : std::views::iota(std::size_t{0}, num_full_caches))
            _flush_full_cache(data + i*_cache_size_in());

        const auto processed_bytes = num_full_caches*_cache_size_in();
        if (size > processed_bytes)
            _flush_cache(data + processed_bytes, size - processed_bytes);
    }

    void _flush_full_cache(const Byte* data) {
        std::vector<Byte> cache(_cache_size_out());
        for (std::size_t i = 0; i < _cache_size_out()/encoded_buffer_size; ++i) {
            const std::size_t in_offset = i*buffer_size;
            const std::size_t out_offset = i*encoded_buffer_size;
            cache[out_offset + 0] = _encode_sextet_0(data + in_offset);
            cache[out_offset + 1] = _encode_sextet_1(data + in_offset);
            cache[out_offset + 2] = _encode_sextet_2(data + in_offset);
            cache[out_offset + 3] = _encode_sextet_3(data + in_offset);
        }
        this->_stream.write(std::span{cache});
    }

    void _flush_cache(const Byte* data, std::size_t num_bytes_in) {
        if (num_bytes_in == 0)
            return;
        if (num_bytes_in > _cache_size_in())
            throw SizeError("Number of bytes cannot be larger than cache size");

        const std::size_t num_full_buffers = num_bytes_in/buffer_size;
        const std::size_t residual = num_bytes_in%buffer_size;

        std::vector<Byte> cache(_cache_size_out());
        for (std::size_t i = 0; i < num_full_buffers; ++i) {
            const std::size_t in_offset = i*buffer_size;
            const std::size_t out_offset = i*encoded_buffer_size;
            cache[out_offset + 0] = _encode_sextet_0(data + in_offset);
            cache[out_offset + 1] = _encode_sextet_1(data + in_offset);
            cache[out_offset + 2] = _encode_sextet_2(data + in_offset);
            cache[out_offset + 3] = _encode_sextet_3(data + in_offset);
        }

        const std::size_t in_offset = num_full_buffers*buffer_size;
        const std::size_t out_offset = num_full_buffers*encoded_buffer_size;
        if (residual > 0) {
            Byte last_buffer[buffer_size] = {
                *(data + in_offset),
                residual > 1 ? *(data + in_offset + 1) : Byte{0},
                residual > 2 ? *(data + in_offset + 2) : Byte{0}
            };
            cache[out_offset] = _encode_sextet_0(last_buffer);
            cache[out_offset + 1] = _encode_sextet_1(last_buffer);
            cache[out_offset + 2] = residual > 1 ? _encode_sextet_2(last_buffer) : '=';
            cache[out_offset + 3] = residual > 2 ? _encode_sextet_3(last_buffer) : '=';
            this->_stream.write(std::span{cache.data(), out_offset + 4});
        } else {
            this->_stream.write(std::span{cache.data(), out_offset});
        }
    }

    Base64EncoderOptions _opts;
};

//! \} group Encoding

}  // namespace GridFormat

namespace GridFormat::Encoding {

//! \addtogroup Encoding
//! \{

//! Base64 encoder
struct Base64 {
    //! Return a base64 stream
    template<typename Stream>
    constexpr auto operator()(Stream& s) const noexcept {
        return Base64Stream{s, _opts};
    }

    //! Return an encoder instance with different options
    constexpr auto operator()(Base64EncoderOptions opts) const {
        Base64 other;
        other._opts = std::move(opts);
        return other;
    }

    //! Return a base64 encoder with the given options
    static Base64 with(Base64EncoderOptions opts) {
        Base64 enc;
        enc._opts = std::move(opts);
        return enc;
    }

 private:
    Base64EncoderOptions _opts = {};
};

//! Instance of the base64 encoder
inline constexpr Base64 base64;

//! \} group Encoding

}  // namespace GridFormat::Encoding

#endif  // GRIDFORMAT_COMMON_ENCODING_BASE64_HPP_
