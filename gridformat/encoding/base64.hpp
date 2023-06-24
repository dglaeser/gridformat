// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Encoding
 * \brief Encoder and stream using base64
 */
#ifndef GRIDFORMAT_COMMON_ENCODING_BASE64_HPP_
#define GRIDFORMAT_COMMON_ENCODING_BASE64_HPP_

#include <vector>
#include <utility>
#include <cassert>
#include <algorithm>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/output_stream.hpp>

namespace GridFormat {

//! \addtogroup Encoding
//! \{

//! Options for fomatted output of ranges with base64 encoding
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

    static constexpr unsigned char base64_alphabet[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    // Top 6 bits of byte 0
    inline Byte _encodeSextet0(const Byte* buffer) const {
        return base64_alphabet[((buffer[0] & 0b1111'1100) >> 2)];
    }
    // Bottom 2 bits of byte 0, Top 4 bits of byte 1
    inline Byte _encodeSextet1(const Byte* buffer) const {
        return base64_alphabet[((buffer[0] & 0b0000'0011) << 4)
               | ((buffer[1] & 0b1111'0000) >> 4)];
    }
    // Bottom 4 bits of byte 1, Top 2 bits of byte 2
    inline Byte _encodeSextet2(const Byte* buffer) const {
        return base64_alphabet[((buffer[1] & 0b0000'1111) << 2)
               | ((buffer[2] & 0b1100'0000) >> 6)];
    }
    // Bottom 6 bits of byte 2
    inline Byte _encodeSextet3(const Byte* buffer) const {
        return base64_alphabet[(buffer[2] & 0b0011'1111)];
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
            cache[out_offset + 0] = _encodeSextet0(data + in_offset);
            cache[out_offset + 1] = _encodeSextet1(data + in_offset);
            cache[out_offset + 2] = _encodeSextet2(data + in_offset);
            cache[out_offset + 3] = _encodeSextet3(data + in_offset);
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
            cache[out_offset + 0] = _encodeSextet0(data + in_offset);
            cache[out_offset + 1] = _encodeSextet1(data + in_offset);
            cache[out_offset + 2] = _encodeSextet2(data + in_offset);
            cache[out_offset + 3] = _encodeSextet3(data + in_offset);
        }

        const std::size_t in_offset = num_full_buffers*buffer_size;
        const std::size_t out_offset = num_full_buffers*encoded_buffer_size;
        if (residual > 0) {
            Byte last_buffer[buffer_size] = {
                *(data + in_offset),
                residual > 1 ? *(data + in_offset + 1) : Byte{0},
                residual > 2 ? *(data + in_offset + 2) : Byte{0}
            };
            cache[out_offset] = _encodeSextet0(last_buffer);
            cache[out_offset + 1] = _encodeSextet1(last_buffer);
            cache[out_offset + 2] = residual > 1 ? _encodeSextet2(last_buffer) : '=';
            cache[out_offset + 3] = residual > 2 ? _encodeSextet3(last_buffer) : '=';
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
